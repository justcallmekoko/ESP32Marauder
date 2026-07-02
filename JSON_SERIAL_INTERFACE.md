# JSON Serial Interface

A small, **machine-readable command set** layered on top of the existing USB
serial CLI, plus a way to run the graphical analyzers **without a screen**. It
lets any host program (automation scripts, desktop tools, companion apps, test
harnesses) read device data over the serial port by parsing JSON instead of
scraping the human-formatted CLI text.

- **Base firmware:** `v1.12.3`
- **Protocol version:** `1` (reported by `jsoninfo`, defined as `MARAUDER_JSON_PROTO` in `configs.h`)
- **Transport:** USB serial (`115200 8N1`) — same port and CLI as today

---

## 1. Design goals

- **Additive & low-risk.** The scanning internals are untouched. The JSON
  commands only *read* the data structures the firmware already maintains
  (`access_points`, `stations`, `ssids`, …). Existing CLI behaviour is unchanged.
- **Parser-friendly.** Every machine-readable line is prefixed with `@J ` so a
  host can cleanly separate it from human output and command echoes (`#…`, `> `).
  Each line is a single, self-contained JSON object (NDJSON) with a `"t"` type tag.
- **Board-neutral.** Compiles and behaves identically on every board, with a
  screen or without one. Nothing here is specific to any particular host.
- **Opt-in streaming.** Continuous analyzer output is gated behind a "JSON mode"
  flag, so a plain serial monitor is never flooded with `@J` lines.

---

## 2. Commands

All responses are single lines prefixed with `@J ` followed by one JSON object.

| Command | Response |
|---------|----------|
| `jsoninfo` | `@J {"t":"info","fw":"v1.12.3","proto":1,"board":"<HARDWARE_NAME>","caps":["wifi",…]}` — handshake / capability advertisement. **Also enables JSON mode** (analyzer streaming). |
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

Notes:
- `caps` always starts with `"wifi"` and appends `"bt"`, `"gps"`, `"sd"`,
  `"screen"` depending on the board's compile-time features.
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

## 4. Files

### Added
| File | Purpose |
|------|---------|
| `esp32_marauder/JsonSerial.h` | Public API + command tokens for the JSON interface |
| `esp32_marauder/JsonSerial.cpp` | `jsoninfo` / `jsonstatus` / `jsonlist` / `jsonmode` + analyzer streaming |
| `JSON_SERIAL_INTERFACE.md` | This document |

### Modified (additive only)
| File | Change |
|------|--------|
| `configs.h` | `#define MARAUDER_JSON_PROTO 1` |
| `CommandLine.cpp` | `#include "JsonSerial.h"`; dispatch JSON commands at the top of `runCommand`; add the `analyzer` command; suppress the interactive `> ` prompt while JSON mode is on |
| `CommandLine.h` | `analyzer` command token + help string |
| `WiFiScan.cpp` | `#include "JsonSerial.h"`; stream analyzer / channel-activity samples independent of the screen |
| `WiFiScan.h` | Defensive `BANNER_TIME` fallback (used by the now screen-independent analyzer loops) |
| `MenuFunctions.h` | `renderGraphUI()` made public so the `analyzer` CLI command can draw the same chart as the menu |

---

## 5. Compatibility & behaviour

- **No change to existing commands.** JSON commands are dispatched first and
  return immediately when matched; everything else falls through to the current
  CLI exactly as before.
- **Quiet by default.** `jsonlist` / `jsonstatus` / `jsoninfo` always respond,
  but continuous analyzer *streaming* only happens after `jsoninfo` or
  `jsonmode 1`, so an ordinary serial monitor is not flooded.
- **Interactive prompt.** While JSON mode is on, the CLI stops re-printing the
  `> ` prompt (it has no trailing newline and would otherwise glue onto the next
  `@J` line). A human on a serial monitor still gets the prompt normally.
- **GPS-NMEA passthrough.** During an active `nmea` passthrough, JSON commands
  are handled ahead of the passthrough **only when JSON mode is on**, so a plain
  NMEA consumer keeps a pure sentence stream.

---

## 6. Build & test

- **Nothing to enable.** The JSON commands compile on every board; build as
  usual (Arduino IDE / `arduino-cli` / PlatformIO): select the board in
  `configs.h`, copy the matching `User_Setup` for TFT_eSPI, compile.
- **Quick test over USB (`115200 8N1`):**
  1. `jsoninfo` → one `@J {"t":"info",…}` line.
  2. `scanall` → `stopscan` → `jsonlist a` → `@J {"t":"ap",…}` lines + `@J {"t":"end",…}`.
  3. `jsonstatus` → one counts line.
  4. `analyzer -t wifi` → `@J {"t":"asample",…}` lines → `stopscan`.

---

## 7. Known limitation

The JSON list commands read the shared `access_points` / `stations` / … lists
from the CLI task without a lock, the same way the existing human-readable `list`
command does. Polling `jsonlist` / `jsonstatus` during an active sniff therefore
shares the firmware's existing unsynchronized-list exposure. Adding a mutex
around all list add/remove/read is a firmware-wide change and is intentionally
out of scope for this additive feature.
