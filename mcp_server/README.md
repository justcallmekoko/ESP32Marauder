# ESP32 Marauder — Venice AI MCP Terminal

Two-part setup:

| File | Role |
|------|------|
| `server.py` | MCP server — owns the serial connection, exposes Marauder commands as tools |
| `client.py` | Interactive terminal — `mcp-use` connects Venice AI (Gemma 4) to the server |

```
You (terminal) ──► client.py (mcp-use + Venice AI / Gemma 4)
                        │  tool calls
                        ▼
                   server.py (MCP server)
                        │  serial
                        ▼
                   ESP32 Marauder (/dev/ttyUSB0)
```

## Install

```bash
cd mcp_server
pip install mcp mcp-use pyserial langchain-openai
```

## Configure

```bash
export VENICE_API_KEY="your_key_here"
export MARAUDER_PORT="/dev/ttyUSB0"   # Linux
# export MARAUDER_PORT="COM3"         # Windows
# leave unset for auto-detect
```

Optional overrides:
```bash
export VENICE_MODEL="gemma-4-uncensored"   # default
export MARAUDER_BAUD="115200"              # default
```

## Run

```bash
python client.py
```

The client launches `server.py` as a subprocess automatically — you don't start the server separately.

## Example session

```
ESP32 Marauder AI Terminal
Model : gemma-4-uncensored via Venice AI
Port  : auto-detect

> connect to my marauder and scan wifi for 10 seconds then tell me what you found
[Gemma 4 calls: connect → scan_wifi → read_output × 5 → stop_scan → list_access_points]

Found 12 access points. Here are the highlights:
  • HomeNetwork_5G  (ch 36, WPA2, -62 dBm)
  • NETGEAR42       (ch 6,  WPA2, -71 dBm)
  ...

> deauth attack on AP index 0
...
```

Type `verbose` to see every tool call the model makes. Type `quit` to exit.

## Available MCP tools

| Tool | Marauder command |
|------|-----------------|
| `list_ports` | — |
| `connect` | — |
| `disconnect` | — |
| `connection_status` | — |
| `send_command` | any command |
| `read_output` | — |
| `scan_wifi` | `scanall` |
| `stop_scan` | `stopscan` |
| `list_access_points` | `list -a` |
| `list_stations` | `list -c` |
| `get_settings` | `settings` |
