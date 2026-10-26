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

- `esp32_marauder/GhostLinkBridge.h` / `.cpp` — the bridge.
- `esp32_marauder/configs.h` — defines `HAS_BLE_UART_BRIDGE` inside the
  `MARAUDER_WS_C5_35` block.
- `esp32_marauder/esp32_marauder.ino` — `ghostlink_obj.RunSetup()` in `setup()`
  and `ghostlink_obj.main()` in `loop()`, both guarded by the flag.

## BLE contract as implemented

- Advertised name: `Marauder-XXXX` (`XXXX` = last two bytes of the BLE address).
  Apps match on the **service UUID**, not the name, so an honest Marauder name
  still links.
- `WD:CAPS` reports `board_name=Marauder_WS_C5_35`, `fw=1.6.9` (latest, so no OTA
  offer), and `ota=0 bt=0 gps=0 cap=0 usb=0` — only what this bridge implements.
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
  - `WD:CAPS fw=1.6.0 chip=esp32c5 bands=2g,5g ota=0 bt=0 gps=0 cap=0 usb=0 link=ble chans=...`
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
