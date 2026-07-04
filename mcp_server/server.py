"""
ESP32 Marauder MCP Server

Exposes the Marauder serial command interface as MCP tools so any MCP client
(Claude Desktop, Claude Code, etc.) can drive the hardware directly.

Serial data is routed back to this host over USB — SD-card PCAP writing is
disabled automatically on connect so all scan/sniff output streams through
the serial port for AI analysis.

Run:
    pip install mcp pyserial
    python server.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import asyncio
import datetime
import json
import os
import sys
import time
import threading
import argparse
from pathlib import Path
from typing import Optional
import serial
import serial.tools.list_ports
from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp import types

# ---------------------------------------------------------------------------
# Serial state — one connection shared across all tool calls
# ---------------------------------------------------------------------------

_serial: Optional[serial.Serial] = None
_lock = threading.Lock()
_active_pcap_file: Optional[Path] = None

PROMPT = b"> "          # Marauder prompt that marks end of response
DEFAULT_TIMEOUT = 8.0   # seconds to wait for the prompt after a command


def _is_termux() -> bool:
    """Detect whether we are running inside Termux on Android."""
    return (
        os.path.isdir("/data/data/com.termux")
        or "com.termux" in os.environ.get("PREFIX", "")
        or "com.termux" in os.environ.get("PATH", "")
        or "com.termux" in os.environ.get("PROOT_L2S_DIR", "")
        or os.path.isdir("/sdcard")
    )


def _open_serial(port: str, baud: int) -> serial.Serial:
    """Open serial port or socket:// URL and return a Serial-compatible object."""
    if port.startswith("socket://"):
        return serial.serial_for_url(port, baudrate=baud, timeout=0.1)
    return serial.Serial(port, baud, timeout=0.1)


def _is_socket() -> bool:
    """True when the current connection is a TCP socket (Termux bridge mode)."""
    if _serial is None:
        return False
    return getattr(_serial, "port", "").startswith("socket://")

# ---------------------------------------------------------------------------
# Capture buffer — populated by scan_and_capture, read by get_capture
# ---------------------------------------------------------------------------

_capture_buffer: str = ""
_capture_meta: dict = {}


def _handle_binary_buffers(raw_bytes: bytearray) -> bytearray:
    """Scan raw bytes for [BUF/BEGIN]...[BUF/CLOSE] blocks, write/append them to files on the host,
    and return the cleaned bytearray with those blocks removed.
    """
    global _active_pcap_file
    cleaned = bytearray()
    cursor = 0
    while True:
        begin_idx = raw_bytes.find(b"[BUF/BEGIN]", cursor)
        if begin_idx == -1:
            cleaned.extend(raw_bytes[cursor:])
            break
        cleaned.extend(raw_bytes[cursor:begin_idx])
        
        close_idx = raw_bytes.find(b"[BUF/CLOSE]", begin_idx + 11)
        if close_idx == -1:
            pcap_data = raw_bytes[begin_idx + 11:]
            cursor = len(raw_bytes)
        else:
            pcap_data = raw_bytes[begin_idx + 11:close_idx]
            cursor = close_idx + 11
            
        if pcap_data:
            try:
                magic = pcap_data[:4]
                is_new_pcap = magic in (b'\xa1\xb2\xc3\xd4', b'\xd4\xc3\xb2\xa1')
                
                if is_new_pcap:
                    d = _capture_dir()
                    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                    _active_pcap_file = d / f"marauder_{ts}.pcap"
                    _active_pcap_file.write_bytes(pcap_data)
                    print(f"[marauder-mcp] Started new local PCAP: {_active_pcap_file} ({len(pcap_data)} bytes)", file=sys.stderr)
                elif _active_pcap_file is not None:
                    with open(_active_pcap_file, "ab") as f:
                        f.write(pcap_data)
                    print(f"[marauder-mcp] Appended {len(pcap_data)} bytes to {_active_pcap_file}", file=sys.stderr)
                else:
                    d = _capture_dir()
                    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                    fallback_file = d / f"marauder_{ts}.log"
                    fallback_file.write_bytes(pcap_data)
                    print(f"[marauder-mcp] Saved non-pcap traffic block ({len(pcap_data)} bytes) to {fallback_file}", file=sys.stderr)
            except Exception as e:
                print(f"[marauder-mcp] Error handling USB streamed traffic: {e}", file=sys.stderr)
                
        if cursor >= len(raw_bytes):
            break
            
    return cleaned


def _read_until_prompt(ser: serial.Serial, timeout: float = DEFAULT_TIMEOUT) -> str:
    """Read bytes from ser until the Marauder '> ' prompt appears or timeout."""
    buf = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf.extend(chunk)
            if buf.endswith(PROMPT):
                cleaned = _handle_binary_buffers(buf[: -len(PROMPT)])
                return cleaned.decode("utf-8", errors="replace").strip()
        else:
            time.sleep(0.02)
    cleaned = _handle_binary_buffers(buf)
    return cleaned.decode("utf-8", errors="replace").strip()


def _should_append_serial(cmd: str) -> bool:
    cmd_lower = cmd.strip().lower()
    if any(cmd_lower.startswith(x) for x in ("sniff", "attack", "scanall")):
        return "-serial" not in cmd_lower
    return False


def _send(command: str, timeout: float = DEFAULT_TIMEOUT) -> str:
    global _serial, _active_pcap_file
    cmd_to_send = command.strip()
    
    # Automatically reset active PCAP file on new scans
    normalized = cmd_to_send.lower()
    if any(normalized.startswith(x) for x in ("scan", "sniff", "attack")):
        _active_pcap_file = None
        
    # Append -serial to commands that stream packet/log buffers
    if _should_append_serial(cmd_to_send):
        cmd_to_send += " -serial"
        
    with _lock:
        if _serial is None or not _serial.is_open:
            return "ERROR: Not connected. Use connect() first."
        if not _is_socket():
            _serial.reset_input_buffer()
        _serial.write((cmd_to_send + "\n").encode())
        _serial.flush()
        return _read_until_prompt(_serial, timeout)


def _stream_for(duration: float) -> str:
    """Read raw serial output for `duration` seconds (no command sent, no prompt wait)."""
    with _lock:
        if _serial is None or not _serial.is_open:
            return ""
        buf = bytearray()
        deadline = time.monotonic() + duration
        while time.monotonic() < deadline:
            try:
                waiting = _serial.in_waiting
            except Exception:
                waiting = 0
            chunk = _serial.read(waiting or 64)
            if chunk:
                buf.extend(chunk)
            else:
                time.sleep(0.02)
    cleaned = _handle_binary_buffers(buf)
    return cleaned.decode("utf-8", errors="replace")


def _fix_bt_output(raw: str) -> str:
    """Insert newlines before BT entries on NO_SCREEN builds."""
    import re
    return re.sub(r"(?<!\n)(?=-?\d+ Device:)", "\n", raw).strip()


def _enable_pcap_capture() -> None:
    """Make sure SavePCAP is enabled so the Marauder captures packets to the buffer."""
    _send("settings -s SavePCAP enable", timeout=4.0)


def _capture_dir() -> Path:
    """Return the best writable capture directory for the current platform.

    On Termux: prefers ~/storage/downloads/marauder_captures (visible in the
    Android Files app) when termux-setup-storage has been run; falls back to
    ~/marauder_captures inside the Termux sandbox otherwise.
    On Linux: ~/marauder_captures.
    """
    if _is_termux():
        d = None
        for p in [Path("/sdcard/Download"), Path("/storage/emulated/0/Download"), Path.home() / "storage" / "downloads"]:
            if p.is_dir():
                d = p / "marauder_captures"
                break
        if d is None:
            d = Path.home() / "marauder_captures"
    else:
        d = Path.home() / "marauder_captures"
    d.mkdir(parents=True, exist_ok=True)
    return d


# ---------------------------------------------------------------------------
# MCP server setup
# ---------------------------------------------------------------------------

app = Server("esp32-marauder")

# Supported scan types for scan_and_capture
SCAN_COMMANDS: dict[str, str] = {
    "scanall":    "scanall",
    "beacon":     "sniffbeacon",
    "probe":      "sniffprobe",
    "deauth":     "sniffdeauth",
    "pmkid":      "sniffpmkid",
    "raw":        "sniffraw",
    "pwn":        "sniffpwn",
    "bt":         "sniffbt -t flock",
    "airtag":     "sniffbt -t airtag",
    "skim":       "sniffskim",
    "sae":        "sniffsae",
    "multissid":  "sniffmultissid",
}


@app.list_tools()
async def list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="list_ports",
            description="List serial ports available on the host.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="connect",
            description=(
                "Open a serial connection to the ESP32 Marauder. "
                "Automatically disables SD-card PCAP writing so all scan output "
                "streams back through USB serial to this host."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "port": {
                        "type": ["string", "null"],
                        "description": "Serial port, e.g. /dev/ttyUSB0 or COM3. Auto-detected if omitted.",
                    },
                    "baud": {
                        "type": ["integer", "null"],
                        "description": "Baud rate (default 115200).",
                        "default": 115200,
                    },
                },
            },
        ),
        types.Tool(
            name="disconnect",
            description="Close the serial connection to the ESP32.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="connection_status",
            description="Check whether the serial connection is open and which port it uses.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="send_command",
            description=(
                "Send any Marauder serial command and return the response. "
                "Examples: 'help', 'scanall', 'stopscan', 'attack -t deauth', "
                "'list -a', 'settings', 'channel -s 6'."
            ),
            inputSchema={
                "type": "object",
                "required": ["command"],
                "properties": {
                    "command": {"type": "string", "description": "The Marauder command string to execute."},
                    "timeout": {"type": "number", "description": "Seconds to wait for prompt (default 8).", "default": 8.0},
                },
            },
        ),
        types.Tool(
            name="read_output",
            description="Read pending bytes from the serial buffer without sending a command. Useful for polling ongoing scan output.",
            inputSchema={
                "type": "object",
                "properties": {
                    "duration": {"type": "number", "description": "How many seconds to collect (default 2).", "default": 2.0}
                },
            },
        ),
        # ---- high-level capture tools ----------------------------------------
        types.Tool(
            name="scan_and_capture",
            description=(
                "Run a scan or sniff operation and capture ALL output through USB serial "
                "back to this Linux host — nothing is written to the SD card. "
                "After the capture window, stops the scan and retrieves the AP/station lists. "
                "Results are stored in the capture buffer (use get_capture to re-read them).\n\n"
                "Output line formats by scan_type:\n"
                "  scanall  → '<rssi> Ch: <ch> <bssid> ESSID: <ssid> <cap_hex...>' (new APs only)\n"
                "             '<n>: ap: <bssid> -> sta: <mac>' (new stations only)\n"
                "  beacon   → same AP format as scanall but fires for EVERY beacon (high volume)\n"
                "  deauth   → '<rssi> Ch: <ch> <src_mac> -> <dst_mac>'\n"
                "  pmkid    → 'Received EAPOL: <bssid>' + periodic stats block every ~1 s\n"
                "  raw      → periodic stats only (no per-packet text); use for traffic volume metrics\n"
                "  bt/airtag → '<rssi> Device: <name_or_mac>' (one per BT advertisement)\n\n"
                "scan_type options: scanall, beacon, probe, deauth, pmkid, raw, pwn, "
                "bt, airtag, skim, sae, multissid."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "scan_type": {
                        "type": "string",
                        "description": "Which scan/sniff to run (default: scanall).",
                        "default": "scanall",
                    },
                    "duration": {
                        "type": "number",
                        "description": "How many seconds to capture (default: 30).",
                        "default": 30.0,
                    },
                },
            },
        ),
        types.Tool(
            name="get_capture",
            description="Return the raw capture buffer from the most recent scan_and_capture call for analysis.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="save_capture_local",
            description=(
                "Save the capture buffer to a file on this device. "
                "Saves as JSON (structured) + TXT (raw). "
                "On Termux/Android saves to ~/storage/downloads/marauder_captures/ "
                "(visible in Android Files app) if termux-setup-storage was run, "
                "otherwise ~/marauder_captures/. On Linux saves to ~/marauder_captures/."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "path": {
                        "type": ["string", "null"],
                        "description": "Directory or full file path on this host (optional).",
                    }
                },
            },
        ),
        # ---- convenience wrappers --------------------------------------------
        types.Tool(
            name="scan_wifi",
            description="Start a WiFi scan (scanall). Use read_output to poll live results.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="stop_scan",
            description="Stop any active scan (stopscan).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_access_points",
            description="List discovered access points (list -a).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_stations",
            description="List discovered stations / clients (list -c).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_ssids",
            description="List the SSID list (list -s).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_probes",
            description="List captured probe SSIDs (list -p).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="get_settings",
            description="Print current Marauder settings.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="read_local_file",
            description=(
                "Read a file from the local Linux host filesystem and return its contents. "
                "Use this to read capture files, logs, or any other local file for analysis."
            ),
            inputSchema={
                "type": "object",
                "required": ["path"],
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Absolute or ~-relative path to the file on this Linux host.",
                    }
                },
            },
        ),
        types.Tool(
            name="flash_firmware",
            description=(
                "Flash/update the ESP32 Marauder firmware using esptool. "
                "Automatically handles closing the active serial connection before flashing, "
                "then runs the flash command and reports status."
            ),
            inputSchema={
                "type": "object",
                "required": ["bin_path"],
                "properties": {
                    "bin_path": {
                        "type": "string",
                        "description": "Path to the compiled firmware binary (.bin file).",
                    },
                    "port": {
                        "type": ["string", "null"],
                        "description": "Serial port. Auto-detected if omitted.",
                    },
                    "baud": {
                        "type": ["integer", "null"],
                        "description": "Flash baud rate (default 921600).",
                        "default": 921600,
                    },
                },
            },
        ),
        types.Tool(
            name="attack",
            description="Launch a specified Wi-Fi attack against selected targets.",
            inputSchema={
                "type": "object",
                "required": ["attack_type"],
                "properties": {
                    "attack_type": {
                        "type": "string",
                        "enum": ["deauth", "beacon", "probe", "funny", "rickroll", "badmsg", "sleep", "sae", "csa", "quiet"],
                        "description": "Type of attack to launch."
                    },
                    "options": {
                        "type": ["string", "null"],
                        "description": "Optional flags/parameters (e.g. '-c' to target clients, '-s <mac>' override source MAC, '-d <mac>' override destination MAC)."
                    }
                }
            }
        ),
        types.Tool(
            name="select_targets",
            description="Select access points, stations/clients, or SSIDs by indices (comma-separated list of numbers, or 'all'). Selection toggles the select status on the device.",
            inputSchema={
                "type": "object",
                "required": ["target_type", "indices"],
                "properties": {
                    "target_type": {
                        "type": "string",
                        "enum": ["ap", "station", "ssid"],
                        "description": "The category of targets to select."
                    },
                    "indices": {
                        "type": "string",
                        "description": "Comma-separated list of indices (e.g. '0,1,3') or 'all' to select all, or filter expression (e.g. '-f \"equals MyWiFi\"')."
                    }
                }
            }
        ),
        types.Tool(
            name="evil_portal",
            description="Manage the Evil Portal phishing module. Starts/stops the portal or loads HTML.",
            inputSchema={
                "type": "object",
                "required": ["action"],
                "properties": {
                    "action": {
                        "type": "string",
                        "enum": ["start", "stop", "sethtml"],
                        "description": "Action to perform."
                    },
                    "html_file": {
                        "type": ["string", "null"],
                        "description": "Name of the HTML portal file to load (required for 'start' with custom file and 'sethtml')."
                    }
                }
            }
        ),
        types.Tool(
            name="manage_ssids",
            description="Add, remove, or generate random SSIDs in the target SSID list.",
            inputSchema={
                "type": "object",
                "required": ["action"],
                "properties": {
                    "action": {
                        "type": "string",
                        "enum": ["add", "remove", "generate"],
                        "description": "Action to perform."
                    },
                    "name": {
                        "type": ["string", "null"],
                        "description": "SSID name/ESSID to add (required for 'add')."
                    },
                    "count": {
                        "type": ["integer", "null"],
                        "description": "Number of random SSIDs to generate (required for 'generate')."
                    },
                    "index": {
                        "type": ["integer", "null"],
                        "description": "Index of SSID to remove (required for 'remove')."
                    }
                }
            }
        ),
        types.Tool(
            name="change_channel",
            description="Set or query the Wi-Fi channel.",
            inputSchema={
                "type": "object",
                "properties": {
                    "channel": {
                        "type": ["integer", "null"],
                        "description": "Channel number (1-14). If omitted, just queries current channel."
                    }
                }
            }
        ),
        types.Tool(
            name="reboot_device",
            description="Reboot the ESP32 Marauder hardware.",
            inputSchema={"type": "object", "properties": {}}
        ),
        types.Tool(
            name="clear_lists",
            description="Clear the discovered access points, stations, or SSIDs lists.",
            inputSchema={
                "type": "object",
                "required": ["list_type"],
                "properties": {
                    "list_type": {
                        "type": "string",
                        "enum": ["ap", "station", "ssid", "all"],
                        "description": "Which list to clear."
                    }
                }
            }
        ),
        types.Tool(
            name="configure_settings",
            description="Enable or disable specific settings, or reset them to default, or print settings.",
            inputSchema={
                "type": "object",
                "properties": {
                    "setting": {
                        "type": ["string", "null"],
                        "description": "Setting name (e.g. SavePCAP, ForceChannel, etc.). If omitted, lists current settings."
                    },
                    "value": {
                        "type": ["string", "null"],
                        "enum": ["enable", "disable"],
                        "description": "Value to set (enable/disable). Required if setting is provided."
                    }
                }
            }
        ),
        types.Tool(
            name="led_control",
            description="Control the onboard Neopixel LED color or pattern.",
            inputSchema={
                "type": "object",
                "required": ["color"],
                "properties": {
                    "color": {
                        "type": "string",
                        "description": "Hex color code without '#' (e.g. 'FF0000' for red) or 'rainbow' for pattern."
                    }
                }
            }
        ),
        types.Tool(
            name="gps_control",
            description="Query or configure GPS tracking, NMEA data, and Point of Interest tagging.",
            inputSchema={
                "type": "object",
                "required": ["action"],
                "properties": {
                    "action": {
                        "type": "string",
                        "enum": ["status", "tracker_start", "tracker_stop", "tag_poi", "nmea_status"],
                        "description": "GPS action to execute."
                    },
                    "poi_label": {
                        "type": ["string", "null"],
                        "description": "Label for Point of Interest tagging (required for 'tag_poi')."
                    }
                }
            }
        ),
        types.Tool(
            name="ble_spam",
            description="Launch Bluetooth Low Energy (BLE) advertisement spamming attacks to simulate pairing/notifications.",
            inputSchema={
                "type": "object",
                "required": ["spam_type"],
                "properties": {
                    "spam_type": {
                        "type": "string",
                        "enum": ["sourapple", "applejuice", "google", "samsung", "windows", "flipper", "all"],
                        "description": "Target brand/type of BLE advertisement spam."
                    }
                }
            }
        ),
        types.Tool(
            name="spoof_airtag",
            description="Spoof Apple AirTag Bluetooth advertisements by index.",
            inputSchema={
                "type": "object",
                "required": ["index"],
                "properties": {
                    "index": {
                        "type": "integer",
                        "description": "Index of the Airtag to spoof."
                    }
                }
            }
        ),
        types.Tool(
            name="add_device",
            description="Manually add an access point or client/station to the list.",
            inputSchema={
                "type": "object",
                "required": ["device_type", "mac"],
                "properties": {
                    "device_type": {
                        "type": "string",
                        "enum": ["ap", "client"],
                        "description": "Type of device to add."
                    },
                    "mac": {
                        "type": "string",
                        "description": "MAC address of the device (e.g. 'AA:BB:CC:DD:EE:FF')."
                    },
                    "channel": {
                        "type": ["integer", "null"],
                        "description": "Channel number (for APs)."
                    },
                    "ssid": {
                        "type": ["string", "null"],
                        "description": "SSID/ESSID (for APs)."
                    },
                    "ap_index": {
                        "type": ["integer", "null"],
                        "description": "Associated AP index (required for clients)."
                    }
                }
            }
        ),
        types.Tool(
            name="network_scan",
            description="Execute advanced diagnostic network scans (ping, port scan, ARP scan, signal monitor, MAC tracking).",
            inputSchema={
                "type": "object",
                "required": ["scan_mode"],
                "properties": {
                    "scan_mode": {
                        "type": "string",
                        "enum": ["ping", "port", "arp", "mac_track", "signal_monitor"],
                        "description": "The diagnostic scan mode to run."
                    },
                    "options": {
                        "type": ["string", "null"],
                        "description": "Extra arguments for scan (e.g. '-f' for ARP scan, '-a -t <ip_index>' or '-s http' for port scan)."
                    }
                }
            }
        ),
        types.Tool(
            name="mac_spoof",
            description="Configure/randomize/clone MAC addresses on the device.",
            inputSchema={
                "type": "object",
                "required": ["action"],
                "properties": {
                    "action": {
                        "type": "string",
                        "enum": ["random_ap", "random_client", "clone_ap", "clone_client"],
                        "description": "MAC spoofing action."
                    },
                    "index": {
                        "type": ["integer", "null"],
                        "description": "Index of AP or client to clone MAC from (required for 'clone_ap' and 'clone_client')."
                    }
                }
            }
        ),
    ]


@app.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
    global _serial, _capture_buffer, _capture_meta
    loop = asyncio.get_event_loop()

    def text(s: str) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=s)]

    # ------------------------------------------------------------------ #
    if name == "list_ports":
        lines = []
        if _is_termux():
            lines.append("socket://127.0.0.1:7555  —  Android TCP bridge (Termux mode)")
        ports = serial.tools.list_ports.comports()
        lines.extend(f"{p.device}  —  {p.description}" for p in ports)
        if not lines:
            return text("No serial ports found.")
        return text("\n".join(lines))

    # ------------------------------------------------------------------ #
    elif name == "connect":
        port = arguments.get("port")
        baud = int(arguments.get("baud", 115200))

        if port is None:
            if _is_termux():
                port = "socket://127.0.0.1:7555"
            else:
                for p in serial.tools.list_ports.comports():
                    desc = (p.description or "").upper()
                    device = p.device.upper()
                    if any(k in desc for k in ("USB", "UART", "CP210", "CH340", "FTDI")) or "TTYUSB" in device or "TTYACM" in device:
                        port = p.device
                        break
                if port is None:
                    return text("Could not auto-detect an ESP32 serial port. Pass 'port' explicitly.")

        # Try to open the connection with retries and DTR/RTS resetting
        conn_error = None
        with _lock:
            if _serial and _serial.is_open:
                _serial.close()
            
            for attempt in range(3):
                try:
                    _serial = _open_serial(port, baud)
                    if not port.startswith("socket://"):
                        # Reset DTR/RTS to prevent ESP32 boot/reset loops and clear buffers
                        _serial.dtr = False
                        _serial.rts = False
                        time.sleep(0.2)
                        _serial.reset_input_buffer()
                        _serial.reset_output_buffer()
                    conn_error = None
                    break
                except (serial.SerialException, PermissionError, OSError) as exc:
                    conn_error = exc
                    time.sleep(0.5)

        if conn_error:
            if port.startswith("socket://"):
                return text(
                    f"ERROR: Could not connect to TCP bridge at {port}.\n"
                    "Make sure the Marauder Controller app is open, the ESP32 is "
                    "plugged in via OTG, and the app shows 'Connected'."
                )
            
            # Help user with Linux dialout/permission errors
            err_msg = str(conn_error)
            if "Permission denied" in err_msg or (isinstance(conn_error, PermissionError)):
                return text(
                    f"ERROR opening {port}: {conn_error}\n\n"
                    "TIP: This is a permission error. You can fix this by running:\n"
                    f"  sudo chmod a+rw {port}\n"
                    "or by adding your user to the dialout group:\n"
                    "  sudo usermod -a -G dialout $USER\n"
                    "(Note: You may need to log out and log back in for group changes to take effect)."
                )
            return text(f"ERROR opening {port}: {conn_error}")

        # Route all scan/sniff data through USB serial instead of SD card
        await loop.run_in_executor(None, _enable_pcap_capture)

        mode = "TCP bridge (Termux)" if port.startswith("socket://") else "USB serial"
        return text(
            f"Connected to {port} at {baud} baud ({mode}).\n"
            "SavePCAP enabled — all scan output streams through USB serial to this host."
        )

    # ------------------------------------------------------------------ #
    elif name == "disconnect":
        with _lock:
            if _serial and _serial.is_open:
                _serial.close()
        return text("Disconnected.")

    # ------------------------------------------------------------------ #
    elif name == "connection_status":
        with _lock:
            if _serial and _serial.is_open:
                return text(f"Connected: {_serial.port} at {_serial.baudrate} baud.")
        return text("Not connected.")

    # ------------------------------------------------------------------ #
    elif name == "send_command":
        cmd = arguments.get("command", "").strip()
        timeout = float(arguments.get("timeout", DEFAULT_TIMEOUT))
        if not cmd:
            return text("ERROR: 'command' is required.")
        result = await loop.run_in_executor(None, _send, cmd, timeout)
        return text(result or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "read_output":
        duration = float(arguments.get("duration", 2.0))
        raw = await loop.run_in_executor(None, _stream_for, duration)
        return text(raw.strip() or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "scan_and_capture":
        scan_type = arguments.get("scan_type", "scanall")
        duration  = float(arguments.get("duration", 30.0))

        cmd = SCAN_COMMANDS.get(scan_type, scan_type)
        started_at = datetime.datetime.now().isoformat(timespec="seconds")

        # Start scan (wait up to 4 s for the initial ack / prompt)
        start_resp = await loop.run_in_executor(None, _send, cmd, 4.0)

        # Stream all serial output for the capture window
        live_output = await loop.run_in_executor(None, _stream_for, duration)

        # BT scans omit newlines between entries on NO_SCREEN builds; fix that
        if scan_type in ("bt", "airtag"):
            live_output = _fix_bt_output(live_output)

        # Stop scan and pull structured lists
        await loop.run_in_executor(None, _send, "stopscan", 6.0)
        ap_list  = await loop.run_in_executor(None, _send, "list -a", DEFAULT_TIMEOUT)
        st_list  = await loop.run_in_executor(None, _send, "list -c", DEFAULT_TIMEOUT)
        ssid_list = await loop.run_in_executor(None, _send, "list -s", DEFAULT_TIMEOUT)

        # Assemble capture buffer
        _capture_buffer = "\n".join([
            f"=== Marauder capture | type={scan_type} | duration={duration}s | started={started_at} ===",
            "",
            "--- Command response ---",
            start_resp or "(none)",
            "",
            "--- Live serial output (streamed through USB) ---",
            live_output.strip() or "(no live output)",
            "",
            "--- Access points (list -a) ---",
            ap_list or "(none)",
            "",
            "--- Stations / clients (list -c) ---",
            st_list or "(none)",
            "",
            "--- SSID list (list -s) ---",
            ssid_list or "(none)",
        ])

        _capture_meta = {
            "scan_type": scan_type,
            "duration_s": duration,
            "started_at": started_at,
            "command": cmd,
        }

        return text(_capture_buffer)

    # ------------------------------------------------------------------ #
    elif name == "get_capture":
        if not _capture_buffer:
            return text("No capture in buffer. Run scan_and_capture first.")
        return text(_capture_buffer)

    # ------------------------------------------------------------------ #
    elif name == "save_capture_local":
        if not _capture_buffer:
            return text("No capture to save. Run scan_and_capture first.")

        path_arg = arguments.get("path")
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        scan_type = _capture_meta.get("scan_type", "capture")

        if path_arg:
            dest = Path(path_arg).expanduser()
            if dest.is_dir():
                base = dest / f"marauder_{scan_type}_{ts}"
            else:
                base = dest.with_suffix("")
        else:
            base = _capture_dir() / f"marauder_{scan_type}_{ts}"

        txt_path  = base.with_suffix(".txt")
        json_path = base.with_suffix(".json")

        txt_path.write_text(_capture_buffer, encoding="utf-8")
        json_path.write_text(
            json.dumps({"meta": _capture_meta, "raw": _capture_buffer}, indent=2),
            encoding="utf-8",
        )

        # Tell the user whether the file is visible from Android Files app
        android_note = ""
        if _is_termux():
            if "storage/downloads" in str(txt_path):
                android_note = "\n(visible in Android Files → Downloads → marauder_captures)"
            else:
                android_note = "\n(only accessible inside Termux — run termux-setup-storage for Android Files access)"

        return text(
            f"Saved capture to:\n"
            f"  {txt_path}\n"
            f"  {json_path}\n"
            f"Size: {len(_capture_buffer):,} bytes{android_note}"
        )

    # ------------------------------------------------------------------ #
    elif name == "scan_wifi":
        result = await loop.run_in_executor(None, _send, "scanall", 4.0)
        return text(result or "(scan started — use read_output to poll)")

    elif name == "stop_scan":
        result = await loop.run_in_executor(None, _send, "stopscan", 4.0)
        return text(result or "(stopped)")

    elif name == "list_access_points":
        result = await loop.run_in_executor(None, _send, "list -a", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "list_stations":
        result = await loop.run_in_executor(None, _send, "list -c", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "list_ssids":
        result = await loop.run_in_executor(None, _send, "list -s", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "list_probes":
        result = await loop.run_in_executor(None, _send, "list -p", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "get_settings":
        result = await loop.run_in_executor(None, _send, "settings", DEFAULT_TIMEOUT)
        return text(result or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "read_local_file":
        raw_path = arguments.get("path", "")
        if not raw_path:
            return text("ERROR: 'path' is required.")
        p = Path(raw_path).expanduser()
        if not p.exists():
            return text(f"ERROR: File not found: {p}")
        if not p.is_file():
            return text(f"ERROR: Not a file: {p}")
        try:
            return text(p.read_text(encoding="utf-8", errors="replace"))
        except OSError as exc:
            return text(f"ERROR reading {p}: {exc}")

    # ------------------------------------------------------------------ #
    elif name == "flash_firmware":
        bin_path = arguments.get("bin_path")
        port = arguments.get("port")
        baud = int(arguments.get("baud", 921600))

        if port is None:
            if _is_termux():
                return text("ERROR: Flashing via TCP bridge (Termux) is not supported. Please flash directly.")
            for p in serial.tools.list_ports.comports():
                desc = (p.description or "").upper()
                device = p.device.upper()
                if any(k in desc for k in ("USB", "UART", "CP210", "CH340", "FTDI")) or "TTYUSB" in device or "TTYACM" in device:
                    port = p.device
                    break
            if port is None:
                return text("Could not auto-detect serial port for flashing.")

        p = Path(bin_path).expanduser()
        if not p.exists():
            return text(f"ERROR: Firmware file not found: {p}")

        # Temporarily close serial connection if open
        was_open = False
        with _lock:
            if _serial and _serial.is_open:
                _serial.close()
                was_open = True

        import subprocess
        # Run esptool using Python venv interpreter
        venv_python = sys.executable
        cmd = [
            venv_python, "-m", "esptool",
            "--chip", "esp32",
            "--port", port,
            "--baud", str(baud),
            "write_flash", "0x0000", str(p)
        ]
        
        print(f"[marauder-mcp] Flashing firmware using command: {' '.join(cmd)}", file=sys.stderr)
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            if res.returncode == 0:
                output = f"SUCCESS: Firmware flashed successfully.\n\nStdout:\n{res.stdout}"
            else:
                output = f"ERROR: Flashing failed with exit code {res.returncode}.\n\nStderr:\n{res.stderr}\n\nStdout:\n{res.stdout}"
        except Exception as e:
            output = f"ERROR executing esptool: {e}"

        # Reopen serial if it was open
        if was_open:
            with _lock:
                try:
                    # Give ESP32 a moment to boot after flashing before opening port
                    time.sleep(1.0)
                    _serial = _open_serial(port, 115200)
                    _serial.dtr = False
                    _serial.rts = False
                    time.sleep(0.2)
                    _serial.reset_input_buffer()
                    _serial.reset_output_buffer()
                    _enable_pcap_capture()
                except Exception as e:
                    output += f"\n\nWARNING: Could not reopen serial port: {e}"

        return text(output)

    # ------------------------------------------------------------------ #
    elif name == "attack":
        attack_type = arguments.get("attack_type")
        options = arguments.get("options") or ""
        cmd = f"attack -t {attack_type}"
        if options:
            cmd += f" {options.strip()}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Attack '{attack_type}' started.")

    # ------------------------------------------------------------------ #
    elif name == "select_targets":
        target_type = arguments.get("target_type")
        indices = arguments.get("indices")
        flag = {"ap": "-a", "station": "-c", "ssid": "-s"}.get(target_type, "-a")
        cmd = f"select {flag} {indices}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Selected {target_type} indices: {indices}")

    # ------------------------------------------------------------------ #
    elif name == "evil_portal":
        action = arguments.get("action")
        html_file = arguments.get("html_file")
        if action == "start":
            cmd = "evilportal -c start"
            if html_file:
                cmd += f" -w {html_file}"
        elif action == "sethtml":
            if not html_file:
                return text("ERROR: 'html_file' is required when action is 'sethtml'.")
            cmd = f"evilportal -c sethtml {html_file}"
        elif action == "stop":
            cmd = "stopscan"
        else:
            return text(f"ERROR: Unknown action '{action}' for evil_portal.")
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Evil Portal action '{action}' executed.")

    # ------------------------------------------------------------------ #
    elif name == "manage_ssids":
        action = arguments.get("action")
        name_val = arguments.get("name")
        count = arguments.get("count")
        index = arguments.get("index")
        if action == "add":
            if not name_val:
                return text("ERROR: 'name' is required when action is 'add'.")
            cmd = f"ssid -a -n {name_val}"
        elif action == "generate":
            if count is None:
                return text("ERROR: 'count' is required when action is 'generate'.")
            cmd = f"ssid -a -g {count}"
        elif action == "remove":
            if index is None:
                return text("ERROR: 'index' is required when action is 'remove'.")
            cmd = f"ssid -r {index}"
        else:
            return text(f"ERROR: Unknown action '{action}' for manage_ssids.")
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"SSID action '{action}' executed.")

    # ------------------------------------------------------------------ #
    elif name == "change_channel":
        channel = arguments.get("channel")
        if channel is not None:
            cmd = f"channel -s {channel}"
        else:
            cmd = "channel"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "reboot_device":
        res = await loop.run_in_executor(None, _send, "reboot", DEFAULT_TIMEOUT)
        return text(res or "Reboot command sent.")

    # ------------------------------------------------------------------ #
    elif name == "clear_lists":
        list_type = arguments.get("list_type")
        if list_type == "all":
            cmd = "clearlist -a -c -s"
        else:
            flag = {"ap": "-a", "station": "-c", "ssid": "-s"}.get(list_type)
            cmd = f"clearlist {flag}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"List '{list_type}' cleared.")

    # ------------------------------------------------------------------ #
    elif name == "configure_settings":
        setting = arguments.get("setting")
        value = arguments.get("value")
        if setting:
            if not value:
                return text("ERROR: 'value' (enable/disable) is required when specifying a setting.")
            cmd = f"settings -s {setting} {value}"
        else:
            cmd = "settings"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "led_control":
        color = arguments.get("color", "").strip()
        if color.lower() == "rainbow":
            cmd = "led -p rainbow"
        else:
            clean_color = color.lstrip("#")
            cmd = f"led -s #{clean_color}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"LED set to {color}.")

    # ------------------------------------------------------------------ #
    elif name == "gps_control":
        action = arguments.get("action")
        poi_label = arguments.get("poi_label")
        if action == "status":
            cmd = "gpsdata"
        elif action == "tracker_start":
            cmd = "gpstracker -c start"
        elif action == "tracker_stop":
            cmd = "gpstracker -c stop"
        elif action == "tag_poi":
            if not poi_label:
                return text("ERROR: 'poi_label' is required when action is 'tag_poi'.")
            cmd = f"wardrivepoi {poi_label}"
        elif action == "nmea_status":
            cmd = "nmea"
        else:
            return text(f"ERROR: Unknown action '{action}' for gps_control.")
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"GPS action '{action}' executed.")

    # ------------------------------------------------------------------ #
    elif name == "ble_spam":
        spam_type = arguments.get("spam_type")
        cmd = f"blespam -t {spam_type}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"BLE spam '{spam_type}' started.")

    # ------------------------------------------------------------------ #
    elif name == "spoof_airtag":
        index = arguments.get("index")
        cmd = f"spoofat -t {index}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Spoofing AirTag {index} started.")

    # ------------------------------------------------------------------ #
    elif name == "add_device":
        device_type = arguments.get("device_type")
        mac = arguments.get("mac")
        channel = arguments.get("channel")
        ssid = arguments.get("ssid")
        ap_index = arguments.get("ap_index")
        if device_type == "ap":
            cmd = f"add -a -b {mac}"
            if channel is not None:
                cmd += f" -ch {channel}"
            if ssid:
                cmd += f" -e {ssid}"
        elif device_type == "client":
            if ap_index is None:
                return text("ERROR: 'ap_index' is required when adding a client/station.")
            cmd = f"add -c -b {mac} -ap {ap_index}"
        else:
            return text(f"ERROR: Unknown device_type '{device_type}'.")
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Device {mac} added.")

    # ------------------------------------------------------------------ #
    elif name == "network_scan":
        scan_mode = arguments.get("scan_mode")
        options = arguments.get("options") or ""
        mode_cmd = {
            "ping": "pingscan",
            "port": "portscan",
            "arp": "arpscan",
            "mac_track": "mactrack",
            "signal_monitor": "sigmon"
        }.get(scan_mode)
        cmd = mode_cmd
        if options:
            cmd += f" {options.strip()}"
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"Network scan '{scan_mode}' executed.")

    # ------------------------------------------------------------------ #
    elif name == "mac_spoof":
        action = arguments.get("action")
        index = arguments.get("index")
        if action == "random_ap":
            cmd = "randapmac"
        elif action == "random_client":
            cmd = "randstamac"
        elif action == "clone_ap":
            if index is None:
                return text("ERROR: 'index' is required to clone AP MAC.")
            cmd = f"cloneapmac -a {index}"
        elif action == "clone_client":
            if index is None:
                return text("ERROR: 'index' is required to clone client/station MAC.")
            cmd = f"clonestamac -s {index}"
        else:
            return text(f"ERROR: Unknown action '{action}' for mac_spoof.")
        res = await loop.run_in_executor(None, _send, cmd, DEFAULT_TIMEOUT)
        return text(res or f"MAC spoofing action '{action}' executed.")

    # ------------------------------------------------------------------ #
    else:
        return text(f"Unknown tool: {name}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

async def main():
    parser = argparse.ArgumentParser(description="ESP32 Marauder MCP server")
    parser.add_argument("--port", help="Serial port to connect on startup (e.g. /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    args = parser.parse_args()

    startup_port = args.port or ("socket://127.0.0.1:7555" if _is_termux() else None)
    if startup_port:
        global _serial
        try:
            _serial = _open_serial(startup_port, args.baud)
            if not startup_port.startswith("socket://"):
                time.sleep(0.5)
                _serial.reset_input_buffer()
            _enable_pcap_capture()
            mode = "TCP bridge (Termux)" if startup_port.startswith("socket://") else "USB serial"
            print(f"[marauder-mcp] Pre-connected to {startup_port} at {args.baud} baud ({mode}). SavePCAP enabled.", file=sys.stderr)
        except serial.SerialException as exc:
            print(f"[marauder-mcp] WARNING: could not open {startup_port}: {exc}", file=sys.stderr)

    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    asyncio.run(main())
