# ESP32 Marauder — CYD 3.5" No Screen (Serial USB Mode)

Firmware for the ESP32-3248S035 (CYD 3.5" ST7796) with the display removed or non-functional.
All interaction happens over USB serial at **115200 baud**.

---

## Connection

| Setting  | Value   |
|----------|---------|
| Baud     | 115200  |
| Data     | 8N1     |
| Prompt   | `> `    |

Commands you type are echoed back prefixed with `#`. Output follows immediately after.

### Linux

```bash
# Find the port
ls /dev/tty{USB,ACM}*

# Connect (minicom)
minicom -D /dev/ttyUSB0 -b 115200

# Connect (screen)
screen /dev/ttyUSB0 115200

# Connect (picocom)
picocom /dev/ttyUSB0 -b 115200
```

---

## Flashing the Firmware

### First flash (esptool)

```bash
esptool.py --port /dev/ttyUSB0 --baud 921600 write_flash 0x1000 cyd_3_5_inch_no_screen.bin
```

### OTA update via SD card (while Marauder is already running)

1. Rename the `.bin` file to `update.bin`
2. Copy to the root of the SD card
3. Insert SD card and connect via serial
4. Run:

```
update -s
```

---

## Boot Output

On first connect you will see the Marauder ASCII banner followed by:

```
[Serial USB Mode] No display - type 'help' for commands

>
```

Type `help` at any time to print the full command list to serial.

---

## Command Reference

### Admin

#### `help`
Print all available commands.

```
help
```

---

#### `channel`
Get or set the current WiFi channel (1–14).

```
channel                  # print current channel
channel -s 6             # set channel to 6
```

---

#### `clearlist`
Clear one or more in-memory lists.

```
clearlist -a             # clear access point list
clearlist -s             # clear SSID list
clearlist -c             # clear station (client) list
```

Flags can be combined:

```
clearlist -a -s -c
```

---

#### `reboot`
Immediately restart the ESP32.

```
reboot
```

---

#### `update`
Flash new firmware from the SD card. The file must be named `update.bin` in the SD root.

```
update -s
```

---

#### `settings`
View or modify persistent settings stored in SPIFFS.

```
settings                              # print all settings as JSON
settings -s <name> enable            # enable a boolean setting
settings -s <name> disable           # disable a boolean setting
settings -r                          # reset all settings to defaults
```

**Available settings:**

| Name        | Type   | Default | Description |
|-------------|--------|---------|-------------|
| ForcePMKID  | bool   | false   | Force PMKID capture mode |
| ForceProbe  | bool   | false   | Force probe request mode |
| SavePCAP    | bool   | true    | Save packet captures to SD |
| EnableLED   | bool   | true    | Enable status LEDs |
| EPDeauth    | bool   | false   | Deauth clients during Evil Portal |
| ChanHop     | bool   | false   | Enable channel hopping during scans |
| ClientSSID  | String | ""      | Saved WiFi SSID for `join -s` |
| ClientPW    | String | ""      | Saved WiFi password for `join -s` |

Examples:

```
settings -s SavePCAP disable
settings -s ChanHop enable
settings -s EnableLED disable
```

---

#### `ls`
List the contents of an SD card directory.

```
ls /                     # list root
ls /logs                 # list /logs directory
```

---

#### `led`
Control the onboard RGB LED (requires HAS_NEOPIXEL_LED hardware).

```
led -s #FF0000           # set color by hex (red)
led -s #00FF00           # green
led -s #0000FF           # blue
led -p rainbow           # rainbow pattern
```

---

#### `brightness`
Read or set the backlight brightness level (0–9). Not applicable when display is absent but the command still compiles.

```
brightness               # print current level
brightness -c            # cycle to next level
brightness -s 5          # set level to 5
```

---

### GPS

GPS commands require a connected GPS module.

#### `gpsdata`
Stream live GPS data until `stopscan` is sent.

```
gpsdata
```

---

#### `gps`
Query individual GPS fields or set the NMEA output type.

```
gps -g fix               # fix status (yes/no)
gps -g sat               # satellite count
gps -g lat               # latitude
gps -g lon               # longitude
gps -g alt               # altitude
gps -g accuracy          # accuracy
gps -g date              # date and time
gps -g text              # raw text output
gps -g nmea              # generate GGA+RMC sentences
gps -g nmea -r           # raw recorded NMEA
gps -g nmea -i           # not-implemented NMEA sentences
gps -g nmea -p           # not-parsed NMEA sentences
gps -t                   # start GPS tracker mode
gps -n native            # set NMEA output type to native
gps -n all               # all constellations
gps -n gps               # GPS only
gps -n glonass           # GLONASS only
gps -n galileo           # Galileo only
gps -n navic             # NavIC only
gps -n qzss              # QZSS only
gps -n beidou            # BeiDou (uses GB prefix)
gps -n beidou -b         # BeiDou (uses BD prefix)
```

---

#### `nmea`
Stream raw NMEA sentences until `stopscan`.

```
nmea
```

---

#### `gpspoi`
Log GPS Points of Interest to SD.

```
gpspoi -s                # start POI session
gpspoi -m                # mark/log current location as POI
gpspoi -e                # end POI session
```

---

#### `gpstracker`
Track and log GPS positions over time.

```
gpstracker -c start
gpstracker -c stop
```

---

#### `wardrive`
Wardriving scan — logs WiFi APs with GPS coordinates to SD as a `.csv` (WiGLE format). Requires GPS module.

```
wardrive
stopscan                 # stop
```

---

#### `wardrivepoi`
Tag a Point of Interest during an active wardrive. Can include an optional label.

```
wardrivepoi
wardrivepoi coffee shop corner
```

---

### WiFi Scanning

All scans are stopped with `stopscan`. Add `-serial` to any scan command to also dump raw packet data to serial.

#### `stopscan`
Stop any active scan or attack.

```
stopscan                 # stop scan, reconnect WiFi
stopscan -f              # stop scan and disconnect WiFi too
```

---

#### `scanall`
Passive scan for both access points and connected stations simultaneously.

```
scanall
```

---

#### `sniffbeacon`
Capture 802.11 beacon frames from nearby access points.

```
sniffbeacon
```

---

#### `sniffprobe`
Capture probe request frames from nearby devices.

```
sniffprobe
```

---

#### `sniffdeauth`
Capture deauthentication frames.

```
sniffdeauth
```

---

#### `sniffraw`
Capture all raw 802.11 frames (promiscuous mode, no filter).

```
sniffraw
```

---

#### `sniffpwn`
Sniff for Pwnagotchi beacons.

```
sniffpwn
```

---

#### `sniffpinescan`
Sniff for PineAP probe responses (WiFi Pineapple detection).

```
sniffpinescan
```

---

#### `sniffmultissid`
Sniff for multi-SSID beacons.

```
sniffmultissid
```

---

#### `sniffsae`
Sniff SAE (Simultaneous Authentication of Equals / WPA3) commit frames.

```
sniffsae
```

---

#### `sniffpmkid`
Capture PMKID handshake material. Optionally force deauth to trigger handshake, or target only selected APs.

```
sniffpmkid                         # passive PMKID sniff on current channel
sniffpmkid -c 6                    # set channel 6 then sniff
sniffpmkid -d                      # sniff + deauth all APs to force handshake
sniffpmkid -d -l                   # deauth + sniff only selected APs
```

> Select target APs first with `select -a <index>` before using `-l`.

---

#### `sigmon`
Monitor WiFi signal strength.

```
sigmon
```

---

#### `packetcount`
Count WiFi packets per second.

```
packetcount
```

---

#### `mactrack`
Track a specific MAC address across channels.

```
mactrack
```

---

### Network Commands (requires WiFi connection)

First connect with `join`, then use these commands.

#### `pingscan`
Ping sweep the connected network.

```
pingscan
```

---

#### `arpscan`
ARP scan the connected network to discover hosts.

```
arpscan
```

---

#### `portscan`
Port scan a discovered IP address.

```
portscan -a -t 0          # full port scan on IP at index 0
portscan -s SSH           # scan for SSH (port 22) on all discovered IPs
portscan -s TELNET
portscan -s DNS
portscan -s HTTP
portscan -s SMTP
portscan -s HTTPS
portscan -s RDP
```

> Run `list -i` to see discovered IP addresses and their indices.

---

### WiFi Attacks

#### `attack`
Run a WiFi attack. Always requires `-t <type>`.

##### Deauthentication

```
attack -t deauth                          # deauth all clients on selected AP(s)
attack -t deauth -d ff:ff:ff:ff:ff:ff    # deauth to broadcast address
attack -t deauth -d AA:BB:CC:DD:EE:FF   # deauth specific client
attack -t deauth -c                       # deauth using station list
attack -t deauth -s AA:BB:CC:DD:EE:FF   # spoof source MAC
```

> Requires at least one AP selected with `select -a <index>` unless `-s` is specified.

##### Beacon Spam

```
attack -t beacon -l      # spam SSIDs from your SSID list
attack -t beacon -r      # spam random SSIDs
attack -t beacon -a      # clone and spam selected APs
```

##### Probe Spam

```
attack -t probe          # send probe requests for selected APs
```

> Requires selected APs.

##### Rick Roll

```
attack -t rickroll       # spam famous song lyric SSIDs
```

##### Funny SSIDs

```
attack -t funny          # spam joke/meme SSID names
```

##### Bad Msg (802.11 Action Frame)

```
attack -t badmsg         # send bad action frames to all stations
attack -t badmsg -c      # target only selected stations
```

##### Sleep Attack

```
attack -t sleep          # send sleep frames to all stations
attack -t sleep -c       # target only selected stations
```

##### SAE Commit Spam

```
attack -t sae            # flood SAE authentication commit frames
```

##### Channel Switch Announcement (CSA)

```
attack -t csa            # send CSA frames to disrupt clients
```

##### Quiet Time Attack

```
attack -t quiet          # send 802.11 Quiet IE frames
```

---

#### `evilportal`
Host a captive portal that captures credentials.

```
evilportal -c start                    # start with default index.html
evilportal -c start -w login.html      # start with specific HTML file from SD
evilportal -c sethtml login.html       # change HTML file without restarting
evilportal -c sethtmlstr               # pipe HTML from serial (interactive)
evilportal -c setap 0                  # set AP SSID from AP list index 0
```

> HTML files must be on the SD card. EPDeauth setting controls whether clients are deauthed to force portal connection.

---

#### `karma`
Karma attack — respond to any probe request with a matching AP.

```
# First sniff probes:
sniffprobe
stopscan

# List captured probes:
list -p

# Launch karma on probe index 0:
karma -p 0
```

---

### List Commands

#### `list`
Display in-memory lists.

```
list -a          # access points (APs)
list -s          # SSIDs
list -c          # stations (clients) grouped under their AP
list -t          # AirTags (from BLE scan)
list -i          # IP addresses (from ARP/ping scan)
list -p          # captured probe requests
```

Output format for APs:
```
[0][CH:6] MyNetwork -72
[1][CH:11] OtherNet -85 (selected)
```

---

#### `info`
Show device info or detailed AP info.

```
info             # device info (chip, memory, WiFi, etc.)
info -a 0        # detailed info for AP at index 0
```

---

### Selection

Selections persist until cleared. Many attacks and saves require targets to be selected first.

#### `select`
Toggle selection state of APs, stations, or SSIDs by index.

```
select -a 0              # toggle AP index 0
select -a 0,1,3          # toggle multiple APs
select -a all            # toggle all APs
select -c 0              # toggle station index 0
select -c all            # toggle all stations
select -s 0              # toggle SSID index 0
select -s all            # toggle all SSIDs

# Filter APs by name
select -a -f "contains Cafe"
select -a -f "equals MyNetwork"
select -a -f "contains Home or contains Office"
```

---

### SSID Management

#### `ssid`
Manage the custom SSID list (used by beacon spam attacks).

```
ssid -a -g 10            # generate 10 random SSIDs
ssid -a -n "Free WiFi"  # add a specific SSID
ssid -r 0                # remove SSID at index 0
list -s                  # view the SSID list
```

---

### Save / Load

Save and load AP or SSID lists to/from the SD card.

```
save -a                  # save AP list to SD
save -s                  # save SSID list to SD
load -a                  # load AP list from SD
load -s                  # load SSID list from SD
```

---

### Join WiFi

Connect the ESP32 to a WiFi network (enables pingscan, arpscan, portscan).

```
join -a 0 -p MyPassword        # connect to AP at index 0 with password
join -s                        # connect using saved ClientSSID / ClientPW settings
```

Save credentials for `join -s`:

```
settings -s ClientSSID enable   # NOTE: use saveSetting via settings command
```

> Currently ClientSSID and ClientPW are String-type settings; set them via the JSON settings file on SPIFFS directly, or through the companion app if available.

---

### MAC Address

#### `randapmac`
Generate and apply a random MAC address for AP mode.

```
randapmac
```

#### `randstamac`
Generate and apply a random MAC address for station mode.

```
randstamac
```

#### `cloneapmac`
Clone the MAC address of a scanned AP.

```
cloneapmac -a 0          # clone MAC of AP at index 0
```

#### `clonestamac`
Clone the MAC address of a scanned station.

```
clonestamac -s 0         # clone MAC of station at index 0
```

---

### Add Manually

Add APs or stations to the in-memory list without scanning.

```
add -a -b AA:BB:CC:DD:EE:FF                     # add AP with MAC (SSID = MAC)
add -a -b AA:BB:CC:DD:EE:FF -e "My AP" -ch 6   # add AP with name and channel
add -c -b 11:22:33:44:55:66 -ap 0              # add station linked to AP 0
```

---

### Bluetooth

#### `sniffbt`
Scan for BLE devices.

```
sniffbt                  # general BLE scan (all devices)
sniffbt -t airtag        # filter for Apple AirTags
sniffbt -t flipper       # filter for Flipper Zero BLE
sniffbt -t flock         # filter for Flock BLE devices
sniffbt -t meta          # filter for Meta (Ray-Ban) glasses
```

---

#### `blespam`
Spam BLE advertisement packets to annoy nearby devices.

```
blespam -t sourapple     # spam Apple devices with fake pairing popups
blespam -t applejuice    # Apple Juice / AirDrop spam
blespam -t windows       # Windows Swift Pair spam
blespam -t samsung       # Samsung fast-pair spam
blespam -t google        # Google Fast Pair spam
blespam -t flipper       # Flipper Zero spam
blespam -t all           # spam all types simultaneously
```

---

#### `spoofat`
Spoof a captured AirTag's identity.

```
sniffbt -t airtag        # first scan for AirTags
stopscan
list -t                  # view captured AirTags
spoofat -t 0             # spoof AirTag at index 0
```

---

#### `sniffskim`
Scan for Bluetooth credit card skimmers.

```
sniffskim
```

---

## Common Workflows

### Deauth a specific network

```
scanall
stopscan
list -a
select -a 0
attack -t deauth
stopscan
```

---

### Capture PMKID from a target AP

```
scanall
stopscan
list -a
select -a 0
sniffpmkid -d -l
stopscan
```

PCAP file is saved to SD automatically if `SavePCAP` is enabled.

---

### Evil Portal

```
# Put index.html on SD card root
evilportal -c setap 0
evilportal -c start
# Clients connect and are served the portal page
# Credentials printed to serial
stopscan
```

---

### Wardrive with GPS

```
wardrive
# Drive around — AP+GPS data auto-saves to SD as WiGLE CSV
wardrivepoi gas station   # tag a notable location
stopscan
```

---

### BLE device flooding

```
blespam -t all
stopscan
```

---

## SD Card Layout

| Path          | Contents |
|---------------|----------|
| `/update.bin` | Firmware file for OTA update (`update -s`) |
| `/index.html` | Default Evil Portal HTML page |
| `/*.pcap`     | Saved packet captures |
| `/*.csv`      | Wardrive logs (WiGLE format) |
| `/*.gpx`      | GPS track files |

---

## Settings Quick Reference

```
settings                              # view all
settings -s SavePCAP disable          # stop saving pcap to SD
settings -s ChanHop enable            # enable channel hopping
settings -s EnableLED disable         # turn off LED
settings -s EPDeauth enable           # deauth clients into evil portal
settings -s ForcePMKID enable         # force PMKID mode
settings -s ForceProbe enable         # force probe mode
settings -r                           # reset to defaults
```
