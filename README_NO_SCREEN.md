# ESP32 Marauder — CYD 3.5" No Screen (Serial USB Mode)

Firmware variant for the **CYD 3.5" ST7796** board with the display removed or broken.
All features are controlled exclusively through a serial USB terminal at **115 200 baud**.

---

## Hardware

| Board | CYD 3.5" (ESP32-3248S035 / similar ST7796 panel) |
|-------|--------------------------------------------------|
| Chip | ESP32 (dual-core Xtensa LX6, 240 MHz) |
| Flash | 4 MB |
| Build target | `MARAUDER_CYD_3_5_INCH_NO_SCREEN` |
| Baud rate | 115 200 |
| SD card CS | GPIO 5 (separate SPI bus: MISO 19, MOSI 23, SCK 18) |
| RGB LED | R=GPIO 22, G=GPIO 16, B=GPIO 17 |
| GPS UART | UART2 — TX=GPIO 21, RX=GPIO 25 |
| Button | GPIO 0 (BOOT button, active-low) |

---

## Flashing the Firmware

### Option A — SD Card OTA (recommended, Android-friendly)

1. Download `esp32_marauder_v*_cyd_3_5_inch_no_screen.bin` from the GitHub Actions artifact.
2. Copy it to the **root** of a microSD card and rename it to `update.bin`.
3. Insert the SD card into the CYD.
4. Connect your Android device via USB OTG and open a serial terminal (e.g. **Serial USB Terminal** by Kai Morich).
5. Set baud rate to **115 200** and connect.
6. Type: `update -s`
7. The board flashes itself and reboots into the new firmware.

### Option B — esptool (computer required)

```bash
esptool.py --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x1000 esp32_marauder_v*_cyd_3_5_inch_no_screen.bin
```

---

## Connecting via Android

1. Install **Serial USB Terminal** (Kai Morich) from the Play Store.
2. Connect the CYD to your Android phone with a USB-C OTG adapter (or Micro-USB OTG if your phone is older).
3. Open the app → tap the connection icon → select the device → set baud to **115 200** → connect.
4. You should see the Marauder banner and the `> ` prompt.

Recommended terminal settings:
- Baud: 115200
- Data bits: 8, Stop bits: 1, Parity: None
- Newline: `CR+LF` (send), `LF` (receive)
- Local echo: on (so you can see what you type)

---

## Boot Output

```
ESP32 Marauder
v1.12.3
By: justcallmekoko

[Serial USB Mode] No display - type 'help' for commands

>
```

Every command you type is echoed back prefixed with `#`. Responses follow immediately.

---

## Command Reference

### Conventions

| Symbol | Meaning |
|--------|---------|
| `<value>` | Required argument |
| `[-flag]` | Optional flag |
| `A/B` | Choose one |
| `index` | Zero-based integer from a `list` command |
| `stopscan` | Stops any running scan or attack |

Most scan and attack commands run continuously until you send `stopscan`.

---

## Administration

### `help`
Print the full command list to serial.

---

### `channel`
Show or set the active WiFi channel (1–14 for 2.4 GHz).

```
channel                   # show current channel
channel -s <1-14>         # set channel
```

---

### `settings`
View, change, or reset firmware settings stored in SPIFFS.

```
settings                              # print all current settings as JSON
settings -s <name> enable             # enable a boolean setting
settings -s <name> disable            # disable a boolean setting
settings -r                           # reset all settings to defaults
```

Common setting names:
- `ChanHop` — auto channel hopping during scans
- `SavePcap` — save captured packets to SD as PCAP
- `ClientSSID` / `ClientPW` — saved WiFi credentials for `join -s`
- `EnableLED` — RGB LED activity indicator

---

### `reboot`
Restart the ESP32 immediately.

```
reboot
```

---

### `update`
Flash new firmware from the SD card.

```
update -s     # flash /update.bin from SD card root, then reboot
```

Put the `.bin` file at the root of the microSD card and name it `update.bin`.

---

### `brightness`
Control backlight brightness (no effect without a display; kept for API compatibility).

```
brightness              # print current level (0-9)
brightness -c           # cycle to next level
brightness -s <0-9>     # set specific level
```

---

### `led`
Control the onboard RGB LED (GPIO 22/16/17).

```
led -s <#RRGGBB>        # set solid colour, e.g. led -s #FF0000 for red
led -p rainbow          # rainbow cycle mode
```

---

### `info`
Display device information (firmware version, heap, chip details) or AP detail.

```
info                    # device + memory info
info -a <index>         # detailed info for AP at index
```

---

### `ls`
List files on the SD card.

```
ls /                    # list root directory
ls /pcaps               # list a subdirectory
```

---

### `stopscan`
Stop any running scan or attack and return to idle.

```
stopscan                # stop gracefully
stopscan -f             # stop and fully disconnect WiFi
```

---

## WiFi — Passive Scanning

### `scanall`
Scan for all access points and associated client stations simultaneously.

```
scanall
```

Results are printed as they arrive. Use `list -a` and `list -c` after stopping to review.

---

### `sniffbeacon`
Sniff 802.11 beacon frames (passive AP discovery).

```
sniffbeacon
```

---

### `sniffprobe`
Sniff 802.11 probe requests (devices looking for networks).

```
sniffprobe
```

Captured probe SSIDs are stored in the probe list, viewable with `list -p`.

---

### `sniffdeauth`
Monitor for 802.11 deauthentication frames (detects deauth attacks in the area).

```
sniffdeauth
```

---

### `sniffraw`
Raw 802.11 packet capture — logs all frames to serial and/or SD PCAP.

```
sniffraw
sniffraw -serial        # also stream hex to serial
```

---

### `sniffpmkid`
Capture PMKID / EAPOL handshakes (WPA2 hash capture for offline cracking).

```
sniffpmkid                      # passive capture on current channel
sniffpmkid -c <channel>         # lock to specific channel first
sniffpmkid -d                   # send deauths to force re-association
sniffpmkid -d -l                # targeted: only deauth selected APs
```

Requires selected target APs when using `-l`. Use `select -a <index>` first.

---

### `sniffsae`
Sniff WPA3 SAE (Simultaneous Authentication of Equals) commit frames.

```
sniffsae
```

---

### `sniffpwn`
Sniff for Pwnagotchi beacons.

```
sniffpwn
```

---

### `sniffpinescan`
Detect WiFi Pineapple devices broadcasting nearby.

```
sniffpinescan
```

---

### `sniffmultissid`
Sniff beacons from APs broadcasting multiple SSIDs.

```
sniffmultissid
```

---

### `sniffskim`
Scan for Bluetooth credit-card skimmer devices.

```
sniffskim
```

---

### `sigmon`
Monitor signal strength of the currently selected AP(s) in real time.

```
sigmon
```

---

### `packetcount`
Display real-time WiFi packet counts by type (beacon, probe, data, etc.).

```
packetcount
```

---

### `mactrack`
Track a specific MAC address — alert when it appears in range.

```
mactrack
```

---

## WiFi — Active / Attacks

> **Note:** All attack commands require appropriate target selection first.
> Use `scanall` → `list -a` → `select -a <index>` to identify and pick targets.

### `attack`
Unified attack launcher. Required flag: `-t <type>`.

#### Deauthentication

Send 802.11 deauth frames to disconnect clients from their AP.

```
attack -t deauth                          # deauth all clients of selected APs (broadcast)
attack -t deauth -c                       # deauth targeted client station list
attack -t deauth -d <dst_mac>             # deauth specific destination MAC
attack -t deauth -s <src_mac> -d <dst_mac>  # manual deauth with custom source MAC
```

Requires at least one AP selected with `select -a`. Use `list -c` after `scanall` to identify client stations, then `select -c <index>` to target them with `-c`.

---

#### Beacon Spam

Flood the 2.4 GHz band with fake AP beacons.

```
attack -t beacon -r                # random SSIDs (continuous spam)
attack -t beacon -l                # use your custom SSID list (see ssid commands)
attack -t beacon -a                # clone beacons from selected APs in scan list
```

---

#### Probe Spam

Send probe request frames impersonating selected APs.

```
attack -t probe
```

Requires selected APs.

---

#### Rick Roll Beacon

Spam beacons with song-lyric SSIDs.

```
attack -t rickroll
```

---

#### Funny Beacon

Spam beacons with humorous SSIDs.

```
attack -t funny
```

---

#### Bad Message Attack

Send malformed 802.11 management frames that can crash or disconnect clients.

```
attack -t badmsg          # attack all nearby stations
attack -t badmsg -c       # attack only selected/targeted stations
```

---

#### Sleep Attack

Force clients into power-save mode via crafted TIM frames.

```
attack -t sleep           # attack all nearby stations
attack -t sleep -c        # attack only targeted stations
```

---

#### SAE Commit Spam (WPA3)

Flood nearby devices with SAE commit frames (WPA3 DoS).

```
attack -t sae
```

---

#### CSA Attack (Channel Switch Announcement)

Broadcast fake channel-switch frames to disrupt clients.

```
attack -t csa
```

---

#### Quiet Attack

Send 802.11 Quiet element frames to silence a frequency band segment.

```
attack -t quiet
```

---

### `evilportal`
Launch a rogue access point with a captive web portal to capture credentials.

```
evilportal -c start                          # start with default SSID/page
evilportal -c start -w <page.html>           # use custom HTML file from SD card
evilportal -c setap <ap_index>              # clone the SSID of a scanned AP
evilportal -c sethtml <page.html>           # pre-select HTML file without starting
evilportal -c sethtmlstr                    # paste raw HTML via serial (interactive)
```

Credentials captured from victims are printed to serial as they arrive.

---

### `karma`
Karma attack — respond to probe requests with a matching fake AP, luring the probing device to connect.

```
# First, capture probes:
sniffprobe
stopscan
list -p              # shows captured probe SSIDs with index

karma -p <index>     # start karma AP matching probe at index
```

---

## WiFi — Network Tools (requires `join`)

These commands only work after connecting to an existing network with `join`.

### `join`
Connect to an access point as a client.

```
join -a <ap_index> -p <password>    # connect to scanned AP by index
join -s                             # connect using saved ClientSSID / ClientPW settings
```

---

### `pingscan`
ICMP ping scan of the local subnet after joining a network.

```
pingscan
```

---

### `arpscan`
ARP scan to discover all active hosts on the local subnet.

```
arpscan
arpscan -f          # fast mode
```

---

### `portscan`
TCP port scan against a discovered host.

```
# First run arpscan, then list IPs:
list -i

portscan -a -t <ip_index>                    # full port scan of IP at index
portscan -s <ssh/telnet/dns/http/smtp/https/rdp>   # scan for specific service
```

---

### `gpspoi`
Mark and manage GPS Points of Interest (requires WiFi join for location context).

```
gpspoi -s     # start POI session
gpspoi -m     # mark current GPS location as POI
gpspoi -e     # end POI session
```

---

## List & Selection Commands

These commands manage the in-memory lists of APs, stations, SSIDs, and other discovered items.

### `list`
Display items from any list.

```
list -a         # access points (APs)
list -c         # client stations (grouped by AP)
list -s         # custom SSID list
list -t         # AirTags (from sniffbt -t airtag)
list -i         # IP addresses (from arpscan/pingscan)
list -p         # probe request SSIDs (from sniffprobe)
```

Example output for `list -a`:
```
[0][CH:6] MyNetwork -72
[1][CH:11] CoffeeShop -65 (selected)
[2][CH:1] XFINITY -80
2 selected
```

---

### `select`
Toggle the selected state of items in a list. Selected items are used as targets for attacks, saves, and PMKID captures.

```
select -a <index>                       # toggle single AP by index
select -a <idx1,idx2,idx3>              # toggle multiple APs (comma-separated)
select -a all                           # toggle all APs
select -a -f "equals <SSID>"           # select by exact SSID match
select -a -f "contains <partial>"      # select by partial SSID match
select -a -f "equals <A> or contains <B>"  # multi-filter

select -c <index>                       # toggle single station
select -c all                           # toggle all stations

select -s <index>                       # toggle SSID in custom list
select -s all                           # toggle all custom SSIDs
```

---

### `ssid`
Manage the custom SSID list used for beacon spam (`attack -t beacon -l`).

```
ssid -a -g <count>          # add <count> random generated SSIDs
ssid -a -n <name>           # add a specific SSID by name
ssid -r <index>             # remove SSID at index
list -s                     # view the list
```

---

### `add`
Manually add an AP or station to the in-memory lists.

```
add -a -b <mac>                          # add AP with just MAC (auto-named)
add -a -b <mac> -ch <channel> -e <ssid> # add AP with full details
add -c -b <mac> -ap <ap_index>           # add client station linked to an AP
```

MAC format: `XX:XX:XX:XX:XX:XX`

---

### `save`
Save current lists to the SD card.

```
save -a     # save AP list to SD
save -s     # save custom SSID list to SD
```

---

### `load`
Load previously saved lists from the SD card.

```
load -a     # load AP list from SD
load -s     # load SSID list from SD
```

---

### `clearlist`
Clear in-memory lists.

```
clearlist -a    # clear AP list
clearlist -s    # clear SSID list
clearlist -c    # clear station list
```

---

### `info`
Show device info or detailed AP info.

```
info                # chip info, free heap, firmware version
info -a <index>     # full details of AP at index (channel, BSSID, RSSI, clients, EAPOL status)
```

---

## MAC Address Commands

### `randapmac`
Set a random MAC address for AP mode operations.

```
randapmac
```

---

### `randstamac`
Set a random MAC address for station mode operations.

```
randstamac
```

---

### `cloneapmac`
Clone the MAC address of a scanned AP.

```
cloneapmac -a <ap_index>
```

---

### `clonestamac`
Clone the MAC address of a scanned client station.

```
clonestamac -s <station_index>
```

---

## Bluetooth Commands

### `sniffbt`
Scan for Bluetooth / BLE devices.

```
sniffbt                     # general BLE scan — all devices
sniffbt -t airtag           # detect Apple AirTags
sniffbt -t flipper          # detect Flipper Zero devices
sniffbt -t flock            # detect Flock safety cameras
sniffbt -t meta             # detect Meta Ray-Ban glasses
```

Results printed to serial as devices are seen. Use `list -t` to view AirTag results.

---

### `blespam`
Spam BLE advertisement packets to flood nearby devices with pairing/notification popups.

```
blespam -t sourapple        # Apple popup spam (fake AirPods, etc.)
blespam -t applejuice       # Apple Juice attack (iOS notification flood)
blespam -t google           # Google Fast Pair popup spam
blespam -t samsung          # Samsung pairing popup spam
blespam -t windows          # Windows Swift Pair popup spam
blespam -t flipper          # Flipper Zero BLE presence spam
blespam -t all              # cycle through all attack types simultaneously
```

---

### `spoofat`
Spoof an AirTag — impersonate a captured AirTag's MAC to confuse tracking apps.

```
# First capture AirTags:
sniffbt -t airtag
stopscan
list -t              # view captured AirTags with index

spoofat -t <index>   # start spoofing AirTag at index
```

---

## GPS Commands

Requires an external GPS module connected to UART2 (TX=21, RX=25).

### `gpsdata`
Stream live GPS position data to serial until `stopscan`.

```
gpsdata
```

---

### `gps`
Query or configure the GPS module.

```
gps -g fix          # current fix status (no fix / 2D / 3D)
gps -g sat          # number of visible satellites
gps -g lat          # latitude
gps -g lon          # longitude
gps -g alt          # altitude (metres)
gps -g accuracy     # horizontal accuracy estimate
gps -g date         # current date and time (UTC)
gps -g text         # raw GPS text output
gps -g nmea         # generate GGA+RMC sentences

gps -n native       # set NMEA output to native mode
gps -n all          # enable all satellite systems
gps -n gps          # GPS only
gps -n glonass      # GLONASS only
gps -n galileo      # Galileo only
gps -n beidou       # BeiDou (uses GB prefix)
gps -n beidou -b    # BeiDou (uses BD prefix)
gps -n navic        # NavIC
gps -n qzss         # QZSS

gps -t              # start GPS tracker mode (logs track to SD)
```

---

### `nmea`
Stream raw NMEA sentences from the GPS module to serial.

```
nmea
```

---

### `gpspoi`
Mark GPS Points of Interest to a log file on SD.

```
gpspoi -s     # start POI logging session
gpspoi -m     # mark current location
gpspoi -e     # end session
```

---

### `gpstracker`
Start or stop the GPS track recorder.

```
gpstracker -c start
gpstracker -c stop
```

---

### `wardrive`
GPS-assisted wardriving — scan for APs and log them with coordinates to SD (Wigle-compatible CSV).

```
wardrive
```

Requires a GPS fix. Stop with `stopscan`.

---

### `wardrivepoi`
Tag the current GPS location with a label during an active wardrive.

```
wardrivepoi                     # tag with no label
wardrivepoi Coffee Shop Spot    # tag with a label
```

Only works while `wardrive` is running.

---

## SD Card Commands

The CYD 3.5" uses a dedicated SPI bus for SD (MISO=19, MOSI=23, SCK=18, CS=5). A FAT32-formatted microSD card is required.

### `ls`
Browse the SD card filesystem.

```
ls /
ls /pcaps
ls /aps
```

---

### Files Written Automatically

| File pattern | Content |
|---|---|
| `/aps/aps_<date>.csv` | AP scan results |
| `/pcaps/<date>.pcap` | Packet captures (when `SavePcap` enabled) |
| `/wardrive_<date>.csv` | Wardrive GPS log (Wigle format) |
| `/gpspoi_<date>.csv` | GPS POI log |
| `/ssids.txt` | Saved SSID list |
| `update.bin` | Firmware file for `update -s` |

---

## Typical Workflows

### Scan → Deauth

```
> scanall                     # discover APs and clients
(wait 20-30 seconds)
> stopscan
> list -a                     # find target AP index
[0][CH:6] TargetNetwork -72
> select -a 0                 # select it
> attack -t deauth            # send broadcast deauths
(attack runs)
> stopscan
```

---

### PMKID Capture (WPA2 hash grab)

```
> scanall
> stopscan
> list -a
[2][CH:11] HomeNetwork -65
> select -a 2
> sniffpmkid -c 11 -d         # lock channel, send deauths to force handshake
(wait for PMKID capture — prints hash when captured)
> stopscan
```

The PMKID hash is also saved to the PCAP file if `SavePcap` is enabled.

---

### Beacon Spam with Custom SSIDs

```
> ssid -a -n "FBI Surveillance Van"
> ssid -a -n "Pretty Fly for a WiFi"
> ssid -a -n "Tell My WiFi Love Her"
> list -s                     # confirm
> attack -t beacon -l         # spam using the list
> stopscan
```

---

### Evil Portal (Credential Phishing)

```
> scanall
> stopscan
> list -a
> evilportal -c setap 3       # clone SSID of AP at index 3
> evilportal -c start         # launch portal (default login page)
(victims connect and enter credentials — printed to serial)
> stopscan
```

To use a custom HTML file from SD:
```
> evilportal -c start -w myportal.html
```

---

### BLE Spam

```
> blespam -t sourapple        # barrage of Apple device popups
> stopscan
> blespam -t all              # everything at once
> stopscan
```

---

### AirTag Hunt

```
> sniffbt -t airtag
(AirTags printed as detected)
> stopscan
> list -t
[0] MAC: AA:BB:CC:DD:EE:FF
> spoofat -t 0                # spoof that AirTag
```

---

### Join WiFi → Port Scan

```
> scanall
> stopscan
> list -a
> join -a 1 -p "mypassword"   # connect to AP index 1
> arpscan                     # discover hosts
> stopscan
> list -i
[0] 192.168.1.1
[1] 192.168.1.100
> portscan -a -t 1            # full scan of 192.168.1.100
> stopscan
```

---

## Settings Reference

View all settings with `settings`. Change with `settings -s <name> enable/disable`.

| Setting | Default | Description |
|---------|---------|-------------|
| `ChanHop` | enable | Auto-hop channels during passive scans |
| `SavePcap` | enable | Write PCAP files to SD card |
| `ClientSSID` | — | Saved SSID for `join -s` |
| `ClientPW` | — | Saved password for `join -s` |
| `EnableLED` | enable | RGB LED activity indicators |
| `SaveAPs` | disable | Auto-save AP list on scan stop |

Reset to defaults: `settings -r`

---

## Serial Output Format

Each command input is echoed back prefixed with `#`:
```
> scanall
#scanall
Scanning for APs and Stations. Stop with stopscan
[AP] MyNetwork CH:6 RSSI:-72 BSSID:AA:BB:CC:DD:EE:FF
...
```

Scan results stream continuously until `stopscan`. After stopping, use `list -a` to review what was captured.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| No `>` prompt on boot | Check baud rate is 115200; try pressing the BOOT button (GPIO 0) to reset |
| `SD card NOT Supported` | Reformat SD as FAT32; try a different card (≤32 GB works best) |
| `GPS not supported` | GPS module not wired; `HAS_GPS` is still compiled in but needs hardware |
| Attack has no effect | You need at least one AP selected — run `scanall`, then `select -a <index>` |
| `update -s` fails | Verify `update.bin` is at the root of the SD card (not in a folder) |
| BLE commands not working | Board is built with `HAS_BT` enabled; try `reboot` if BLE stack gets stuck |
| Command does nothing | Some commands require WiFi join first (`pingscan`, `arpscan`, `portscan`) |

---

## Building Locally

1. Install Arduino IDE 2.x with the ESP32 board package (version 2.0.11).
2. In `esp32_marauder/configs.h`, uncomment:
   ```c
   #define MARAUDER_CYD_3_5_INCH_NO_SCREEN
   ```
3. In `User_Setup_Select.h` (project root), **leave all entries commented out** — TFT_eSPI is not used in this build.
4. Select board: **LOLIN D32** with partition scheme **Minimal SPIFFS**.
5. Compile and upload.
