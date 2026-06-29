# ESP32 Marauder MCP Server

Exposes the Marauder serial interface as MCP tools so Claude (or any MCP client) can drive your ESP32 directly — scan WiFi, list APs, send attack commands, read output, change settings, all through conversation.

## Install

```bash
cd mcp_server
pip install mcp pyserial
```

## Run standalone (test)

```bash
# Let the server auto-detect the port:
python server.py

# Or pin a specific port on startup:
python server.py --port /dev/ttyUSB0 --baud 115200
```

## Wire into Claude Desktop

Add to `~/Library/Application Support/Claude/claude_desktop_config.json` (macOS)
or `%APPDATA%\Claude\claude_desktop_config.json` (Windows):

```json
{
  "mcpServers": {
    "esp32-marauder": {
      "command": "python",
      "args": [
        "/absolute/path/to/ESP32Marauder-cyd/mcp_server/server.py",
        "--port", "/dev/ttyUSB0"
      ]
    }
  }
}
```

Replace `/dev/ttyUSB0` with your actual port (`COM3` on Windows, `/dev/cu.usbserial-*` on macOS).

## Wire into Claude Code (`.mcp.json`)

Create `.mcp.json` in the project root:

```json
{
  "mcpServers": {
    "esp32-marauder": {
      "command": "python",
      "args": ["mcp_server/server.py", "--port", "/dev/ttyUSB0"]
    }
  }
}
```

## Available tools

| Tool | What it does |
|------|-------------|
| `list_ports` | List all serial ports on the host |
| `connect` | Open serial connection (auto-detects port if omitted) |
| `disconnect` | Close the connection |
| `connection_status` | Check if connected and which port |
| `send_command` | Send any Marauder command and get the response |
| `read_output` | Drain pending bytes from the buffer (poll ongoing scans) |
| `scan_wifi` | Start `scanall` |
| `stop_scan` | Run `stopscan` |
| `list_access_points` | Run `list -a` |
| `list_stations` | Run `list -c` |
| `get_settings` | Run `settings` |

## Example conversation with Claude

> "Connect to my Marauder and run a WiFi scan for 10 seconds, then list all the access points you found."

Claude will call `connect → scan_wifi → read_output (×n) → stop_scan → list_access_points` and present the results.
