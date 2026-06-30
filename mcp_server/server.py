"""
ESP32 Marauder MCP Server

Exposes the Marauder serial command interface as MCP tools so any MCP client
(Claude Desktop, Claude Code, etc.) can drive the hardware directly.

Run:
    pip install mcp pyserial
    python server.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import asyncio
import sys
import time
import threading
import argparse
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


def _read_until_prompt(ser: serial.Serial, timeout: float = DEFAULT_TIMEOUT) -> str:
    """Read bytes from ser until the Marauder '> ' prompt appears or timeout."""
    buf = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf.extend(chunk)
            if buf.endswith(PROMPT):
                # Strip the trailing prompt so callers get clean output
                return buf[: -len(PROMPT)].decode("utf-8", errors="replace").strip()
        else:
            time.sleep(0.02)
    # Timeout — return whatever accumulated
    return buf.decode("utf-8", errors="replace").strip()


def _send(command: str, timeout: float = DEFAULT_TIMEOUT) -> str:
    global _serial
    with _lock:
        if _serial is None or not _serial.is_open:
            return "ERROR: Not connected. Use connect() first."
        _serial.reset_input_buffer()
        _serial.write((command.strip() + "\n").encode())
        _serial.flush()
        return _read_until_prompt(_serial, timeout)


# ---------------------------------------------------------------------------
# MCP server setup
# ---------------------------------------------------------------------------

app = Server("esp32-marauder")


@app.list_tools()
async def list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="list_ports",
            description="List serial ports available on the host. Useful for finding which port the ESP32 is on.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="connect",
            description="Open a serial connection to the ESP32 Marauder.",
            inputSchema={
                "type": "object",
                "properties": {
                    "port": {
                        "type": "string",
                        "description": "Serial port, e.g. /dev/ttyUSB0 or COM3. Auto-detected if omitted.",
                    },
                    "baud": {
                        "type": "integer",
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
                    "command": {
                        "type": "string",
                        "description": "The Marauder command string to execute.",
                    },
                    "timeout": {
                        "type": "number",
                        "description": "Seconds to wait for the '> ' prompt (default 8).",
                        "default": 8.0,
                    },
                },
            },
        ),
        types.Tool(
            name="read_output",
            description=(
                "Read any pending bytes from the serial buffer without sending a command. "
                "Useful for polling ongoing scan output."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "duration": {
                        "type": "number",
                        "description": "How many seconds to collect output (default 2).",
                        "default": 2.0,
                    }
                },
            },
        ),
        # ---- convenience wrappers for common operations ----
        types.Tool(
            name="scan_wifi",
            description="Start a full WiFi scan (scanall). Returns the live scan header; use read_output to poll results.",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="stop_scan",
            description="Stop any active scan (stopscan).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_access_points",
            description="List discovered access points (-a flag).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="list_stations",
            description="List discovered stations / clients (-c flag).",
            inputSchema={"type": "object", "properties": {}},
        ),
        types.Tool(
            name="get_settings",
            description="Print current Marauder settings.",
            inputSchema={"type": "object", "properties": {}},
        ),
    ]


@app.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
    global _serial

    def text(s: str) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=s)]

    # ------------------------------------------------------------------ #
    if name == "list_ports":
        ports = serial.tools.list_ports.comports()
        if not ports:
            return text("No serial ports found.")
        lines = [f"{p.device}  —  {p.description}" for p in ports]
        return text("\n".join(lines))

    # ------------------------------------------------------------------ #
    elif name == "connect":
        port = arguments.get("port")
        baud = int(arguments.get("baud", 115200))

        if port is None:
            # Auto-detect: pick first USB/UART/CP210x/CH340 port
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
                _serial = serial.Serial(port, baud, timeout=0.1)
            except serial.SerialException as exc:
                return text(f"ERROR opening {port}: {exc}")

        # Drain any boot banner
        time.sleep(0.5)
        with _lock:
            _serial.reset_input_buffer()

        return text(f"Connected to {port} at {baud} baud.")

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
        result = await asyncio.get_event_loop().run_in_executor(None, _send, cmd, timeout)
        return text(result or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "read_output":
        duration = float(arguments.get("duration", 2.0))
        with _lock:
            if _serial is None or not _serial.is_open:
                return text("ERROR: Not connected.")
            buf = bytearray()
            deadline = time.monotonic() + duration
            while time.monotonic() < deadline:
                chunk = _serial.read(_serial.in_waiting or 1)
                if chunk:
                    buf.extend(chunk)
                else:
                    time.sleep(0.05)
        return text(buf.decode("utf-8", errors="replace").strip() or "(no output)")

    # ------------------------------------------------------------------ #
    elif name == "scan_wifi":
        result = await asyncio.get_event_loop().run_in_executor(None, _send, "scanall", 4.0)
        return text(result or "(scan started — use read_output to poll)")

    elif name == "stop_scan":
        result = await asyncio.get_event_loop().run_in_executor(None, _send, "stopscan", 4.0)
        return text(result or "(stopped)")

    elif name == "list_access_points":
        result = await asyncio.get_event_loop().run_in_executor(None, _send, "list -a", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "list_stations":
        result = await asyncio.get_event_loop().run_in_executor(None, _send, "list -c", DEFAULT_TIMEOUT)
        return text(result or "(none)")

    elif name == "get_settings":
        result = await asyncio.get_event_loop().run_in_executor(None, _send, "settings", DEFAULT_TIMEOUT)
        return text(result or "(no output)")

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

    if args.port:
        # Pre-connect if a port was supplied on the command line
        global _serial
        try:
            _serial = serial.Serial(args.port, args.baud, timeout=0.1)
            time.sleep(0.5)
            _serial.reset_input_buffer()
            print(f"[marauder-mcp] Pre-connected to {args.port} at {args.baud} baud.", file=sys.stderr)
        except serial.SerialException as exc:
            print(f"[marauder-mcp] WARNING: could not open {args.port}: {exc}", file=sys.stderr)

    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    asyncio.run(main())
