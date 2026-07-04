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


def _read_until_prompt(ser: serial.Serial, timeout: float = DEFAULT_TIMEOUT) -> str:
    """Read bytes from ser until the Marauder '> ' prompt appears or timeout."""
    buf = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf.extend(chunk)
            if buf.endswith(PROMPT):
                return buf[: -len(PROMPT)].decode("utf-8", errors="replace").strip()
        else:
            time.sleep(0.02)
    return buf.decode("utf-8", errors="replace").strip()


def _send(command: str, timeout: float = DEFAULT_TIMEOUT) -> str:
    global _serial
    with _lock:
        if _serial is None or not _serial.is_open:
            return "ERROR: Not connected. Use connect() first."
        if not _is_socket():
            _serial.reset_input_buffer()
        _serial.write((command.strip() + "\n").encode())
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
    return buf.decode("utf-8", errors="replace")


def _fix_bt_output(raw: str) -> str:
    """Insert newlines before BT entries on NO_SCREEN builds.

    On MARAUDER_CYD_3_5_INCH_NO_SCREEN the Serial.println() that terminates
    each BT advertisement line is inside #ifdef HAS_SCREEN and is omitted,
    so entries concatenate on a single line like:
        -72 Device: MyPhone-80 Device: aa:bb:cc:dd:ee:ff-61 Device: Speaker
    Split on the pattern that starts each entry (negative RSSI followed by ' Device:').
    """
    import re
    return re.sub(r"(?<!\n)(?=-?\d+ Device:)", "\n", raw).strip()


def _disable_sd_capture() -> None:
    """Turn off SD-card PCAP writing so all data routes through USB serial."""
    _send("settings -s SavePCAP disable", timeout=4.0)


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
                    if any(k in desc for k in ("USB", "UART", "CP210", "CH340", "FTDI")):
                        port = p.device
                        break
                if port is None:
                    return text("Could not auto-detect an ESP32 serial port. Pass 'port' explicitly.")

        with _lock:
            if _serial and _serial.is_open:
                _serial.close()
            try:
                _serial = _open_serial(port, baud)
            except serial.SerialException as exc:
                if port.startswith("socket://"):
                    return text(
                        f"ERROR: Could not connect to TCP bridge at {port}.\n"
                        "Make sure the Marauder Controller app is open, the ESP32 is "
                        "plugged in via OTG, and the app shows 'Connected'."
                    )
                return text(f"ERROR opening {port}: {exc}")

        if not port.startswith("socket://"):
            time.sleep(0.5)
            with _lock:
                _serial.reset_input_buffer()

        # Route all scan/sniff data through USB serial instead of SD card
        await loop.run_in_executor(None, _disable_sd_capture)

        mode = "TCP bridge (Termux)" if port.startswith("socket://") else "USB serial"
        return text(
            f"Connected to {port} at {baud} baud ({mode}).\n"
            "SavePCAP disabled — all scan output streams through USB serial to this host."
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
            _disable_sd_capture()
            mode = "TCP bridge (Termux)" if startup_port.startswith("socket://") else "USB serial"
            print(f"[marauder-mcp] Pre-connected to {startup_port} at {args.baud} baud ({mode}). SavePCAP disabled.", file=sys.stderr)
        except serial.SerialException as exc:
            print(f"[marauder-mcp] WARNING: could not open {startup_port}: {exc}", file=sys.stderr)

    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    asyncio.run(main())
