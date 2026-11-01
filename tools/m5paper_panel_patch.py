"""Latch IT8951 I/O failures in the project's pinned M5GFX dependency.

The pinned driver bounds each HRDY wait but ignores some failures while
uploading images. A stuck panel could therefore repeat that wait for every
row and later publish a failed frame as successful. This small, hash-checked
patch aborts the upload and exposes the error to M5PaperDisplay. Only this
environment's .pio/libdeps copy is changed; upstream license notices remain.
"""
from hashlib import sha256
from pathlib import Path


ORIGINAL_HASH = {
    "Panel_IT8951.hpp": "f14a2de0473dd9b569696864937f19df2ce818e75dced3c5befd74db04090796",
    "Panel_IT8951.inl": "98baa1fcd50edb59a8f91b50538b83b9b788b517f5dca49b03f1e9d029d0208d",
}
PATCHED_HASH = {
    "Panel_IT8951.hpp": "fc73ced94d0d29a8b501a998599783bffd036b499575b843bbd4b22f23a2d67f",
    "Panel_IT8951.inl": "10a2ad2124d87462d3ebd262a8f15769c6ec84b801532cd760f302a59ac560bc",
}

REPLACEMENTS = {
    "Panel_IT8951.hpp": [
        ("    void setVCOM(uint16_t vcom);\n",
         "    void setVCOM(uint16_t vcom);\n\n"
         "    void clearIoError(void) { _io_error = false; }\n"
         "    bool hasIoError(void) const { return _io_error; }\n"),
        ("    bool _in_transaction = false;\n",
         "    bool _in_transaction = false;\n    bool _io_error = false;\n"),
    ],
    "Panel_IT8951.inl": [
        ("  bool Panel_IT8951::init(bool use_reset)\n  {\n",
         "  bool Panel_IT8951::init(bool use_reset)\n  {\n    _io_error = false;\n"),
        ("      uint16_t buf[20];\n", "      uint16_t buf[20] = {};\n"),
        ("    return true;\n  }\n\n  uint16_t Panel_IT8951::getVCOM",
         "    return !_io_error;\n  }\n\n  uint16_t Panel_IT8951::getVCOM"),
        ("  bool Panel_IT8951::_wait_busy_pin(uint32_t timeout)\n  {\n",
         "  bool Panel_IT8951::_wait_busy_pin(uint32_t timeout)\n  {\n"
         "    if (_io_error) return false;\n"),
        ('            ESP_LOGW("Panel_IT8951", "wait_busy: timeout");\n#endif\n            return false;',
         '            ESP_LOGW("Panel_IT8951", "wait_busy: timeout");\n#endif\n'
         '            _io_error = true;\n            return false;'),
        ("      if (infobuf[0] == 65535) break; // IT8951 crashed\n",
         "      if (_io_error || infobuf[0] == 65535) break; // IT8951 I/O failure\n"),
        ("    cs_control(true);\n    return infobuf[0] != 0;\n",
         "    cs_control(true);\n    if (infobuf[0] != 0) _io_error = true;\n"
         "    return infobuf[0] != 0;\n"),
        ("    bgr888_t* readbuf = reinterpret_cast<lgfx::bgr888_t*>(&writebuf[2]);\n\n"
         "    if (writebuf == nullptr) return;\n",
         "    if (writebuf == nullptr) { _io_error = true; return; }\n"
         "    bgr888_t* readbuf = reinterpret_cast<lgfx::bgr888_t*>(&writebuf[2]);\n"),
        ("      _set_area(x, y, w, h);\n      flg_setarea = true;\n      _wait_busy();\n",
         "      if (!_set_area(x, y, w, h) || !_wait_busy())\n"
         "      { heap_free(writebuf); return; }\n      flg_setarea = true;\n"),
        ("            _set_area(x + prev_pos, y, new_pos - prev_pos, 1);\n",
         "            if (!_set_area(x + prev_pos, y, new_pos - prev_pos, 1))\n"
         "            { heap_free(writebuf); return; }\n"),
        ("          _wait_busy();\n          _bus->writeBytes((uint8_t*)writebuf, writepos << 1, true, false);\n",
         "          if (!_wait_busy()) { heap_free(writebuf); return; }\n"
         "          _bus->writeBytes((uint8_t*)writebuf, writepos << 1, true, false);\n"),
    ],
}


def digest(text):
    return sha256(text.encode("utf-8")).hexdigest()


def transform(name, text):
    for before, after in REPLACEMENTS[name]:
        if text.count(before) != 1:
            raise RuntimeError(f"M5Paper driver patch context changed: {name}")
        text = text.replace(before, after, 1)
    return text


def patched_text(name, text):
    checksum = digest(text)
    if checksum == PATCHED_HASH[name]:
        return text
    if checksum != ORIGINAL_HASH[name]:
        raise RuntimeError(f"M5Paper requires the pinned M5GFX IT8951 source: {name}")
    patched = transform(name, text)
    if digest(patched) != PATCHED_HASH[name]:
        raise RuntimeError(f"M5Paper driver patch output mismatch: {name}")
    return patched


def patch_folder(folder):
    # Validate both files before changing either; unknown dependency edits fail.
    updates = []
    for name in ORIGINAL_HASH:
        path = folder / name
        text = path.read_text(encoding="utf-8")
        patched = patched_text(name, text)
        if patched != text:
            updates.append((path, patched))
    for path, text in updates:
        path.write_text(text, encoding="utf-8", newline="\n")


if "Import" in globals():
    Import("env")
    if env.subst("$PIOENV") != "m5paper_v11":
        raise RuntimeError("IT8951 patch is specific to m5paper_v11")
    dependency_root = Path(env.subst("$PROJECT_LIBDEPS_DIR")).resolve()
    folder = (dependency_root / env.subst("$PIOENV") / "M5GFX" /
              "src/lgfx/v1/panel").resolve()
    if not folder.is_relative_to(dependency_root):
        raise RuntimeError("M5GFX path is outside the project's dependency directory")
    patch_folder(folder)
    print("M5Paper: pinned IT8951 driver has checked, latched I/O failures")
