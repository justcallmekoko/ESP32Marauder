# Rebuilding the tested firmware

## Requirements

- Git
- PowerShell 7 or Windows PowerShell 5.1
- Arduino CLI
- Espressif Arduino core `esp32:esp32` version `3.3.5` (includes the upstream ESP32-C5 MSPI soft-reset fix)
- network access for the first download of the pinned libraries

Install the core once:

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32@3.3.5
```

## 1. Use a disposable source worktree

The package intentionally does not modify the repository source. Keep the package branch checked out and create a sibling worktree at the recorded base:

```powershell
$packageRoot = (Resolve-Path './contrib/waveshare-esp32-c5-35').Path
$sourceWorktree = Join-Path (Split-Path (Get-Location).Path -Parent) 'ESP32Marauder-waveshare-c5-source'
git worktree add --detach $sourceWorktree 9f6c10dbce99efe3b2ff56f5805a0f2036b67fdf
git -C $sourceWorktree apply --index --binary (Join-Path $packageRoot 'source/waveshare-c5-tested-snapshot.patch')
git -C $sourceWorktree write-tree
```

The final command must print tree `adb2fa1bb9ba49d8a662bedc0ca0d1bc3e1305f9`. It is the hardware-tested tree from commit `f401e813276d8ebb3d660a314776572139b1146a` plus one publication-only diagnostic change: the runtime-generated SD file-manager password is redacted from serial output.

## 2. Compile

Run the package helper against that disposable source tree:

```powershell
& (Join-Path $packageRoot 'tools/build-waveshare-c5.ps1') -RepositoryRoot $sourceWorktree
```

The helper clones the library revisions in `source/dependency-lock.txt` into the package-local ignored `.build/libraries` directory, selects the supplied TFT profile in that private library copy, and compiles with:

```text
esp32:esp32:esp32c5:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled
-DMARAUDER_WAVESHARE_C5 -fno-exceptions
-Wl,-zmuldefs
```

It also supplies GCC file/macro prefix maps for C and C++ inputs. Absolute host
roots are replaced by stable relative labels such as `package`, `source`, and
`arduino`, preventing usernames and workstation paths from being retained in
published `__FILE__` diagnostic strings.

Expected reference result:

- sketch: 2,109,718 bytes
- global variables: 67,044 bytes
- application BIN: 2,109,872 bytes
- application SHA-256: `64307554007412dd8e4d480e27310024909879ed1697107765d493dd907eaac2`

Byte-identical output can depend on the complete toolchain and host environment. A successful build with the pinned core and dependency commits remains the primary reproducibility requirement.

The prebuilt files under `firmware/` are the path-sanitized 3.3.5 artifacts
recorded in `firmware/BUILD-MANIFEST.json`.

When finished, remove only the disposable worktree you created:

```powershell
git worktree remove $sourceWorktree
```

## Arduino IDE profile

`profile/boards.local.fragment.txt` defines the selectable board entry `Waveshare ESP32-C5 Marauder (3.5in Touch)`. The optional installer only manages its marked block in the selected core's `boards.local.txt` and creates a backup first:

```powershell
./contrib/waveshare-esp32-c5-35/tools/install-board-profile.ps1
```

The profile does not install libraries or apply the source snapshot. It also changes cached Arduino IDE DTR/RTS selections to `off` only for existing `esp32:esp32:waveshare_c5_marauder-*-serial` entries. A timestamped cache backup is created before a material change; unrelated boards remain unchanged, and missing or malformed cache data does not block profile installation. Restart Arduino IDE after installation so it reloads both the profile and monitor settings. This migration prevents the known IDE-cache override but cannot guarantee reset-free port opening with every USB driver or serial client.

For an IDE build, use the H4W9/TFT_eSPI `ESP32-C5` revision recorded in `source/dependency-lock.txt`, copy `profile/User_Setup_marauder_waveshare_c5.h` into that library, and select it from `User_Setup_Select.h`.

Run the isolated installer checks with:

```powershell
./contrib/waveshare-esp32-c5-35/tools/test-install-board-profile.ps1
```
