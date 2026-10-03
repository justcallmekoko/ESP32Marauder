# Known limitations and security boundary

## Hardware and validation

- Only the Waveshare ESP32-C5 revision 1.2 setup described here received the focused hardware run.
- The existing Pancake target was compile-tested for compatibility, not rerun on hardware.
- The published application and merged images were rebuilt from the hardware-tested source and pinned toolchain with compiler prefix maps that remove host paths, plus one diagnostic-only change that redacts the runtime-generated SD file-manager password from serial output. These path-sanitized binaries were generated, structurally verified, and hashed but were not separately hardware-flashed. The immediately preceding non-publishable application build completed the focused OTA 0 hardware run. SD update and repeated internal slot switching were validated on an earlier image from the same integration sequence.
- The corrected Flipper BLE payload was not present in the final over-the-air capture, so that specific radio observation remains limited.
- Hidden APs are labeled correctly, but joining a hidden AP remains a separate unresolved behavior.
- Some upper 5 GHz channels can still be rejected by the regional driver; the channel-summary fix only addresses paging and bounds safety.
- Opening the CH343 serial interface with DTR or RTS asserted can reset the board. Use 115200 baud with DTR and RTS disabled.

## Device lock, wipe, and firmware slots

- The device lock is a software access-control layer. Secure Boot, flash encryption, and irreversible eFuse configuration are intentionally not enabled; USB reflashing remains available for recovery.
- Passwords are salted and hashed, but an attacker with physical access and specialist flash-analysis tooling is outside this feature's protection boundary.
- A Wipe PIN is optional. Five failed unlock attempts always trigger the configured user-data wipe even when no Wipe PIN exists.
- The wipe removes NVS, formats SPIFFS, erases the coredump partition, deletes the complete mounted SD contents, recreates `/SCRIPTS`, and verifies that the card is writable. Firmware, bootloader, partition table, and OTA application slots remain.
- FAT deletion and flash erasure are not certified forensic sanitization. Wear-leveling, storage-controller behavior, interrupted power, or a removed/failing SD card can prevent guaranteed physical-media erasure.
- An interrupted wipe is marked in NVS and retried on the next boot while that marker remains available.
- The fifth-failure display intentionally reveals only `DEVICE LOCKED`; the explicit Wipe-PIN path provides owner-facing progress and failure details.
- The firmware-slot confirmation code is fixed and documented in `FLASH.md`. It prevents accidental changes but is not a secret or a second authentication factor.
- Slot changes validate the target image before changing boot metadata. This cannot prove that every feature in an older but structurally valid image is compatible with current settings.

## SD file manager

- The random WPA2 access-point password is the authentication boundary. The HTTP UI has no separate user account or end-to-end TLS.
- The request token protects browser actions against ordinary cross-site requests; it is delivered to every client already admitted to the WPA2 network and is not a second authentication factor.
- Zero-byte multipart uploads are not reported by the pinned web-server stack and are currently unsupported.
- A power loss or SD failure during replacement can leave a hidden recovery file. FAT 8.3 aliases may make such an internal long filename addressable even when the long-name prefix is hidden by the UI; recovery files must not be treated as secret storage.
- A client that already knows the WPA2 password can continue sending an oversized upload until disconnect, creating a local resource-exhaustion window.
- One Wi-Fi client at a time is the intended operating mode.

## Saved Wi-Fi profiles

- Saved Wi-Fi profile passwords and the legacy client credential are stored on internal flash without application-layer encryption. The device lock limits ordinary UI access but does not encrypt these files; physical flash access or firmware compromise can expose them. Credential encryption and migration remain a separate future change.

These limitations are documented so the package reproduces the tested state without overstating its security properties.
