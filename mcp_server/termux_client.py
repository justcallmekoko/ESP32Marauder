"""
ESP32 Marauder — Termux-native AI client (stdlib only, zero pip deps)

Requires ONLY: pkg install python openssl
Run:
    export VENICE_API_KEY=your_key_here
    python termux_client.py

The Marauder Controller Android app must be open with the ESP32 plugged
in via OTG cable. The app runs a ForegroundService that bridges the USB
serial connection via TCP on port 7555, bound to all interfaces (0.0.0.0).

Termux, the hotspot, and the app all run on the SAME phone, so the bridge
is reachable at the phone's own gateway IP. Termux can't enumerate network
interfaces without root (netlink / /proc/net / /sys/class/net are blocked),
so this client simply TRIES the well-known Android hotspot gateway addresses
(192.168.43.1 first) plus loopback, and uses whichever accepts a connection.
Override with `export MARAUDER_HOST=<ip>` if your hotspot uses another subnet.

No root, no pyserial, no pydantic.
"""

import datetime
import json
import os
import pathlib
import re
import socket
import ssl
import sys
import time
import urllib.error
import urllib.request

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

VENICE_BASE_URL = "https://api.venice.ai/api/v1"
DEFAULT_MODEL   = "gemma-4-uncensored"
BRIDGE_PORT     = int(os.getenv("MARAUDER_PORT", "7555"))
PROMPT          = b"> "                  # Marauder end-of-response marker

SCAN_COMMANDS: dict[str, str] = {
    "scanall":   "scanall",
    "beacon":    "sniffbeacon",
    "probe":     "sniffprobe",
    "deauth":    "sniffdeauth",
    "pmkid":     "sniffpmkid",
    "raw":       "sniffraw",
    "pwn":       "sniffpwn",
    "bt":        "sniffbt -t flock",
    "airtag":    "sniffbt -t airtag",
    "skim":      "sniffskim",
    "sae":       "sniffsae",
    "multissid": "sniffmultissid",
}

# ---------------------------------------------------------------------------
# Global serial state
# ---------------------------------------------------------------------------

_sock: socket.socket | None = None
_connected_host: str = ""
_capture_buffer: str = ""
_capture_meta: dict = {}

# Well-known Android hotspot / tether gateway addresses. Because Termux, the
# hotspot, and the Marauder Controller app all run on the SAME phone, the app's
# ServerSocket bound to 0.0.0.0:7555 is reachable at the phone's own gateway IP
# on whichever local interface is up. We try these in order rather than trying
# to enumerate interfaces — Termux can't read netlink / /proc/net / /sys/class/net
# without root, so `ip addr` and friends silently fail.
HOTSPOT_GATEWAYS = [
    "192.168.43.1",   # AOSP / most Android hotspots
    "192.168.44.1",   # some Samsung / secondary hotspot subnet
    "192.168.137.1",  # Windows-style tether (rare on Android)
    "172.20.10.1",    # iOS-style personal hotspot (rare on Android)
    "127.0.0.1",      # loopback — works now that the app is a ForegroundService
]


def _candidate_hosts() -> list[str]:
    """Ordered list of addresses to try for the bridge, most-likely first."""
    hosts: list[str] = []
    override = os.getenv("MARAUDER_HOST", "")
    if override:
        hosts.append(override)
    for h in HOTSPOT_GATEWAYS:
        if h not in hosts:
            hosts.append(h)
    return hosts

# ---------------------------------------------------------------------------
# Serial helpers
# ---------------------------------------------------------------------------

def _drain() -> None:
    """Discard any pending bytes in the bridge socket."""
    assert _sock is not None
    while True:
        try:
            if not _sock.recv(4096):
                break
        except socket.timeout:
            break


def _read_until_prompt(timeout: float = 8.0) -> str:
    """Accumulate bytes until the Marauder '> ' prompt appears or timeout."""
    assert _sock is not None
    buf = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            chunk = _sock.recv(4096)
            if chunk:
                buf.extend(chunk)
                if buf.endswith(PROMPT):
                    return buf[: -len(PROMPT)].decode("utf-8", errors="replace").strip()
        except socket.timeout:
            pass
    return buf.decode("utf-8", errors="replace").strip()


def _send_cmd(command: str, timeout: float = 8.0) -> str:
    assert _sock is not None
    _sock.sendall((command.strip() + "\n").encode())
    return _read_until_prompt(timeout)


def _stream_for(duration: float) -> str:
    """Read raw serial output for `duration` seconds without waiting for prompt."""
    assert _sock is not None
    buf = bytearray()
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        try:
            buf.extend(_sock.recv(4096))
        except socket.timeout:
            pass
    return buf.decode("utf-8", errors="replace")


def _fix_bt(raw: str) -> str:
    """Fix missing newlines between BT entries on MARAUDER_CYD_3_5_INCH_NO_SCREEN builds."""
    return re.sub(r"(?<!\n)(?=-?\d+ Device:)", "\n", raw).strip()


def _connect_bridge() -> str:
    global _sock, _connected_host
    candidates = _candidate_hosts()
    tried: list[str] = []
    for host in candidates:
        try:
            sock = socket.create_connection((host, BRIDGE_PORT), timeout=3)
        except OSError as exc:
            tried.append(f"{host} ({exc.__class__.__name__})")
            continue
        sock.settimeout(0.1)
        _sock = sock
        _connected_host = host
        time.sleep(0.5)
        _drain()
        _sock.sendall(b"settings -s SavePCAP disable\n")
        _read_until_prompt(4.0)
        return (
            f"Connected to ESP32 via Android bridge at {host}:{BRIDGE_PORT}.\n"
            "SavePCAP disabled — all scan output streams through USB serial."
        )
    _sock = None
    _connected_host = ""
    return (
        f"ERROR: Could not reach the Marauder bridge on port {BRIDGE_PORT}.\n"
        f"Tried: {', '.join(tried)}\n"
        "Checklist:\n"
        "  1. The Marauder Controller app is OPEN and shows the persistent\n"
        "     'Marauder Bridge active' notification.\n"
        "  2. The ESP32 is plugged into the phone via OTG (app shows 'Connected').\n"
        "  3. If your hotspot uses a non-default subnet, set it explicitly:\n"
        "     export MARAUDER_HOST=<the app's Bridge IP>\n"
        "     (the app's connection screen shows 'Bridge: <ip>:7555')."
    )

# ---------------------------------------------------------------------------
# Tool dispatch
# ---------------------------------------------------------------------------

def dispatch(name: str, args: dict) -> str:
    global _sock, _connected_host, _capture_buffer, _capture_meta

    def need_sock() -> str | None:
        return None if _sock else "ERROR: Not connected. Call connect first."

    # ------------------------------------------------------------------ #
    if name == "list_ports":
        cands = ", ".join(f"{h}:{BRIDGE_PORT}" for h in _candidate_hosts())
        return f"Android TCP bridge candidates (Termux mode): {cands}"

    elif name == "connect":
        return _connect_bridge()

    elif name == "disconnect":
        if _sock:
            try:
                _sock.close()
            except OSError:
                pass
            _sock = None
        _connected_host = ""
        return "Disconnected."

    elif name == "connection_status":
        if _sock:
            try:
                _sock.getpeername()
                return f"Connected to Android bridge at {_connected_host}:{BRIDGE_PORT}."
            except OSError:
                pass
        return "Not connected."

    elif name == "send_command":
        if err := need_sock():
            return err
        cmd = args.get("command", "").strip()
        if not cmd:
            return "ERROR: 'command' is required."
        return _send_cmd(cmd, float(args.get("timeout", 8.0))) or "(no output)"

    elif name == "read_output":
        if err := need_sock():
            return err
        return _stream_for(float(args.get("duration", 2.0))).strip() or "(no output)"

    elif name == "scan_and_capture":
        if err := need_sock():
            return err
        scan_type  = args.get("scan_type", "scanall")
        duration   = float(args.get("duration", 30.0))
        cmd        = SCAN_COMMANDS.get(scan_type, scan_type)
        started_at = datetime.datetime.now().isoformat(timespec="seconds")

        start_resp   = _send_cmd(cmd, 4.0)
        live_output  = _stream_for(duration)
        if scan_type in ("bt", "airtag"):
            live_output = _fix_bt(live_output)

        _send_cmd("stopscan", 6.0)
        ap_list   = _send_cmd("list -a")
        st_list   = _send_cmd("list -c")
        ssid_list = _send_cmd("list -s")

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
            "scan_type":  scan_type,
            "duration_s": duration,
            "started_at": started_at,
            "command":    cmd,
        }
        return _capture_buffer

    elif name == "get_capture":
        return _capture_buffer or "No capture in buffer. Run scan_and_capture first."

    elif name == "save_capture_local":
        if not _capture_buffer:
            return "No capture to save. Run scan_and_capture first."
        path_arg  = args.get("path")
        ts        = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        scan_type = _capture_meta.get("scan_type", "capture")
        if path_arg:
            dest = pathlib.Path(path_arg).expanduser()
            base = (dest / f"marauder_{scan_type}_{ts}") if dest.is_dir() else dest.with_suffix("")
        else:
            base_dir = pathlib.Path.home() / "marauder_captures"
            base_dir.mkdir(exist_ok=True)
            base = base_dir / f"marauder_{scan_type}_{ts}"
        txt_path  = base.with_suffix(".txt")
        json_path = base.with_suffix(".json")
        txt_path.write_text(_capture_buffer, encoding="utf-8")
        json_path.write_text(
            json.dumps({"meta": _capture_meta, "raw": _capture_buffer}, indent=2),
            encoding="utf-8",
        )
        return f"Saved:\n  {txt_path}\n  {json_path}\nSize: {len(_capture_buffer):,} bytes"

    elif name == "scan_wifi":
        if err := need_sock(): return err
        return _send_cmd("scanall", 4.0) or "(scan started — use read_output to poll)"

    elif name == "stop_scan":
        if err := need_sock(): return err
        return _send_cmd("stopscan", 4.0) or "(stopped)"

    elif name == "list_access_points":
        if err := need_sock(): return err
        return _send_cmd("list -a") or "(none)"

    elif name == "list_stations":
        if err := need_sock(): return err
        return _send_cmd("list -c") or "(none)"

    elif name == "list_ssids":
        if err := need_sock(): return err
        return _send_cmd("list -s") or "(none)"

    elif name == "list_probes":
        if err := need_sock(): return err
        return _send_cmd("list -p") or "(none)"

    elif name == "get_settings":
        if err := need_sock(): return err
        return _send_cmd("settings") or "(no output)"

    elif name == "attack":
        if err := need_sock(): return err
        attack_type = args.get("attack_type")
        options = args.get("options") or ""
        cmd = f"attack -t {attack_type}"
        if options:
            cmd += f" {options.strip()}"
        return _send_cmd(cmd) or f"Attack '{attack_type}' started."

    elif name == "select_targets":
        if err := need_sock(): return err
        target_type = args.get("target_type")
        indices = args.get("indices")
        flag = {"ap": "-a", "station": "-c", "ssid": "-s"}.get(target_type, "-a")
        cmd = f"select {flag} {indices}"
        return _send_cmd(cmd) or f"Selected {target_type} indices: {indices}"

    elif name == "evil_portal":
        if err := need_sock(): return err
        action = args.get("action")
        html_file = args.get("html_file")
        if action == "start":
            cmd = "evilportal -c start"
            if html_file:
                cmd += f" -w {html_file}"
        elif action == "sethtml":
            if not html_file:
                return "ERROR: 'html_file' is required when action is 'sethtml'."
            cmd = f"evilportal -c sethtml {html_file}"
        elif action == "stop":
            cmd = "stopscan"
        else:
            return f"ERROR: Unknown action '{action}' for evil_portal."
        return _send_cmd(cmd) or f"Evil Portal action '{action}' executed."

    elif name == "manage_ssids":
        if err := need_sock(): return err
        action = args.get("action")
        name_val = args.get("name")
        count = args.get("count")
        index = args.get("index")
        if action == "add":
            if not name_val:
                return "ERROR: 'name' is required when action is 'add'."
            cmd = f"ssid -a -n {name_val}"
        elif action == "generate":
            if count is None:
                return "ERROR: 'count' is required when action is 'generate'."
            cmd = f"ssid -a -g {count}"
        elif action == "remove":
            if index is None:
                return "ERROR: 'index' is required when action is 'remove'."
            cmd = f"ssid -r {index}"
        else:
            return f"ERROR: Unknown action '{action}' for manage_ssids."
        return _send_cmd(cmd) or f"SSID action '{action}' executed."

    elif name == "change_channel":
        if err := need_sock(): return err
        channel = args.get("channel")
        if channel is not None:
            cmd = f"channel -s {channel}"
        else:
            cmd = "channel"
        return _send_cmd(cmd) or "(no output)"

    elif name == "reboot_device":
        if err := need_sock(): return err
        return _send_cmd("reboot") or "Reboot command sent."

    elif name == "clear_lists":
        if err := need_sock(): return err
        list_type = args.get("list_type")
        if list_type == "all":
            cmd = "clearlist -a -c -s"
        else:
            flag = {"ap": "-a", "station": "-c", "ssid": "-s"}.get(list_type)
            cmd = f"clearlist {flag}"
        return _send_cmd(cmd) or f"List '{list_type}' cleared."

    elif name == "configure_settings":
        if err := need_sock(): return err
        setting = args.get("setting")
        value = args.get("value")
        if setting:
            if not value:
                return "ERROR: 'value' (enable/disable) is required when specifying a setting."
            cmd = f"settings -s {setting} {value}"
        else:
            cmd = "settings"
        return _send_cmd(cmd) or "(no output)"

    elif name == "led_control":
        if err := need_sock(): return err
        color = args.get("color", "").strip()
        if color.lower() == "rainbow":
            cmd = "led -p rainbow"
        else:
            clean_color = color.lstrip("#")
            cmd = f"led -s #{clean_color}"
        return _send_cmd(cmd) or f"LED set to {color}."

    elif name == "gps_control":
        if err := need_sock(): return err
        action = args.get("action")
        poi_label = args.get("poi_label")
        if action == "status":
            cmd = "gpsdata"
        elif action == "tracker_start":
            cmd = "gpstracker -c start"
        elif action == "tracker_stop":
            cmd = "gpstracker -c stop"
        elif action == "tag_poi":
            if not poi_label:
                return "ERROR: 'poi_label' is required when action is 'tag_poi'."
            cmd = f"wardrivepoi {poi_label}"
        elif action == "nmea_status":
            cmd = "nmea"
        else:
            return f"ERROR: Unknown action '{action}' for gps_control."
        return _send_cmd(cmd) or f"GPS action '{action}' executed."

    elif name == "ble_spam":
        if err := need_sock(): return err
        spam_type = args.get("spam_type")
        cmd = f"blespam -t {spam_type}"
        return _send_cmd(cmd) or f"BLE spam '{spam_type}' started."

    elif name == "spoof_airtag":
        if err := need_sock(): return err
        index = args.get("index")
        cmd = f"spoofat -t {index}"
        return _send_cmd(cmd) or f"Spoofing AirTag {index} started."

    elif name == "add_device":
        if err := need_sock(): return err
        device_type = args.get("device_type")
        mac = args.get("mac")
        channel = args.get("channel")
        ssid = args.get("ssid")
        ap_index = args.get("ap_index")
        if device_type == "ap":
            cmd = f"add -a -b {mac}"
            if channel is not None:
                cmd += f" -ch {channel}"
            if ssid:
                cmd += f" -e {ssid}"
        elif device_type == "client":
            if ap_index is None:
                return "ERROR: 'ap_index' is required when adding a client/station."
            cmd = f"add -c -b {mac} -ap {ap_index}"
        else:
            return f"ERROR: Unknown device_type '{device_type}'."
        return _send_cmd(cmd) or f"Device {mac} added."

    elif name == "network_scan":
        if err := need_sock(): return err
        scan_mode = args.get("scan_mode")
        options = args.get("options") or ""
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
        return _send_cmd(cmd) or f"Network scan '{scan_mode}' executed."

    elif name == "mac_spoof":
        if err := need_sock(): return err
        action = args.get("action")
        index = args.get("index")
        if action == "random_ap":
            cmd = "randapmac"
        elif action == "random_client":
            cmd = "randstamac"
        elif action == "clone_ap":
            if index is None:
                return "ERROR: 'index' is required to clone AP MAC."
            cmd = f"cloneapmac -a {index}"
        elif action == "clone_client":
            if index is None:
                return "ERROR: 'index' is required to clone client/station MAC."
            cmd = f"clonestamac -s {index}"
        else:
            return f"ERROR: Unknown action '{action}' for mac_spoof."
        return _send_cmd(cmd) or f"MAC spoofing action '{action}' executed."

    else:
        return f"Unknown tool: {name}"

# ---------------------------------------------------------------------------
# Venice AI HTTPS (stdlib urllib)
# ---------------------------------------------------------------------------

TOOLS = [
    {"type": "function", "function": {
        "name": "list_ports",
        "description": "List available connections (shows the Android TCP bridge in Termux).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "connect",
        "description": (
            "Connect to the ESP32 Marauder via the Android app TCP bridge. "
            "Automatically disables SD-card PCAP writing so all scan output streams through USB serial."
        ),
        "parameters": {"type": "object", "properties": {
            "port": {"type": "string", "description": "Omit to use the Android bridge automatically."},
        }},
    }},
    {"type": "function", "function": {
        "name": "disconnect",
        "description": "Disconnect from the ESP32.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "connection_status",
        "description": "Check whether the bridge connection is open.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "send_command",
        "description": "Send any Marauder serial command and return the response. Examples: 'help', 'scanall', 'stopscan', 'attack -t deauth', 'list -a', 'settings', 'channel -s 6'.",
        "parameters": {"type": "object", "required": ["command"], "properties": {
            "command": {"type": "string", "description": "The Marauder command string to execute."},
            "timeout": {"type": "number", "description": "Seconds to wait for prompt (default 8)."},
        }},
    }},
    {"type": "function", "function": {
        "name": "read_output",
        "description": "Read pending bytes from the serial buffer without sending a command. Useful for polling ongoing scan output.",
        "parameters": {"type": "object", "properties": {
            "duration": {"type": "number", "description": "How many seconds to collect (default 2)."},
        }},
    }},
    {"type": "function", "function": {
        "name": "scan_and_capture",
        "description": (
            "Run a scan or sniff and capture ALL output through USB serial — nothing written to SD card. "
            "Stops scan after duration, retrieves AP/station/SSID lists. Results stored in capture buffer. "
            "scan_type options: scanall, beacon, probe, deauth, pmkid, raw, pwn, bt, airtag, skim, sae, multissid. "
            "Output formats: scanall='<rssi> Ch:<ch> <bssid> ESSID:<ssid>'; deauth='<rssi> Ch:<ch> <src>-><dst>'; bt='<rssi> Device:<name>'."
        ),
        "parameters": {"type": "object", "properties": {
            "scan_type": {"type": "string", "description": "Which scan/sniff to run (default: scanall)."},
            "duration":  {"type": "number", "description": "How many seconds to capture (default: 30)."},
        }},
    }},
    {"type": "function", "function": {
        "name": "get_capture",
        "description": "Return the raw capture buffer from the most recent scan_and_capture call for analysis.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "save_capture_local",
        "description": "Save the capture buffer to ~/marauder_captures/ as JSON + TXT. Pass path to override directory.",
        "parameters": {"type": "object", "properties": {
            "path": {"type": "string", "description": "Directory or full file path (optional)."},
        }},
    }},
    {"type": "function", "function": {
        "name": "scan_wifi",
        "description": "Start a WiFi scan (scanall). Use read_output to poll live results.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "stop_scan",
        "description": "Stop any active scan (stopscan).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "list_access_points",
        "description": "List discovered access points (list -a).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "list_stations",
        "description": "List discovered stations/clients (list -c).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "list_ssids",
        "description": "List the SSID list (list -s).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "list_probes",
        "description": "List captured probe SSIDs (list -p).",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "get_settings",
        "description": "Print current Marauder settings.",
        "parameters": {"type": "object", "properties": {}},
    }},
    {"type": "function", "function": {
        "name": "attack",
        "description": "Launch a specified Wi-Fi attack against selected targets.",
        "parameters": {
            "type": "object",
            "required": ["attack_type"],
            "properties": {
                "attack_type": {
                    "type": "string",
                    "enum": ["deauth", "beacon", "probe", "funny", "rickroll", "badmsg", "sleep", "sae", "csa", "quiet"],
                    "description": "Type of attack to launch."
                },
                "options": {
                    "type": "string",
                    "description": "Optional flags/parameters (e.g. '-c' to target clients, '-s <mac>' override source MAC, '-d <mac>' override destination MAC)."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "select_targets",
        "description": "Select access points, stations/clients, or SSIDs by indices (comma-separated list of numbers, or 'all'). Selection toggles the select status on the device.",
        "parameters": {
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
    }},
    {"type": "function", "function": {
        "name": "evil_portal",
        "description": "Manage the Evil Portal phishing module. Starts/stops the portal or loads HTML.",
        "parameters": {
            "type": "object",
            "required": ["action"],
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["start", "stop", "sethtml"],
                    "description": "Action to perform."
                },
                "html_file": {
                    "type": "string",
                    "description": "Name of the HTML portal file to load (required for 'start' with custom file and 'sethtml')."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "manage_ssids",
        "description": "Add, remove, or generate random SSIDs in the target SSID list.",
        "parameters": {
            "type": "object",
            "required": ["action"],
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["add", "remove", "generate"],
                    "description": "Action to perform."
                },
                "name": {
                    "type": "string",
                    "description": "SSID name/ESSID to add (required for 'add')."
                },
                "count": {
                    "type": "integer",
                    "description": "Number of random SSIDs to generate (required for 'generate')."
                },
                "index": {
                    "type": "integer",
                    "description": "Index of SSID to remove (required for 'remove')."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "change_channel",
        "description": "Set or query the Wi-Fi channel.",
        "parameters": {
            "type": "object",
            "properties": {
                "channel": {
                    "type": "integer",
                    "description": "Channel number (1-14). If omitted, just queries current channel."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "reboot_device",
        "description": "Reboot the ESP32 Marauder hardware.",
        "parameters": {"type": "object", "properties": {}}
    }},
    {"type": "function", "function": {
        "name": "clear_lists",
        "description": "Clear the discovered access points, stations, or SSIDs lists.",
        "parameters": {
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
    }},
    {"type": "function", "function": {
        "name": "configure_settings",
        "description": "Enable or disable specific settings, or reset them to default, or print settings.",
        "parameters": {
            "type": "object",
            "properties": {
                "setting": {
                    "type": "string",
                    "description": "Setting name (e.g. SavePCAP, ForceChannel, etc.). If omitted, lists current settings."
                },
                "value": {
                    "type": "string",
                    "enum": ["enable", "disable"],
                    "description": "Value to set (enable/disable). Required if setting is provided."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "led_control",
        "description": "Control the onboard Neopixel LED color or pattern.",
        "parameters": {
            "type": "object",
            "required": ["color"],
            "properties": {
                "color": {
                    "type": "string",
                    "description": "Hex color code without '#' (e.g. 'FF0000' for red) or 'rainbow' for pattern."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "gps_control",
        "description": "Query or configure GPS tracking, NMEA data, and Point of Interest tagging.",
        "parameters": {
            "type": "object",
            "required": ["action"],
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["status", "tracker_start", "tracker_stop", "tag_poi", "nmea_status"],
                    "description": "GPS action to execute."
                },
                "poi_label": {
                    "type": "string",
                    "description": "Label for Point of Interest tagging (required for 'tag_poi')."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "ble_spam",
        "description": "Launch Bluetooth Low Energy (BLE) advertisement spamming attacks to simulate pairing/notifications.",
        "parameters": {
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
    }},
    {"type": "function", "function": {
        "name": "spoof_airtag",
        "description": "Spoof Apple AirTag Bluetooth advertisements by index.",
        "parameters": {
            "type": "object",
            "required": ["index"],
            "properties": {
                "index": {
                    "type": "integer",
                    "description": "Index of the Airtag to spoof."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "add_device",
        "description": "Manually add an access point or client/station to the list.",
        "parameters": {
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
                    "type": "integer",
                    "description": "Channel number (for APs)."
                },
                "ssid": {
                    "type": "string",
                    "description": "SSID/ESSID (for APs)."
                },
                "ap_index": {
                    "type": "integer",
                    "description": "Associated AP index (required for clients)."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "network_scan",
        "description": "Execute advanced diagnostic network scans (ping, port scan, ARP scan, signal monitor, MAC tracking).",
        "parameters": {
            "type": "object",
            "required": ["scan_mode"],
            "properties": {
                "scan_mode": {
                    "type": "string",
                    "enum": ["ping", "port", "arp", "mac_track", "signal_monitor"],
                    "description": "The diagnostic scan mode to run."
                },
                "options": {
                    "type": "string",
                    "description": "Extra arguments for scan (e.g. '-f' for ARP scan, '-a -t <ip_index>' or '-s http' for port scan)."
                }
            }
        }
    }},
    {"type": "function", "function": {
        "name": "mac_spoof",
        "description": "Configure/randomize/clone MAC addresses on the device.",
        "parameters": {
            "type": "object",
            "required": ["action"],
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["random_ap", "random_client", "clone_ap", "clone_client"],
                    "description": "MAC spoofing action."
                },
                "index": {
                    "type": "integer",
                    "description": "Index of AP or client to clone MAC from (required for 'clone_ap' and 'clone_client')."
                }
            }
        }
    }},
]

SYSTEM_PROMPT = """\
You are a hands-on ESP32 Marauder RF-security analyst with direct USB serial
access to the hardware via MCP tools.

## MANDATORY RULES — no exceptions
- ALWAYS call connect() as your very first tool call on every request, no matter what.
  Do not ask the user to connect. Do not ask them to verify cables or the app.
  Just call connect(). If it fails, report the error text and stop.
- NEVER describe what you are about to do — call the tool and report real results.
- NEVER ask the user to verify hardware state — the tools do that for you.

## Workflow (follow in order, no skipping)
1. Call connect() — always, unconditionally, before anything else.
2. To gather wireless data, use scan_and_capture(scan_type, duration) to perform a scan/sniff run.
3. To select specific targets (access points, stations, or SSIDs) for targeting or analysis, use select_targets(target_type, indices).
4. To launch attacks (deauth, beacon spam, probe spam, funny, rickroll, badmsg, sleep, sae, csa, quiet), use attack(attack_type, options).
5. Use dedicated tools for other hardware features:
   - Configure channel: change_channel(channel)
   - Control LED: led_control(color)
   - Manage SSIDs: manage_ssids(action, name, count, index)
   - Run BLE spamming: ble_spam(spam_type)
   - Spoof AirTags: spoof_airtag(index)
   - Manage GPS / Wardriving POIs: gps_control(action, poi_label)
   - Run network/diagnostic scans (ping, port scan, ARP scan, signal monitor, MAC tracking): network_scan(scan_mode, options)
   - Configure MAC addresses: mac_spoof(action, index)
   - Manage Evil Portal: evil_portal(action, html_file)
   - Clear discovered targets list: clear_lists(list_type)
   - Custom settings configuration: configure_settings(setting, value)
   - Manually add device: add_device(device_type, mac, channel, ssid, ap_index)
   - Reboot device: reboot_device()
6. Analyze the returned text and report specific findings with real numbers.
7. Optionally call save_capture_local() to write .txt + .json to ~/marauder_captures/.

## Serial output line formats
- scanall AP:      `<rssi> Ch: <ch> <bssid> ESSID: <ssid> <cap_hex...>`
  - Hidden AP: ESSID field equals the BSSID string instead of a name
  - Each AP fires once only (duplicates suppressed)
- scanall station: `<n>: ap: <bssid> -> sta: <mac>`
- beacon sniff:    same AP format but fires for EVERY beacon (~10/s per AP)
- deauth sniff:    `<rssi> Ch: <ch> <src_mac> -> <dst_mac>`
- pmkid sniff:     `Received EAPOL: <bssid>` + periodic stats block every ~1 s
- raw sniff:       NO per-packet text; only periodic stats every ~1 s
- bt/airtag sniff: `<rssi> Device: <name_or_mac>` (one per advertisement)

## Analysis guidance
- Strongest APs: sort by RSSI (least negative = strongest signal).
- Deauth floods: many lines from same src_mac to broadcast (ff:ff:ff:ff:ff:ff) = deauth attack.
- Hidden SSIDs: ESSID field equals BSSID string = AP hiding its name.
- Rogue APs: same SSID on multiple BSSIDs, or BSSID on unexpected channel.
- PMKID: count "Received EAPOL" lines; "Complete EAPOL: N" = full 4-way handshake captured.
- BT/AirTag: group by RSSI range for proximity; repeated MAC = persistent tracker.
"""


def _venice_post(messages: list, api_key: str, model: str) -> dict:
    body = json.dumps({
        "model":       model,
        "messages":    messages,
        "tools":       TOOLS,
        "tool_choice": "auto",
    }).encode()
    req = urllib.request.Request(
        f"{VENICE_BASE_URL}/chat/completions",
        data=body,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type":  "application/json",
        },
    )
    ctx = ssl.create_default_context()
    with urllib.request.urlopen(req, context=ctx, timeout=120) as resp:
        return json.loads(resp.read())


# ---------------------------------------------------------------------------
# Agent loop
# ---------------------------------------------------------------------------

def run_agent(user_input: str, api_key: str, model: str) -> str:
    messages: list[dict] = [
        {"role": "system", "content": SYSTEM_PROMPT},
        {"role": "user",   "content": user_input},
    ]
    for _ in range(15):
        try:
            resp = _venice_post(messages, api_key, model)
        except urllib.error.HTTPError as exc:
            return f"Venice AI HTTP error {exc.code}: {exc.read().decode()}"
        except Exception as exc:
            return f"Venice AI error: {exc}"

        choice = resp["choices"][0]
        msg    = choice["message"]
        messages.append(msg)

        tool_calls = msg.get("tool_calls") or []
        if not tool_calls:
            return msg.get("content") or ""

        for tc in tool_calls:
            fn_name = tc["function"]["name"]
            try:
                fn_args = json.loads(tc["function"].get("arguments") or "{}")
            except json.JSONDecodeError:
                fn_args = {}
            result = dispatch(fn_name, fn_args)
            messages.append({
                "role":         "tool",
                "tool_call_id": tc["id"],
                "content":      result,
            })

    return "(max steps reached)"


# ---------------------------------------------------------------------------
# REPL
# ---------------------------------------------------------------------------

def main() -> None:
    api_key = os.environ.get("VENICE_API_KEY")
    if not api_key:
        sys.exit("ERROR: VENICE_API_KEY environment variable is not set.")

    model = os.environ.get("VENICE_MODEL", DEFAULT_MODEL)

    override = os.getenv("MARAUDER_HOST", "")
    bridge_info = f"{override}:{BRIDGE_PORT}" if override else f"auto ({', '.join(HOTSPOT_GATEWAYS[:3])}…):{BRIDGE_PORT}"
    print("ESP32 Marauder AI Terminal (Termux — stdlib only)")
    print(f"Model : {model} via Venice AI")
    print(f"Bridge: {bridge_info}  (Marauder Controller app)")
    print("Type 'quit' to exit.\n")

    while True:
        try:
            line = input("\033[92m>\033[0m ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nBye.")
            break

        if not line:
            continue
        if line.lower() in ("quit", "exit", "q"):
            break

        try:
            result = run_agent(line, api_key, model)
            print(f"\n{result}")
        except KeyboardInterrupt:
            print("\n[cancelled]")
        except Exception as exc:
            print(f"\n[error] {exc}")

    if _sock:
        try:
            _sock.close()
        except OSError:
            pass


if __name__ == "__main__":
    main()
