# Tested source snapshot

`waveshare-c5-tested-snapshot.patch` is the complete source delta used to produce the published firmware. It is a reproducibility artifact inside an additive contribution; it is not a request to merge that delta wholesale into Marauder's main source tree.

- upstream base: `9f6c10dbce99efe3b2ff56f5805a0f2036b67fdf`
- hardware-tested source head: `f401e813276d8ebb3d660a314776572139b1146a`
- hardware-tested source tree: `9fa19d322f33c67078edcfbc8ebfa83f03d4d335`
- expected published Git tree: `adb2fa1bb9ba49d8a662bedc0ca0d1bc3e1305f9`
- publication-only delta: redact the runtime-generated SD file-manager password from serial output
- snapshot patch SHA-256: `0977e6750c046b1933cdde339adea611170f46fddfe4adbb2d93ec518de1c617`
- author identity: `p-keminer <p-keminer@users.noreply.github.com>`

Apply the snapshot only in a disposable sibling worktree while this package branch remains checked out:

```powershell
$packageRoot = (Resolve-Path './contrib/waveshare-esp32-c5-35').Path
$sourceWorktree = Join-Path (Split-Path (Get-Location).Path -Parent) 'ESP32Marauder-waveshare-c5-source'
git worktree add --detach $sourceWorktree 9f6c10dbce99efe3b2ff56f5805a0f2036b67fdf
git -C $sourceWorktree apply --index --binary (Join-Path $packageRoot 'source/waveshare-c5-tested-snapshot.patch')
git -C $sourceWorktree write-tree
```

`git write-tree` must print `adb2fa1bb9ba49d8a662bedc0ca0d1bc3e1305f9`.

## Snapshot history represented

1. Waveshare ESP32-C5 target integration
2. ADC battery and dual-band channel hardening
3. Samsung and Flipper BLE payload correction
4. Invalid-file behavior when SD is unavailable
5. Capability-gated CLI help
6. Channel information in AP menu labels
7. Probe request counters in `list -p`
8. Complete channel-summary paging
9. Immediate save rejection without SD
10. Host-AP start status and AP network identity
11. Stable final channel-summary page
12. Acknowledgement-gated AP failure dialog
13. Wildcard probe labeling
14. Hidden AP identification
15. Host-AP event lifecycle
16. Idempotent SSID snapshot loading
17. SD file manager
18. SD file-manager upload hardening
19. Evil Portal and file-manager lifecycle hardening
20. AP and station snapshot persistence
21. Dual-band channel-state synchronization and rejected-channel recovery
22. Waveshare packet-monitor control layout
23. Software device lock, retry delays, PIN configuration, and explicit Wipe PIN
24. NVS, SPIFFS, coredump, and SD user-data wipe with post-wipe SD health check
25. Verified OTA-slot selection with masked developer confirmation
26. Saved-list file selection, transactional saves, and I/O result handling
27. Cursor navigation and larger controls for the C5 touch keyboard
28. Firmware-image validation and acknowledged update failures
29. Station detail display and offline OUI labels
30. Band-aware persistent Wi-Fi profiles and selected-BSSID joins
31. Explicit dual-band channel controls and live hopping state
32. Corrected targeted disconnect addressing and transmit accounting
33. Active EAPOL target filtering and bounded Quiet scheduling
34. Advancing replay and 802.11 sequence state with consistent WPA3 lengths
35. Band-correct association metadata and operating classes
36. Target-bound SAE anti-clogging token handling
37. SD file-manager password redaction in serial diagnostics

General bug fixes and reusable features represented by this snapshot are maintained as separate focused pull requests. The snapshot remains a reproducibility artifact for the complete tested device image and is not intended to be merged wholesale into Marauder's main source tree.

The original per-commit mail export is intentionally not included: the repository stores CRLF source blobs, and replay through mail-patch normalization was not byte-identical. The combined binary diff above was reapplied to the recorded base and its resulting tree was verified exactly.
