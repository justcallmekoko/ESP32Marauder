# ESP32 Marauder — CYD 3.5" (No Screen)

ESP32 Marauder firmware for the **ESP32-3248S035** (CYD 3.5" ST7796) running headless over USB serial at 115200 baud. No display required — all control is via serial terminal or the included Venice AI MCP client.

---

## Hardware

| Item | Value |
|------|-------|
| Board | ESP32-3248S035 (CYD 3.5") |
| Chip | ESP32-D0WD-V3 |
| Flash | 4 MB |
| PSRAM | None |
| SD slot | Yes (separate SPI — CS=5, MISO=19, MOSI=23, SCK=18) |
| Serial | UART0 via USB, 115200 baud |

---

## Build

CI runs automatically on push and via **Actions → Run workflow**.

The workflow produces a single merged binary (`*_merged.bin`) that combines bootloader + partition table + application — flash it at `0x0000`, no address math needed.

Artifact is available under the **Actions** tab after each successful run, retained for 5 days.

---

## Flash

### Erase + flash (first time or after corruption)

```bash
pip install esptool

# Erase everything first
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 erase_flash

# Flash the merged binary at 0x0000
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x0000 esp32_marauder_v*_cyd_3_5_inch_no_screen_merged.bin
```

### Update only (bootloader already intact)

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x0000 esp32_marauder_v*_cyd_3_5_inch_no_screen_merged.bin
```

> **Port:** `/dev/ttyUSB0` on Linux · `/dev/cu.usbserial-*` on macOS · `COM3` on Windows  
> Hold **BOOT** button while connecting if the device doesn't enter flash mode automatically.

### SD card OTA

Copy `update.bin` (rename the app `.bin` to `update.bin`) to the root of the SD card, then:

```
> update -s
```

---

## Serial terminal

Connect at **115200 baud**. The device prints a banner and `> ` prompt on boot.

```
picocom -b 115200 /dev/ttyUSB0        # Linux / macOS
# Ctrl+A then Ctrl+X to exit
```

Or use any serial terminal app (Android: Serial USB Terminal · Windows: PuTTY).

---

## Commands

### Admin

| Command | Description |
|---------|-------------|
| `help` | Print all commands |
| `reboot` | Reboot the device |
| `channel -s <1-14>` | Set WiFi channel |
| `clearlist -a` | Clear access point list |
| `clearlist -c` | Clear station list |
| `clearlist -s` | Clear SSID list |
| `settings` | Show current settings |
| `settings -s <name> enable\|disable` | Toggle a setting |
| `settings -r` | Reset settings to default |
| `update -s` | OTA from SD card (`/update.bin`) |
| `ls <dir>` | List SD card directory |
| `led -s <hex>` | Set LED color (e.g. `ff0000`) |
| `led -p rainbow` | Rainbow LED pattern |
| `brightness -s <0-9>` | Set brightness level |
| `brightness -c cycle` | Cycle brightness |
| `info [-a <index>]` | Show AP info |
| `gpsdata` | Print raw GPS data |
| `nmea` | Stream NMEA sentences |

### WiFi — Scan / Sniff

| Command | Description |
|---------|-------------|
| `scanall` | Passive scan for APs and stations |
| `sniffbeacon` | Sniff beacon frames |
| `sniffprobe` | Sniff probe requests |
| `sniffdeauth` | Sniff deauth frames |
| `sniffpmkid [-c <ch>] [-d] [-l]` | Sniff PMKID handshakes |
| `sniffpwn` | Sniff pwnagotchi frames |
| `sniffraw` | Raw 802.11 frame capture |
| `sniffpinescan` | Pine64 scan mode |
| `sniffmultissid` | Multi-SSID sniff |
| `sniffsae` | Sniff SAE (WPA3) handshakes |
| `sniffskim` | Sniff Bluetooth skimmers |
| `sigmon` | Signal strength monitor |
| `packetcount` | Count packets per channel |
| `wardrive` | GPS-tagged wardrive (requires GPS) |
| `stopscan [-f]` | Stop active scan (`-f` flush to SD) |
| `mactrack` | Track a MAC address |

### WiFi — Attack

| Command | Description |
|---------|-------------|
| `attack -t deauth [-c]` | Deauth attack on selected APs |
| `attack -t beacon [-l\|-r\|-a]` | Beacon flood (list / random / all) |
| `attack -t probe` | Probe flood |
| `attack -t rickroll` | Rick-roll beacon flood |
| `attack -t badmsg [-c]` | Bad management frame flood |
| `attack -t sleep [-c]` | Sleep attack |
| `attack -t sae` | SAE commit flood |
| `attack -t csa` | Channel switch announcement attack |
| `attack -t quiet` | Quiet element attack |

### WiFi — Management

| Command | Description |
|---------|-------------|
| `list -a` | List access points |
| `list -c` | List stations |
| `list -s` | List SSIDs |
| `list -t` | List AirTags |
| `list -i` | List IP addresses |
| `list -p` | List probe SSIDs |
| `select -a <index>` | Select AP by index |
| `select -c <index>` | Select station by index |
| `select -s <index>` | Select SSID by index |
| `select -f "contains <str>"` | Filter select |
| `ssid -a [-n <name>\|-g <count>]` | Add SSID (named or random) |
| `ssid -r <index>` | Remove SSID |
| `add -a -b <mac> [-ch <ch>] [-e <ssid>]` | Manually add AP |
| `save -a` | Save APs to SD |
| `save -s` | Save SSIDs to SD |
| `load -a` | Load APs from SD |
| `load -s` | Load SSIDs from SD |
| `join -a <index> -p <pass>` | Connect to AP |
| `join -s` | Disconnect |
| `randapmac` | Randomize AP MAC |
| `randstamac` | Randomize station MAC |
| `cloneapmac [-a <index>]` | Clone AP MAC |
| `clonestamac [-s <index>]` | Clone station MAC |

### Network

| Command | Description |
|---------|-------------|
| `pingscan` | Ping scan local subnet |
| `portscan -a -t <index>` | Port scan all common ports on IP |
| `portscan -s <ssh\|http\|https\|...>` | Scan specific service port |
| `arpscan [-f]` | ARP scan (`-f` fast) |
| `evilportal -c start -w <file.html>` | Start evil portal |
| `evilportal sethtml <file.html>` | Set portal HTML |
| `karma -p <index>` | Karma attack on probe SSID |

### Bluetooth

| Command | Description |
|---------|-------------|
| `sniffbt -t airtag\|flipper\|flock\|meta` | Sniff BT devices |
| `blespam -t applejuice\|google\|samsung\|windows\|flipper\|all` | BLE spam |
| `spoofat -t <index>` | Spoof AirTag |
| `sniffskim` | Sniff Bluetooth skimmers |

### Settings reference

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `SavePCAP` | bool | true | Save captures to SD as PCAP |
| `EnableLED` | bool | true | RGB LED enabled |
| `ChanHop` | bool | false | Auto channel hop during scan |
| `ForcePMKID` | bool | false | Force PMKID capture mode |
| `ForceProbe` | bool | false | Force probe response |
| `EPDeauth` | bool | false | Deauth clients during evil portal |
| `ClientSSID` | string | — | SSID for `join` command |
| `ClientPW` | string | — | Password for `join` command |

---

## MCP Server (Venice AI)

Control the device through natural language using **Venice AI** (Gemma 4) via the `mcp-use` terminal client.

### Install

```bash
cd mcp_server
pip install mcp mcp-use pyserial langchain-openai
```

### Configure

```bash
export VENICE_API_KEY="your_key_here"
export MARAUDER_PORT="/dev/ttyUSB0"    # your serial port
```

### Run

```bash
python mcp_server/client.py
```

The client auto-launches the MCP server subprocess — one command is all you need.

```
ESP32 Marauder AI Terminal
Model : gemma-4-uncensored via Venice AI
Port  : /dev/ttyUSB0

> scan wifi for 15 seconds then list what you found
> deauth attack on AP index 0
> stop the scan and save results to SD
```

Type `verbose` to watch every tool call. Type `quit` to exit.

### Available MCP tools

| Tool | Action |
|------|--------|
| `list_ports` | Detect serial ports |
| `connect` | Open serial connection |
| `disconnect` | Close connection |
| `connection_status` | Check connection state |
| `send_command` | Send any Marauder command |
| `read_output` | Poll ongoing scan output |
| `scan_wifi` | Start `scanall` |
| `stop_scan` | Run `stopscan` |
| `list_access_points` | Run `list -a` |
| `list_stations` | Run `list -c` |
| `get_settings` | Run `settings` |

---

## SD card layout

```
/
├── update.bin          ← OTA firmware (rename app bin to this)
├── *.pcap              ← packet captures (auto-saved if SavePCAP enabled)
├── *.csv               ← wardrive logs
└── html/
    └── portal.html     ← evil portal HTML
```
