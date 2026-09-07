#!/usr/bin/env bash
#
# Local build for the Waveshare ESP32-S3-Touch-LCD-3.49 Marauder target.
#
# Mirrors the steps the GitHub CI runs for this board in
# .github/workflows/build_parallel.yml (matrix row "Waveshare S3 Touch LCD 3.49"):
# same arduino-esp32 core, same pinned third-party libraries, same platform.txt
# tweaks, same FQBN and -D build flag.
#
# By default everything (core, toolchain, libraries, build output) lives in an
# isolated sandbox so it never touches an existing Arduino / arduino-cli setup.
# First run downloads the esp32 core + toolchain (~2 GB) into that sandbox.
#
# Usage:
#   tools/build_waveshare_s3_349.sh                 # compile only
#   tools/build_waveshare_s3_349.sh -p /dev/ttyACM0 # compile, then upload
#   tools/build_waveshare_s3_349.sh --refresh-libs  # re-clone the pinned libs
#   tools/build_waveshare_s3_349.sh --global        # use your normal arduino-cli setup
#   tools/build_waveshare_s3_349.sh --sandbox DIR   # put the sandbox somewhere else
#
# Output (sandbox mode): $SANDBOX/out/esp32_marauder.ino.bin
#                        $SANDBOX/out/esp32_marauder.ino.merged.bin  (flash @ 0x0)
#
set -euo pipefail

# ---------------------------------------------------------------------------
# Board / build definition -- keep in sync with the CI matrix row.
# ---------------------------------------------------------------------------
BUILD_FLAG="MARAUDER_WAVESHARE_S3_349"
FQBN="esp32:esp32:esp32s3:PartitionScheme=min_spiffs,FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc"
CORE_VERSION="3.3.4"
CORE_INDEX_URL="https://github.com/espressif/arduino-esp32/releases/download/${CORE_VERSION}/package_esp32_dev_index.json"
NIMBLE_VERSION="2.3.8"

# name in <user>/libraries  |  github repo  |  git ref (tag/branch)
LIBS=(
  "ESP32Ping|marian-craciunescu/ESP32Ping|1.6"
  "AsyncTCP|ESP32Async/AsyncTCP|v3.4.8"
  "MicroNMEA|stevemarple/MicroNMEA|v2.0.6"
  "ESPAsyncWebServer|ESP32Async/ESPAsyncWebServer|v3.8.1"
  "TFT_eSPI|Bodmer/TFT_eSPI|V2.5.34"
  "XPT2046_Touchscreen|PaulStoffregen/XPT2046_Touchscreen|v1.4"
  "lv_arduino|lvgl/lv_arduino|3.0.0"
  "JPEGDecoder|Bodmer/JPEGDecoder|1.8.0"
  "NimBLE-Arduino|h2zero/NimBLE-Arduino|${NIMBLE_VERSION}"
  "Adafruit_NeoPixel|adafruit/Adafruit_NeoPixel|1.12.0"
  "ArduinoJson|bblanchon/ArduinoJson|v6.18.2"
  "LinkedList|ivanseidel/LinkedList|v1.3.3"
  "EspSoftwareSerial|plerup/espsoftwareserial|8.1.0"
  "Adafruit_BusIO|adafruit/Adafruit_BusIO|1.15.0"
  "Adafruit_MAX1704X|adafruit/Adafruit_MAX1704X|1.0.2"
)

# ---------------------------------------------------------------------------
# Args
# ---------------------------------------------------------------------------
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SANDBOX="${MBUILD_DIR:-${XDG_CACHE_HOME:-$HOME/.cache}/esp32marauder-waveshare-s3-349}"
USE_GLOBAL=0
REFRESH_LIBS=0
PORT=""

while [ $# -gt 0 ]; do
  case "$1" in
    -p|--port)        PORT="${2:?--port needs a value}"; shift 2 ;;
    --sandbox)        SANDBOX="${2:?--sandbox needs a value}"; shift 2 ;;
    --global)         USE_GLOBAL=1; shift ;;
    --refresh-libs)   REFRESH_LIBS=1; shift ;;
    -h|--help)        sed -n '3,22p' "$0" | sed 's/^#\{0,1\} \{0,1\}//'; exit 0 ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

log() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }

# Pinned libraries always live in their own dir, isolated from any Arduino setup,
# so this never overwrites libraries you already have installed.
mkdir -p "$SANDBOX/libraries" "$SANDBOX/out" "$SANDBOX/build"
LIB_DIR="$SANDBOX/libraries"

# ---------------------------------------------------------------------------
# arduino-cli + esp32 core: sandboxed by default, system-wide with --global.
# --global reuses whatever core/toolchain you already have; sandbox mode
# downloads its own (~2 GB, first run only) and touches nothing else.
# ---------------------------------------------------------------------------
if [ "$USE_GLOBAL" -eq 1 ]; then
  command -v arduino-cli >/dev/null 2>&1 || { echo "arduino-cli not on PATH (needed with --global)" >&2; exit 1; }
  ACLI=(arduino-cli)
  CONFIG_ARGS=()
  DATA_DIR="$(arduino-cli config get directories.data 2>/dev/null || echo "$HOME/.arduino15")"
else
  mkdir -p "$SANDBOX/bin" "$SANDBOX/data" "$SANDBOX/downloads"
  CONFIG_FILE="$SANDBOX/arduino-cli.yaml"
  cat > "$CONFIG_FILE" <<YAML
directories:
  data: $SANDBOX/data
  downloads: $SANDBOX/downloads
  user: $SANDBOX/user
board_manager:
  additional_urls:
    - $CORE_INDEX_URL
YAML
  if [ ! -x "$SANDBOX/bin/arduino-cli" ] && ! command -v arduino-cli >/dev/null 2>&1; then
    log "Installing arduino-cli into $SANDBOX/bin"
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$SANDBOX/bin" sh
  fi
  if [ -x "$SANDBOX/bin/arduino-cli" ]; then ACLI=("$SANDBOX/bin/arduino-cli"); else ACLI=(arduino-cli); fi
  CONFIG_ARGS=(--config-file "$CONFIG_FILE")
  DATA_DIR="$SANDBOX/data"
fi

# CONFIG_ARGS may be empty; guard the expansion for macOS's bash 3.2 + `set -u`.
acli() { "${ACLI[@]}" ${CONFIG_ARGS[@]+"${CONFIG_ARGS[@]}"} "$@"; }

log "arduino-cli: $("${ACLI[@]}" version)"

# ---------------------------------------------------------------------------
# Core
# ---------------------------------------------------------------------------
log "Installing esp32:esp32@${CORE_VERSION}"
acli core update-index --additional-urls "$CORE_INDEX_URL"
if acli core list | awk '{print $1"@"$2}' | grep -qx "esp32:esp32@${CORE_VERSION}"; then
  echo "already installed"
else
  acli core install "esp32:esp32@${CORE_VERSION}" --additional-urls "$CORE_INDEX_URL"
fi

# ---------------------------------------------------------------------------
# Libraries (pinned clones, matching CI). Sketch-bundled esp32_marauder/libraries
# is stale and unused -- CI clones these fresh, so do we.
# ---------------------------------------------------------------------------
for entry in "${LIBS[@]}"; do
  IFS='|' read -r name repo ref <<< "$entry"
  dest="$LIB_DIR/$name"
  if [ "$REFRESH_LIBS" -eq 1 ] && [ -d "$dest" ]; then rm -rf "$dest"; fi
  if [ -d "$dest" ]; then
    echo "  have $name ($ref)"
  else
    log "Cloning $name @ $ref"
    git clone --depth 1 --branch "$ref" "https://github.com/$repo" "$dest" \
      || git clone --depth 1 "https://github.com/$repo" "$dest"  # fallback if ref isn't a branch/tag
  fi
done
# Adafruit_TCA8418 is vendored in the repo, not fetched.
rm -rf "$LIB_DIR/Adafruit_TCA8418"
cp -r "$REPO_ROOT/libraries/Adafruit_TCA8418" "$LIB_DIR/Adafruit_TCA8418"

# ---------------------------------------------------------------------------
# platform.txt / cpp_flags tweaks the CI applies for core 3.3.4
#   - link with -Wl,-zmuldefs  (tolerate duplicate symbols from the lib forks)
#   - build the core libs with -fno-exceptions
# Idempotent: only patched once.
# ---------------------------------------------------------------------------
log "Patching platform.txt for core ${CORE_VERSION}"
while IFS= read -r -d '' pt; do
  if ! grep -q 'zmuldefs' "$pt"; then
    sed -i.bak 's/compiler.c.elf.extra_flags=/compiler.c.elf.extra_flags=-Wl,-zmuldefs /' "$pt" && rm -f "$pt.bak"
    echo "  patched $pt"
  fi
done < <(find "$DATA_DIR/packages/esp32/hardware/esp32" -name platform.txt -print0 2>/dev/null)

while IFS= read -r -d '' cf; do
  if grep -q -- '-fexceptions' "$cf"; then
    sed -i.bak 's/-fexceptions/-fno-exceptions/g' "$cf" && rm -f "$cf.bak"
    echo "  patched $cf"
  fi
done < <(find "$DATA_DIR/packages/esp32/tools/esp32-arduino-libs" -name cpp_flags -print0 2>/dev/null)

# ---------------------------------------------------------------------------
# Compile
# ---------------------------------------------------------------------------
COMPILE_ARGS=(
  compile
  --fqbn "$FQBN"
  --build-property "compiler.cpp.extra_flags=-D${BUILD_FLAG}"
  --warnings none
  --libraries "$LIB_DIR"
  --output-dir "$SANDBOX/out"
  --build-path "$SANDBOX/build"
)
[ -n "$PORT" ] && COMPILE_ARGS+=(--upload --port "$PORT")

log "Compiling $BUILD_FLAG"
acli "${COMPILE_ARGS[@]}" "$REPO_ROOT/esp32_marauder"

# ---------------------------------------------------------------------------
# Done
# ---------------------------------------------------------------------------
OUT="$SANDBOX/out"
log "Build OK"
echo "Artifacts in: $OUT"
ls -1 "$OUT" 2>/dev/null | sed 's/^/  /' || true
cat <<EOF

Flash the app bin with arduino-cli:
  ${ACLI[*]} ${CONFIG_ARGS[*]-} upload --fqbn "$FQBN" --port <PORT> --input-dir "$OUT" "$REPO_ROOT/esp32_marauder"

...or the merged image with esptool (ESP32-S3 bootloader lives at 0x0):
  esptool.py --chip esp32s3 --port <PORT> --baud 921600 write_flash 0x0 "$OUT"/esp32_marauder.ino.merged.bin
EOF
