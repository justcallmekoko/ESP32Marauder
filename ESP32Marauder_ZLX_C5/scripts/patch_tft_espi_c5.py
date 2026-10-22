Import("env")

from pathlib import Path
import shutil
import urllib.request

ENV_NAME = env.subst("$PIOENV")
LIBDEPS = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / ENV_NAME
TFT_DIR = LIBDEPS / "TFT_eSPI"
PROC_DIR = TFT_DIR / "Processors"
VENDORED = Path(env.subst("$PROJECT_DIR")) / "scripts" / "tft_c5"

MIRROR = "https://cdn.jsdelivr.net/gh/BruceDevices/firmware@ac869d3d99ba222fd2fe7f76b707e4929385bd4c/lib/TFT_eSPI/Processors"
FILES = ["TFT_eSPI_ESP32_C5.h", "TFT_eSPI_ESP32_C5.c"]


def ensure_file(filename: str, target: Path) -> None:
    if target.exists() and target.stat().st_size > 1000:
        return
    target.parent.mkdir(parents=True, exist_ok=True)
    local = VENDORED / filename
    if not (local.exists() and local.stat().st_size > 1000):
        alt = VENDORED / (filename + ".txt")
        if alt.exists() and alt.stat().st_size > 1000:
            local = alt
    if local.exists() and local.stat().st_size > 1000:
        shutil.copyfile(local, target)
        print(f"[preprocess] copied bundled {filename}")
        return
    url = f"{MIRROR}/{filename}"
    print(f"[preprocess] download {filename}")
    with urllib.request.urlopen(url, timeout=30) as response:
        target.write_bytes(response.read())


def patch_text(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if new in text:
        return
    if old not in text:
        raise RuntimeError(f"Unexpected TFT_eSPI layout: {path}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")
    print(f"[preprocess] patched: {path.name}")


def apply_patch(*_args, **_kwargs) -> None:
    if not TFT_DIR.exists():
        print("[preprocess] TFT_eSPI not installed yet; retry after lib_deps")
        return

    for filename in FILES:
        ensure_file(filename, PROC_DIR / filename)

    patch_text(
        TFT_DIR / "TFT_eSPI.h",
        '#elif defined (ESP32)\n  #include "Processors/TFT_eSPI_ESP32.h"',
        '#elif defined(CONFIG_IDF_TARGET_ESP32C5)\n  #include "Processors/TFT_eSPI_ESP32_C5.h"\n#elif defined (ESP32)\n  #include "Processors/TFT_eSPI_ESP32.h"',
    )

    cpp_path = TFT_DIR / "TFT_eSPI.cpp"
    cpp = cpp_path.read_text(encoding="utf-8")
    marker = "#elif defined(CONFIG_IDF_TARGET_ESP32C5)"
    if marker not in cpp:
        old = '  #else\n    #include "Processors/TFT_eSPI_ESP32.c"'
        new = '  #elif defined(CONFIG_IDF_TARGET_ESP32C5)\n    #include "Processors/TFT_eSPI_ESP32_C5.c"\n  #else\n    #include "Processors/TFT_eSPI_ESP32.c"'
        if old not in cpp:
            raise RuntimeError(f"Unexpected TFT_eSPI.cpp layout: {cpp_path}")
        cpp_path.write_text(cpp.replace(old, new, 1), encoding="utf-8")
        print("[preprocess] patched: TFT_eSPI.cpp")


apply_patch()
env.AddPreAction("checkprogsize", apply_patch)
env.AddPreAction("$BUILD_DIR/src/main.cpp.o", apply_patch)
