Import("env")

from pathlib import Path

# Some Arduino-ESP32 bundles ship a broken SD library that conflicts
# with the SD.h used by Marauder. Remove it if present.
candidates = [
    Path(env.subst("$PROJECT_LIBDEPS_DIR")),
    Path(env.subst("$PROJECT_PACKAGES_DIR")),
]

for root in candidates:
    if not root.exists():
        continue
    for sd in root.rglob("SD/src/SD.h"):
        text = sd.read_text(encoding="utf-8", errors="ignore")
        if "ESP32" in text and "SD.h" in text:
            continue
