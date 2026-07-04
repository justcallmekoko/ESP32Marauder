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
                        │  serial / TCP socket
                        ▼
                   ESP32 Marauder
```

---

## Linux / Desktop

### Install

```bash
cd mcp_server
pip install mcp mcp-use pyserial langchain-openai
```

### Configure

```bash
export VENICE_API_KEY="your_key_here"
export MARAUDER_PORT="/dev/ttyUSB0"   # or leave unset for auto-detect
```

Optional overrides:
```bash
export VENICE_MODEL="gemma-4-uncensored"   # default
export VENICE_TIMEOUT="120"               # API timeout in seconds
export MARAUDER_BAUD="115200"             # default
```

### Run

```bash
python client.py
```

Captures save to `~/marauder_captures/`.

---

## Android (Termux) — no root required

The USB serial connection is bridged from the Android app to Termux over a local TCP socket — no root, no USB passthrough needed.

### Architecture

```
ESP32 Marauder
    │ USB OTG
    ▼
Marauder Controller app (Android)
    │ TCP socket  localhost:5555
    ▼
server.py  (socket://127.0.0.1:5555)
    │ MCP stdio
    ▼
client.py  (Venice AI agent)
```

### One-time setup

```bash
bash termux_setup.sh
```

This script:
1. Installs system packages (`python`, `openssl`, `clang`)
2. Installs Python dependencies
3. Runs `termux-setup-storage` — tap **Allow** so captures land in your Downloads folder
4. Saves your `VENICE_API_KEY` to `~/.bashrc`

### Hardware requirements

- Android phone with OTG support
- OTG adapter or OTG cable
- [Marauder Controller](https://play.google.com/store/apps/details?id=com.justcallmekoko.maraudercompanion) app (free) — bridges USB serial to `localhost:5555`

### Run

1. Open Marauder Controller, plug in the ESP32 via OTG, tap **Connect**
2. In Termux:

```bash
python client.py
```

### Captures on Android

| `termux-setup-storage` run? | Save path | Visible in Android Files? |
|-----------------------------|-----------|--------------------------|
| Yes (recommended) | `~/storage/downloads/marauder_captures/` | ✅ Downloads → marauder_captures |
| No | `~/marauder_captures/` | ❌ Termux internal only |

---

## Example session

```
ESP32 Marauder AI Terminal
Model   : gemma-4-uncensored via Venice AI
Port    : socket://127.0.0.1:5555
Saves   : /data/data/com.termux/files/home/storage/downloads/marauder_captures
Mode    : Termux/Android (TCP bridge on localhost:5555)

> scan wifi for 30 seconds and tell me what you found

[agent calls: connect → scan_and_capture(scanall, 30) → analysis]

Found 8 access points across channels 1, 6, 11:
  • HomeNetwork_5G   ch 36  WPA2  -58 dBm
  • NETGEAR42        ch  6  WPA2  -71 dBm
  • [hidden]         ch  1  WPA2  -79 dBm  (BSSID used as ESSID)
  ...
3 stations associated. No open networks detected.

> save that capture

Saved to ~/storage/downloads/marauder_captures/marauder_scanall_20260703_211500.txt
```

Type `verbose` to see every tool call. Type `quit` to exit.

---

## Available MCP tools

| Tool | Description |
|------|-------------|
| `list_ports` | List serial ports (shows TCP bridge on Termux) |
| `connect` | Open connection — auto-detects USB or TCP bridge |
| `disconnect` | Close connection |
| `connection_status` | Check if connected and which port |
| `send_command` | Send any raw Marauder command |
| `read_output` | Poll live serial output |
| `scan_and_capture` | Run scan, capture all output, stop, return structured results |
| `get_capture` | Re-read the last capture buffer without re-scanning |
| `save_capture_local` | Save capture to device storage (Downloads on Android) |
| `scan_wifi` | Start `scanall` (use `read_output` to poll) |
| `stop_scan` | Stop any active scan |
| `list_access_points` | `list -a` |
| `list_stations` | `list -c` |
| `list_ssids` | `list -s` |
| `list_probes` | `list -p` |
| `get_settings` | Print Marauder settings |
| `read_local_file` | Read any file from device storage for analysis |
