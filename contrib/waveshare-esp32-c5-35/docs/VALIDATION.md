# Validation record

Validation date: 2026-08-09 (Europe/Berlin)

Tested hardware:

- Waveshare ESP32-C5-WIFI6-KIT-N32R8-UM, ESP32-C5 revision 1.2
- Waveshare 3.5inch Capacitive Touch LCD
- display microSD interface
- published path-sanitized application image SHA-256 `64307554007412dd8e4d480e27310024909879ed1697107765d493dd907eaac2`
- tested source head `f401e813276d8ebb3d660a314776572139b1146a`

## Build checks

| Check | Result |
| --- | --- |
| Waveshare C5 path-sanitized build | PASS - sketch 2,109,718 bytes; globals 67,044 bytes; application BIN 2,109,872 bytes |
| Existing Pancake C5 compatibility build | PASS - sketch 2,234,492 bytes; globals 74,852 bytes; application BIN 2,234,640 bytes |
| Firmware manifest and checksums | PASS - every recorded byte count and SHA-256 matches; all four embedded segments match the merged 8 MiB image at their documented offsets |
| Package scripts | PASS - all four PowerShell files parse successfully; isolated board-profile installer tests and build preparation completed |
| Enclosure STL exports | PASS - all nine binary STLs parse with valid facet counts and finite bounds; copied SHA-256 values match `enclosure/SHA256SUMS.txt` |
| Published asset metadata | PASS - all STL headers are generic; the PNG contains only IHDR/IDAT/IEND chunks and no author, text, EXIF, or local-path metadata |
| Focused OTA-safe USB hardware update | PASS on the immediately preceding build from the same final source and toolchain - Core-3.3.5 `boot_app0.bin` at `0xe000` plus application at `0x10000`; both transfer hashes verified |
| Published binary privacy scan | PASS - no username, email address, absolute host path, SSID, BSSID, device MAC, or known secret signature in the application or merged image |
| SD firmware update workflow | PASS on an earlier image from the same integration sequence; uploaded hash matched and OTA 1 booted |
| Repeated in-device slot transitions | PASS on the earlier integration image; four consecutive first-attempt transitions completed |
| Post-flash boot, display and touch | PASS |
| Reset, panic, watchdog or touch loss during focused tests | Not observed |

The published path-sanitized application and merged images were not separately
hardware-flashed. They use the hardware-tested source tree, pinned libraries,
core, FQBN, and functional defines; the only source change redacts the
runtime-generated SD file-manager password from serial diagnostics. GCC
file/macro prefix mapping removes host paths from diagnostic strings. The
SD-update and repeated slot-transition workflows were not repeated with the
published binary. See `KNOWN_LIMITATIONS.md`.

## Focused functional results

| Area | Result | Notes |
| --- | --- | --- |
| Target boot, display, touch and device information | PASS | Portrait display and capacitive touch operational |
| ADC battery and channel boundaries | PASS | Battery monitor and dual-band boundaries stable |
| Samsung BLE payload | PASS | Valid 0x0075 manufacturer payload observed in a btsnoop capture |
| Flipper BLE payload | LIMITED | Corrected static length was reviewed; the retest capture did not contain the Flipper signature |
| SD load/save with no card | PASS | Invalid load result and immediate save rejection |
| Capability-dependent CLI help | PASS | Unsupported commands absent on this target |
| AP labels and hidden/wildcard identification | PASS | Channel prefix, hidden AP fallback, and wildcard probe label verified |
| Probe counter output | PASS | `[REQ:n]` visible and plausible |
| Channel-summary paging | PASS | Every page shows seven entries; the overlapping final page remains in bounds with no phantom bar |
| Packet-monitor controls | PASS | Channel and scale controls separated and enlarged for the Waveshare enclosure |
| Host AP error/status/lifecycle | PASS | Acknowledged errors, AP IP/MAC, and client lifecycle verified |
| SSID list loading | PASS | Repeated load replaces the in-memory snapshot without duplicates |
| SD file manager | PASS | Browse, download, upload, explicit replace, delete, navigation, stop/restart |
| Filename and binary uploads | PASS | Text, PNG name containing literal `%20`, and a 2.6 MB HEIC round-tripped |
| File-manager display and password masking | PASS | Centered layout; SHOW/HIDE is edge-triggered and resets to hidden |
| Serial credential redaction | PASS (source and binary inspection) | Wi-Fi join and SD file-manager diagnostics emit `<redacted>`; the runtime file-manager password remains available through SHOW/HIDE on the device display |
| Download concurrency and stop guard | PASS | Large download completed while display actions remained stable |
| Evil Portal missing HTML path | PASS | Persistent acknowledged error, clean menu return, no empty log |
| Device-lock enrollment and unlock | PASS | Masked entry with SHOW/HIDE, minimum length, confirmation, reboot persistence |
| Retry delays and fifth-failure wipe | PASS | Increasing delays observed; fifth failure removed user data and disclosed only `DEVICE LOCKED` |
| PIN configuration | PASS | Unlock password, optional Wipe PIN, and lock enable/disable changed successfully |
| Explicit Wipe PIN | PASS | NVS, SPIFFS, coredump, and complete SD contents removed; SD remained writable and healthy |
| Post-wipe file manager and SD update | PASS | Multiple file formats round-tripped; firmware update retained SD contents |
| Firmware-slot UI | PASS | Reduced target-only menu, masked generic prompt, multiline high-contrast errors |
| Firmware-slot transition | PASS | OTA 0 -> SD update/OTA 1 -> internal verified switch/OTA 0 |
| Saved-list selection and transactional writes | PASS | Multiple files, cancel paths, replacement safety, and normal/error results verified |
| Touch-keyboard editing | PASS | Cursor, Home/End navigation, targeted deletion, larger controls, and debounce verified |
| Station details and offline OUI labels | PASS | Raw station MAC remains available; recognized manufacturers display without changing unknown entries |
| Persistent Wi-Fi profiles | PASS | Same-name networks remain separate by band; selected BSSID/channel joins and replacement behavior verified |
| Manual channel control | PASS | Unset/hopping option, 2.4/5-GHz band selection, fixed-channel state, and hopping restoration verified |
| Targeted disconnect addressing | PASS | Both directions used the selected AP BSSID; transmit failures are accounted for separately |
| Active EAPOL target filtering | PASS | 795 active frames used only the selected target; visible canary and foreign BSSIDs had zero hits |
| Quiet scheduling | PASS | 7,268 target beacons used Count 1, Period 5, Duration 20 TU, and Offset 32 TU |
| EAPOL replay, sequence, and WPA3 lengths | PASS | Replay/sequence advanced monotonically; 75/75 WPA3 frames used body length 95 and zero key-data length |
| Association channel metadata | PASS | Channel 1 and 36 captures contained the expected supported-channel tuple, operating class, and sequence behavior |
| SAE token target binding | PASS | 616 frames used one selected target; 608 commits echoed requested 32-byte tokens with correct placement |
| Final regression smoke | PASS | Unlock, display, touch, SD, file manager, AP scan/stop, saved Wi-Fi join, shutdown status, slot menu, and channel picker |

This was a focused regression pass, not a full hardware matrix rerun. Previously completed unaffected tests were intentionally not repeated.
