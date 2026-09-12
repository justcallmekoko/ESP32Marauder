# Waveshare ESP32-C5 3.5-inch Marauder package

This directory is a self-contained, optional hardware contribution for:

- Waveshare ESP32-C5-WIFI6-KIT-N32R8-UM
- Waveshare 3.5inch Capacitive Touch LCD (ST7796S + FT6336U)

Merging this directory does not enable the target and does not change any existing Marauder source, installer, workflow, or configuration file. The tested source delta is carried only as a reference snapshot patch at `source/waveshare-c5-tested-snapshot.patch`; maintainers can review, split, or apply it separately.

## Contents

| Path | Purpose |
| --- | --- |
| `firmware/` | Path-sanitized application and merged flash images, partition data, hashes, and build identity |
| `profile/` | TFT_eSPI setup and an optional Arduino `boards.local.txt` fragment |
| `source/waveshare-c5-tested-snapshot.patch` | Complete source diff that reproduces the tested source tree from the recorded upstream base |
| `tools/` | Reproducible CLI build helper, OTA-safe flash helper, and optional local board-profile installer |
| `wiring/` | Pin-by-pin PNG wiring diagram for the tested display and controller |
| `enclosure/` | Slicer-ready enclosure STLs, print notes, hashes, and the recorded parts list |
| `docs/` | Build, flash, validation, limitations, and provenance notes |

## Fastest hardware trial

For a new device or an unknown partition layout, `firmware/esp32_marauder_waveshare_c5_35_merged.bin` provides a complete 8 MiB image starting at address `0x0`. This erases the complete internal flash; see `docs/FLASH.md` before using it. For later USB updates, use the OTA-safe two-file procedure, which writes `boot_app0.bin` and the application together. Opening a serial monitor with DTR/RTS enabled can reset this board. The optional board-profile installer disables both lines in the profile and migrates matching cached Arduino IDE monitor entries, preserving unrelated board settings and backing up the cache before a change. This is a defensive host-side setting; a particular USB driver or serial client may still reset the board when opening the port and must be validated separately.

The publishable application image is rebuilt from the hardware-tested source
and pinned toolchain with compiler prefix maps that remove host usernames and
absolute build paths. Its SHA-256 is:

`64307554007412dd8e4d480e27310024909879ed1697107765d493dd907eaac2`

The path-sanitized binary was not separately hardware-flashed. The immediately
preceding build from the hardware-tested source tree and toolchain completed the
documented hardware run. The published snapshot adds only a diagnostic change
that redacts the runtime-generated SD file-manager password from serial output;
see `docs/VALIDATION.md` and `docs/KNOWN_LIMITATIONS.md`.

## Rebuild

The hardware-tested source state is based on upstream commit `9f6c10dbce99efe3b2ff56f5805a0f2036b67fdf` and ends at tested commit `f401e813276d8ebb3d660a314776572139b1146a`. The published snapshot adds the serial diagnostic redaction recorded in `source/SNAPSHOT.md`. Rebuilds require Espressif Arduino core `esp32:esp32` 3.3.5 for the upstream ESP32-C5 MSPI soft-reset fix. Apply the reference snapshot to a disposable branch or worktree, then run `tools/build-waveshare-c5.ps1`. See `source/SNAPSHOT.md` and `docs/BUILD.md`.

## Scope

This package captures one fully tested integration snapshot. General Marauder bug fixes in that snapshot are intended for separate focused pull requests; their presence here documents and reproduces the flashed image, not a request to merge the full snapshot into the main source tree.

The device lock is a software access-control layer, not a replacement for ESP32-C5 Secure Boot or flash encryption. Its limits, wipe behavior, and the documented firmware-slot confirmation code are described in `docs/KNOWN_LIMITATIONS.md` and `docs/FLASH.md`.

Use Marauder and its wireless features only on systems and networks you own or are explicitly authorized to test.
