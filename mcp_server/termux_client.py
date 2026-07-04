"""
ESP32 Marauder — Termux-native AI client (stdlib only, zero pip deps)

Requires ONLY: pkg install python openssl
Run:
    export VENICE_API_KEY=your_key_here
    python termux_client.py

The Marauder Controller Android app must be open with the ESP32 plugged
in via OTG cable. The app bridges the USB serial connection to localhost:5555
automatically — no root, no pyserial, no pydantic.
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
BRIDGE          = ("127.0.0.1", 5555)   # Android app TCP bridge
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
_capture_buffer: str = ""
_capture_meta: dict = {}

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
    global _sock
    try:
        _sock = socket.create_connection(BRIDGE, timeout=10)
        _sock.settimeout(0.1)
    except OSError as exc:
        _sock = None
        return (
            f"ERROR: Cannot connect to Android bridge at {BRIDGE[0]}:{BRIDGE[1]} — {exc}\n"
            "Make sure the Marauder Controller app is open and the ESP32 is plugged in."
        )
    time.sleep(0.5)
    _drain()
    _sock.sendall(b"settings -s SavePCAP disable\n")
    _read_until_prompt(4.0)
    return (
        f"Connected to ESP32 via Android bridge on {BRIDGE[0]}:{BRIDGE[1]}.\n"
        "SavePCAP disabled — all scan output streams through USB serial."
    )

# ---------------------------------------------------------------------------
# Tool dispatch
# ---------------------------------------------------------------------------

def dispatch(name: str, args: dict) -> str:
    global _sock, _capture_buffer, _capture_meta

    def need_sock() -> str | None:
        return None if _sock else "ERROR: Not connected. Call connect first."

    # ------------------------------------------------------------------ #
    if name == "list_ports":
        return f"socket://127.0.0.1:5555  —  Android TCP bridge (Termux mode)"

    elif name == "connect":
        return _connect_bridge()

    elif name == "disconnect":
        if _sock:
            try:
                _sock.close()
            except OSError:
                pass
            _sock = None
        return "Disconnected."

    elif name == "connection_status":
        if _sock:
            try:
                _sock.getpeername()
                return f"Connected to Android bridge at {BRIDGE[0]}:{BRIDGE[1]}."
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
]

SYSTEM_PROMPT = """\
You are a hands-on ESP32 Marauder RF-security analyst with direct USB serial
access to the hardware via MCP tools.

## Workflow
1. If not connected, call connect() first. It auto-disables SD-card capture so
   ALL scan/sniff data streams back to this host through USB serial.
2. To gather data, use scan_and_capture(scan_type, duration) — it starts the
   scan, collects live serial output for `duration` seconds, stops the scan,
   then pulls the AP/station/SSID lists. Everything is buffered locally.
3. The captured data is returned directly and also stored in the capture buffer.
   Call get_capture() at any time to re-read the last capture without re-scanning.
4. To persist findings, call save_capture_local() — it writes both a .txt
   (human-readable) and a .json (structured) file to ~/marauder_captures/.

## Serial output line formats (for parsing capture data)
- scanall AP discovery:    `<rssi> Ch: <ch> <bssid> ESSID: <ssid> <cap_hex...>`
  - Hidden AP (empty SSID): ESSID field contains the BSSID again
  - Only fires ONCE per AP (first discovery); duplicates are suppressed
- scanall station:         `<n>: ap: <bssid> -> sta: <mac>`  (or sta -> ap)
- beacon sniff:            same format as scanall AP but fires for EVERY beacon (~10/s per AP)
- deauth sniff:            `<rssi> Ch: <ch> <src_mac> -> <dst_mac>`
- pmkid sniff:             `Received EAPOL: <bssid>` + periodic stats block every ~1 s
- raw sniff:               NO per-packet text; only periodic stats every ~1 s
- bt/airtag sniff:         `<rssi> Device: <name_or_mac>` (one per advertisement)
- Periodic stats block (raw/pmkid): multi-line with Mgmt/Data/Channel/Beacon/Probe/RSSI counts

## Analysis guidance
- Parse beacon captures: count unique BSSIDs, spot channels, note open vs encrypted.
- Deauth floods: many lines from the same src_mac to ff:ff:ff:ff:ff:ff = broadcast deauth attack.
- Hidden SSIDs: ESSID field equals BSSID string = AP is hiding its name.
- Rogue AP signals: same SSID on multiple BSSIDs, or BSSID on an unexpected channel.
- PMKID/EAPOL: count "Received EAPOL" lines; "Complete EAPOL: N" in stats = full 4-way handshake.
- BT/AirTag: group by RSSI range to estimate proximity; repeated MAC = persistent tracker.

When the user asks you to do something, use the tools to actually do it — do not
just describe how. Report real numbers and specific findings from the capture
data, not generic advice.
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

    print("ESP32 Marauder AI Terminal (Termux — stdlib only)")
    print(f"Model : {model} via Venice AI")
    print(f"Bridge: {BRIDGE[0]}:{BRIDGE[1]}  (Marauder Controller app)")
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
