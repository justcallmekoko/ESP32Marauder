# JSON Serial Interface

A small, **machine-readable command set** layered on top of the existing USB
serial CLI, plus a way to run the graphical analyzers **without a screen**, plus
a **length-prefixed binary capture stream** that lets a host stand in for the
device's SD card and GPS. It lets any host program (automation scripts, desktop
tools, companion apps, test harnesses) read device data over the serial port by
parsing JSON instead of scraping the human-formatted CLI text.

- **Base firmware:** `v1.12.4`
- **Protocol version:** `2` (reported by `jsoninfo`, defined as `MARAUDER_JSON_PROTO` in `configs.h`)
- **Transport:** USB serial (`115200 8N1` on connect; raise with `jsonbaud`) — same port and CLI as today

### Protocol history

| `proto` | Adds |
|---------|------|
| `1` | Initial JSON command set (`jsoninfo` / `jsonstatus` / `jsonlist` / `jsonmode`) + screen-free analyzers. |
| `2` | Length-prefixed **binary capture streaming** (`"capstream"` capability), `jsonbaud` (raise the line rate), and `{"t":"drop","n":N}` loss reporting. Replaces the old text `[BUF/BEGIN]…[BUF/CLOSE]` capture markers. |

A host should read `proto` from the `jsoninfo` reply and the `caps` array (look
for `"capstream"`) rather than assuming a version — everything in proto 1 still
behaves exactly as before.

---

## 1. Design goals

- **Additive & low-risk.** The scanning internals are untouched. The JSON
  commands only *read* the data structures the firmware already maintains
  (`access_points`, `stations`, `ssids`, …). Existing CLI behaviour is unchanged.
- **Parser-friendly.** Every machine-readable text line is prefixed with `@J ` so
  a host can cleanly separate it from human output and command echoes (`#…`,
  `> `). Each such line is a single, self-contained JSON object (NDJSON) with a
  `"t"` type tag. Binary capture frames start with a `0xFE` sync byte that can
  never appear in UTF-8 text, so a host demultiplexer separates frames from text
  unambiguously (see §4).
- **Board-neutral.** Compiles and behaves identically on every board, with a
  screen or without one. Nothing here is specific to any particular host.
- **Opt-in streaming.** Continuous analyzer output is gated behind a "JSON mode"
  flag, so a plain serial monitor is never flooded with `@J` lines. Binary
  capture streaming only happens for a capture command explicitly issued with
  `-serial`.
- **Host stands in for SD/GPS.** With capture streaming the host can save pcap
  and log data itself and supply GPS, so an SD card and on-board GPS module are
  optional.

---

## 2. Commands

Text responses are single lines prefixed with `@J ` followed by one JSON object.

| Command | Response |
|---------|----------|
| `jsoninfo` | `@J {"t":"info","fw":"v1.12.4","proto":2,"board":"<HARDWARE_NAME>","caps":["wifi","capstream",…]}` — handshake / capability advertisement. **Also enables JSON mode** (analyzer streaming). |
| `jsonstatus` | `@J {"t":"status","mode":<n>,"running":<bool>,"free":<heap>,"aps":<n>,"stas":<n>,"ssids":<n>,"ips":<n>,"probes":<n>,"airtags":<n>}` — poll-friendly device state. |
| `jsonlist a` | per access point: `@J {"t":"ap","i":..,"ch":..,"rssi":..,"sel":..,"pkts":..,"sec":..,"wps":..,"nsta":..,"bssid":"..","essid":".."}` |
| `jsonlist s` | per SSID: `@J {"t":"ssid","i":..,"ch":..,"sel":..,"essid":".."}` |
| `jsonlist c` | per station (client): `@J {"t":"sta","ap":..,"i":..,"sel":..,"pkts":..,"mac":".."}` |
| `jsonlist i` | per IP: `@J {"t":"ip","i":..,"ip":".."}` |
| `jsonlist p` | per probe request: `@J {"t":"probe","i":..,"req":..,"sel":..,"essid":".."}` |
| `jsonlist t` | per AirTag: `@J {"t":"airtag","i":..,"rssi":..,"sel":..,"mac":".."}` |
| `jsonlist <x>` (unknown) | `@J {"t":"err","cmd":"jsonlist","arg":"x"}` |
| *(end of any list)* | `@J {"t":"end","list":"<a\|s\|c\|i\|p\|t>","n":<rows-actually-sent>}` |
| `jsonmode 1` / `jsonmode 0` | `@J {"t":"jsonmode","on":<bool>}` — toggles JSON mode (streaming) without a full handshake. Accepts `1`/`on`/`true`. |
| `jsonbaud <rate>` | `@J {"t":"baud","rate":<n>}` — **proto ≥ 2.** Raise (or lower) the serial line rate so a host can drain captured pcap faster than the 115200 default. See §5. |

Notes:
- `caps` always starts with `"wifi"`, then `"capstream"` (proto ≥ 2, always
  present — capture streaming and `jsonbaud` are board-independent), then appends
  `"bt"`, `"gps"`, `"sd"`, `"screen"` depending on the board's compile-time
  features.
- Every `jsonlist` stream is terminated by a `{"t":"end",…}` line whose `n` is
  the exact number of rows emitted, so a host knows when the list is complete.
- `essid` values are UTF-8-escaped: valid UTF-8 passes through, control
  characters / `"` / `\` are escaped, and invalid bytes become `U+FFFD`. The
  emitted line is therefore always valid JSON, even for arbitrary 802.11 SSID
  bytes.

---

## 3. Headless (screen-free) analyzers

The graphical analyzers — **Channel Analyzer**, **BT Analyzer**, and **Channel
Activity** — were previously reachable only from the on-device touchscreen menu,
and their data emit lived inside the screen-drawing path. A host with no display
(or a screen board that never opened the analyzer menu) could not receive that
data.

This is now available over serial:

- **New CLI command** `analyzer [-t <wifi|bt|chan>]` starts the analyzer with no
  screen interaction (defaults to the WiFi channel analyzer). Stop with
  `stopscan`. On a screen board it also renders the same on-device chart the menu
  would.
- `signalAnalyzerLoop` / `channelActivityLoop` were refactored so the **sample
  computation and the `@J` emit run unconditionally**; only the on-device chart
  drawing and touch buttons remain `#ifdef HAS_SCREEN`. The underlying data
  collection was already screen-independent.

Streamed lines (only while JSON mode is on):

| Line | Meaning |
|------|---------|
| `@J {"t":"asample","mode":<n>,"ch":<ch\|-1>,"v":<sample>}` | one rolling-graph point per refresh (Channel / BT analyzer). `ch` is `-1` when not channel-meaningful (BT). |
| `@J {"t":"chan","page":<p>,"ch":[<real channels>],"v":[<counts>]}` | the currently displayed page of the Channel Activity bar chart, with real channel numbers so no out-of-band mapping is needed. |

Typical host flow: connect → `jsoninfo` (enables JSON mode) → `analyzer -t wifi`
→ consume `@J asample` / `@J chan` → `stopscan`.

---

## 4. Binary capture streaming (proto ≥ 2)

Any capture that would normally be written to the SD card (pcap sniffs, `.log`
captures, `.gpx` tracks) can instead be streamed to the host over USB. Issue the
capture command with the `-serial` flag (with the `SavePCAP` setting enabled);
`Buffer` then emits each flush as a length-prefixed binary **frame** on the
serial port instead of writing a file. This lets a phone/desktop save the capture
itself, so an on-device SD card is optional.

### Frame layout

Each `Buffer` flush (`Buffer::saveSerial`) is one frame, written with a single
`Serial.write` so it is not interleaved with other output:

```
+--------+--------+--------+---------+-----------------+--------+
|  SYNC  |  seq   |  type  |   len   |     payload     | crc32  |
| 4 bytes| 4 LE   | 1 byte | 4 LE    |   len bytes     | 4 LE   |
+--------+--------+--------+---------+-----------------+--------+
  FEEDFACE                                     ^ everything from seq..payload
```

| Field | Size | Meaning |
|-------|------|---------|
| `SYNC` | 4 | Constant `FE ED FA CE`. Begins with `0xFE`, a byte that never occurs in valid UTF-8, so a host's line reader can switch into binary-frame mode the moment it sees it — text `@J`/console lines and binary frames never collide. |
| `seq` | 4, little-endian | Per-capture frame counter, **reset to 0** each time a capture is opened. A host detects dropped frames as a gap in `seq`. |
| `type` | 1 | Payload kind: `0` = pcap, `1` = log, `2` = gpx. |
| `len` | 4, little-endian | Payload length in bytes. |
| `payload` | `len` | The raw pcap / log / gpx bytes for this flush. For pcap, the first frame's payload begins with the standard pcap global header (link-type 105). |
| `crc32` | 4, little-endian | CRC-32 (IEEE 802.3, poly `0xEDB88320`) computed over **`seq` + `type` + `len` + `payload`** — i.e. every byte between `SYNC` and `crc32` (`9 + len` bytes). The host recomputes it and rejects a frame on mismatch. |

### Loss reporting

The capture ring is bounded. When it is full a packet is dropped and counted
instead of vanishing silently; right after the next frame the firmware emits a
text line reporting the exact number lost since the previous report:

```
@J {"t":"drop","n":<N>}
```

A host should account for these against the `seq` sequence so the saved file's
gaps are explicit.

### Host demultiplexing

A host reads the byte stream once and splits it three ways:

1. A line that starts with `@J ` → a JSON object (parse by `"t"`).
2. Any other text line → ordinary console output.
3. A `0xFE` byte where a line would start → a binary frame: read `SYNC`, then
   `seq`/`type`/`len`, then `len` payload bytes, then `crc32`; validate the CRC.

Because `0xFE` can never begin a UTF-8 text line, this split is unambiguous.

---

## 5. Raising the line rate — `jsonbaud`

`jsonbaud <rate>` lets a host drain captures faster than the 115200 default
(companion apps raise the link to 921600 after the handshake):

- The confirmation `@J {"t":"baud","rate":<n>}` is sent **at the old rate** and
  `Serial.flush()`ed before the switch, so the host can read it, then re-open its
  own port at the new rate — no bytes are lost across the change.
- A missing or bogus argument (`rate < 9600`) is clamped to `115200`.
- On native USB-CDC boards (`ARDUINO_USB_CDC_ON_BOOT`) the underlying
  `updateBaudRate()` is a harmless no-op — USB-CDC runs at its own speed
  regardless of the requested rate — but the `@J {"t":"baud"}` reply is still
  emitted so the host handshake is identical on every board.

---

## 6. Files

### Added
| File | Purpose |
|------|---------|
| `esp32_marauder/JsonSerial.h` | Public API + command tokens for the JSON interface |
| `esp32_marauder/JsonSerial.cpp` | `jsoninfo` / `jsonstatus` / `jsonlist` / `jsonmode` / `jsonbaud` + analyzer streaming |
| `JSON_SERIAL_INTERFACE.md` | This document |

### Modified (additive only)
| File | Change |
|------|--------|
| `configs.h` | `#define MARAUDER_JSON_PROTO 2` |
| `JsonSerial.cpp` / `JsonSerial.h` | proto ≥ 2: `jsonbaud` command + `"t":"baud"` reply; `"capstream"` advertised in `jsoninfo` caps |
| `Buffer.cpp` / `Buffer.h` | proto ≥ 2: `saveSerial()` emits the length-prefixed binary frame (`SYNC`/`seq`/`type`/`len`/`payload`/`crc32`); a `dropped` counter + `{"t":"drop"}` reporting; `seq_no` and `stream_type` framing state. Replaces the old `[BUF/BEGIN]…[BUF/CLOSE]` text markers. |
| `CommandLine.cpp` | `#include "JsonSerial.h"`; dispatch JSON commands at the top of `runCommand`; add the `analyzer` command; suppress the interactive `> ` prompt while JSON mode is on |
| `CommandLine.h` | `analyzer` command token + help string |
| `WiFiScan.cpp` | `#include "JsonSerial.h"`; stream analyzer / channel-activity samples independent of the screen |
| `WiFiScan.h` | Defensive `BANNER_TIME` fallback (used by the now screen-independent analyzer loops) |
| `MenuFunctions.h` | `renderGraphUI()` made public so the `analyzer` CLI command can draw the same chart as the menu |

---

## 7. Compatibility & behaviour

- **No change to existing commands.** JSON commands are dispatched first and
  return immediately when matched; everything else falls through to the current
  CLI exactly as before.
- **Quiet by default.** `jsonlist` / `jsonstatus` / `jsoninfo` always respond,
  but continuous analyzer *streaming* only happens after `jsoninfo` or
  `jsonmode 1`, and binary capture *frames* only for a capture started with
  `-serial`, so an ordinary serial monitor is not flooded.
- **Interactive prompt.** While JSON mode is on, the CLI stops re-printing the
  `> ` prompt (it has no trailing newline and would otherwise glue onto the next
  `@J` line). A human on a serial monitor still gets the prompt normally.
- **GPS-NMEA passthrough.** During an active `nmea` passthrough, JSON commands
  are handled ahead of the passthrough **only when JSON mode is on**, so a plain
  NMEA consumer keeps a pure sentence stream.

---

## 8. Build & test

- **Nothing to enable.** The JSON commands compile on every board; build as
  usual (Arduino IDE / `arduino-cli` / PlatformIO): select the board in
  `configs.h`, copy the matching `User_Setup` for TFT_eSPI, compile.
- **Quick test over USB (`115200 8N1`):**
  1. `jsoninfo` → one `@J {"t":"info",…,"proto":2,"caps":[…,"capstream",…]}` line.
  2. `scanall` → `stopscan` → `jsonlist a` → `@J {"t":"ap",…}` lines + `@J {"t":"end",…}`.
  3. `jsonstatus` → one counts line.
  4. `analyzer -t wifi` → `@J {"t":"asample",…}` lines → `stopscan`.
  5. `jsonbaud 921600` → `@J {"t":"baud","rate":921600}` (re-open the host port at 921600).
  6. A pcap sniff issued with `-serial` (with `SavePCAP` on) → binary frames
     (`FE ED FA CE …`) → validate each CRC-32 → optional `@J {"t":"drop",…}`.

---

## 9. Known limitation

The JSON list commands read the shared `access_points` / `stations` / … lists
from the CLI task without a lock, the same way the existing human-readable `list`
command does. Polling `jsonlist` / `jsonstatus` during an active sniff therefore
shares the firmware's existing unsynchronized-list exposure. Adding a mutex
around all list add/remove/read is a firmware-wide change and is intentionally
out of scope for this additive feature.
