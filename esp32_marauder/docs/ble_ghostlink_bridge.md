# WarDriveGo-compatible wardrive BLE bridge

An optional BLE wardrive bridge for the Waveshare ESP32-C5-Touch-LCD-3.5
(`MARAUDER_WS_C5_35`). The board advertises a BLE peripheral that speaks the
published *WardriveESP / GhostLink* wire protocol, so the **WarDriveGo** iOS app
discovers it by service UUID and streams Marauder's Wi-Fi scan results over BLE
— phone-based wardriving with the Marauder as the scanning node.

It identifies **honestly as a Marauder**: it advertises a `Marauder-XXXX` name
and reports `board_name=Marauder_WS_C5_35` in `WD:CAPS`. It is **not** WardriveESP
firmware and does not pretend to be — it only speaks the same wire protocol for
interoperability. It implements no OTA characteristic and reports a firmware
version WarDriveGo will not try to upgrade, so the app offers no firmware update.

The same BLE server also exposes a second surface — the **Biscuit Manager** GATT
service plus the standard Device Information Service — so the Biscuit Manager app
can drive the board as a pure wardriver too. Both links share one Wi-Fi scan;
each scanned AP is fanned out to whichever link(s) asked, in that link's format.
See *Biscuit Manager link* below.

Strictly opt-in: scoped entirely behind `HAS_BLE_UART_BRIDGE`, which is only
defined for this one board, so no other Marauder target is affected.

> **Clean-room interop note.** Only the *wire protocol* is implemented here, from
> the publicly published WardriveESP `PROTOCOL.md`. No third-party firmware source
> is copied, derived from, or bundled. WardriveESP's `NOTICE.md` states the
> protocol is MIT/original and that its BLE UUIDs, frame layout and `WD:AP` line
> format "match GhostESP's GhostLink bridge for interoperability only" — so a
> clean-room implementation of the wire protocol is the intended path. If any
> third-party source is ever bundled, preserve its MIT notice.

## Files

- `esp32_marauder/GhostLinkBridge.h` / `.cpp` — the bridge (both surfaces).
- `esp32_marauder/BiscuitLink.h` / `.cpp` — standalone, dependency-free
  clean-room codec for the Biscuit Manager line protocol (parse CMD / format
  `DATA:AP` / status JSON / band filter). MIT; links into the bridge.
- `esp32_marauder/configs.h` — defines `HAS_BLE_UART_BRIDGE` inside the
  `MARAUDER_WS_C5_35` block.
- `esp32_marauder/esp32_marauder.ino` — `ghostlink_obj.RunSetup()` in `setup()`
  and `ghostlink_obj.main()` in `loop()`, both guarded by the flag.

## BLE contract as implemented

- Advertised name: `Marauder-XXXX` (`XXXX` = last two bytes of the BLE address).
  Apps match on the **service UUID**, not the name, so an honest Marauder name
  still links.
- `WD:CAPS` reports `board_name=Marauder_WS_C5_35`, `fw=99.99.99` (sentinel above
  any real WardriveESP release, so the app's version check never offers an OTA
  and never goes stale), and `ota=0 bt=0 gps=0 cap=0 usb=0` — only what this
  bridge implements.
- Service UUID: `47686F73-7445-5350-4272-696467655376`
  - RX   `0147686F-7374-4553-5042-726964675852` (write / write-no-response)
  - TX   `0247686F-7374-4553-5042-726964675854` (notify)
  - CTRL  `0347686F-7374-4553-5042-72694C525443` (write / write-no-response)
- Preferred MTU 247; each notification is one GhostLink frame.

### Frame header (12 bytes, little-endian)

| offset | field        | value                              |
|-------:|--------------|------------------------------------|
| 0..1   | magic        | `'G'`, `'B'`                       |
| 2      | version      | `1`                                |
| 3      | frame type   | 1 CMD, 2 ACK, 3 DATA, 4 END, 5 ERR |
| 4      | status       | 0 = OK                             |
| 5      | flags        | CMD: `0x01` first / `0x02` more    |
| 6..9   | command id   | uint32                             |
| 10..11 | payload len  | uint16                             |

The companion concatenates **DATA** payloads and splits on `\n`, so a record
may span several frames.

### Control + handshake

- Commands arrive on RX or CTRL, either as a CMD frame (payload = text) or as
  raw unframed text. A framed CMD gets an ACK with the same command id.
- On TX subscribe (and at `wdstream start -wifi`) the board greets with, before
  any `WD:AP`:
  - `WD:CAPS fw=99.99.99 chip=esp32c5 bands=2g,5g ota=0 bt=0 gps=0 cap=0 usb=0 board_name=Marauder_WS_C5_35 link=ble chans=...`
    — capabilities are reported **honestly** for what v1 implements.
  - `WD:MODE mode=wifi`
  - `WD:BEGIN type=wifi interval=0 channel=auto`
  - `WD:PLAN chans=...`
- `wdstream start -wifi` → begin streaming; `stop` → `WD:END reason=stop`.
- Any other text is forwarded to Marauder's normal CLI (`cli_obj.runCommand`).
  v1 does **not** tee CLI text output back over BLE — only the GhostLink DATA
  path carries wardrive records.

### Record formats

```
WD:AP ts=<ms> bssid=AA:BB:CC:DD:EE:FF ssid_hex=<lowerhex> rssi=<dBm> ch=<n> auth=<OPEN|WEP|WPA|WPA2|WPA3|WPA2_WPA3|UNKNOWN> hidden=<0|1>
WD:STATUS aps=<n> scans=<n>
WD:END reason=stop
```

`ch > 14` indicates a 5 GHz AP. `ssid_hex` is empty when `hidden=1`.

## Biscuit Manager link (second surface)

A second GATT service on the same NimBLE server lets the **Biscuit Manager** app
drive the board as a wardriving node. It is a simple newline-delimited text
protocol (no GhostLink framing): the phone writes commands to CMD and subscribes
to RSP for replies.

- Service UUID `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
  - CMD `beb5483e-36e1-4688-b7f5-ea07361b26a8` (write) — `CMD:<cmd>[:params]`
  - RSP `beb5483e-36e1-4688-b7f5-ea07361b26a9` (notify) — replies, one line each
  - STA `beb5483e-36e1-4688-b7f5-ea07361b26aa` (read) — status JSON
  - SET `beb5483e-36e1-4688-b7f5-ea07361b26ab` (read/write) — settings (unused)
- Device Information Service `0x180A`, which the app reads to identify the node:
  Manufacturer (`2A29`) `Biscuit Shop`, Model (`2A24`) `Biscuit DIY`, Firmware
  (`2A26`) `MARAUDER_VERSION`.
- Advertising: two 128-bit service UUIDs cannot share one 31-byte PDU, so while
  unconnected the advertised UUID + scan-response name rotates (~1.28 s) between
  the WardriveESP slot (`Marauder-XXXX`) and the Biscuit slot (`Biscuit`).
- **GAP device name follows the active link.** The connected name (`0x2A00`)
  becomes `Biscuit` once the app subscribes to RSP, and reverts to `Marauder-XXXX`
  on a WardriveESP TX-subscribe or disconnect. A dual-protocol app (WarDriveGo
  speaks both surfaces) classifies the device by this name after connecting, so a
  static name made the Biscuit session never start and the WardriveESP link
  stall; with this, WarDriveGo lists the board under **both** its WardriveESP and
  Biscuit sections, both streaming.

### Commands and replies

- `CMD:wardriveall` / `CMD:scanap` → all bands; `CMD:wardrive` → 2.4 GHz only
  (ch ≤ 14); `CMD:wardrive5g` → 5 GHz only (ch ≥ 36). Reply `RSP:<cmd>:OK` then
  `STATUS:2:Scan started: <cmd>`, then `DATA:AP` lines.
- `CMD:stopscan` → `RSP:stopscan:OK` then `STATUS:1:Ready`.
- Any other `CMD:<x>` → `RSP:<x>:ERROR:unsupported`.
- On RSP subscribe the board greets with `TIME:REQUEST:` then `STATUS:1:Ready`,
  and repeats `TIME:REQUEST:` every 5 s until the app answers `TIME:SYNC:<iso>`
  (the value is unused — only its arrival stops the retry).

```
DATA:AP:<SSID>,<BSSID-upper>,<channel>,<rssi>,<security>
STATUS:<n>:<message>          # n: 1 ready, 2 busy
{"status":<n>,"battery":<0..100|-1>}   # the STA read characteristic
```

The SSID has `,`, CR and LF flattened to `_` so it can never break the
comma/line-delimited protocol. APs are deduplicated by BSSID (60 s window) so a
parked stream stops re-sending every sweep.

> **Clean-room interop note.** `BiscuitLink.{h,cpp}` implements only the *wire
> format* (UUIDs, command tokens, line layouts) from the public Biscuit-Wiki GATT
> Reference — an interface specification, not copied source. MIT-licensed and
> dependency-free.

## Build

```bash
arduino-cli compile \
  --fqbn "esp32:esp32:esp32c5:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled" \
  --libraries "<libs dir>" \
  --build-property "compiler.cpp.extra_flags=-DMARAUDER_WS_C5_35" \
  esp32_marauder
```

Same toolchain the CI `ws_c5_35` target uses: esp32 core `3.3.4`,
NimBLE-Arduino `2.3.8`, TFT_eSPI `H4W9@ESP32-C5` with
`User_Setup_ws_c5_35.h` selected.

## Coexistence notes (single-core ESP32-C5)

- Wi-Fi sweeps use `WiFi.scanNetworks(async)`, so touch and the main loop keep
  running during a scan. Sweeps are paced (`GL_SWEEP_GAP_MS`).
- Wi-Fi scanning and the BLE peripheral share one radio; throughput drops while
  a central is connected. That is accepted for v1.
- Marauder's own BLE features (`blespam`, `findmy`, `sniffbt`, ...) call
  `NimBLEDevice::deinit()`, which tears the bridge down. `main()` detects this
  (`NimBLEDevice::isInitialized()`) and re-initializes + re-advertises, but the
  active BLE link is lost for the duration of that feature.
- Intended for use while Marauder is otherwise idle; the bridge does not
  arbitrate the radio against an in-progress Marauder Wi-Fi scan.
