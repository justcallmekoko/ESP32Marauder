from __future__ import annotations

import unittest
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DISPLAY_SOURCE = REPOSITORY_ROOT / "esp32_marauder" / "Display.cpp"
SKETCH_SOURCE = REPOSITORY_ROOT / "esp32_marauder" / "esp32_marauder.ino"


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    opening_brace = source.index("{", start)
    depth = 0
    for index in range(opening_brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening_brace + 1 : index]
    raise AssertionError(f"unterminated function: {signature}")


class BrightnessRestoreTests(unittest.TestCase):
    def test_display_reinitialization_restores_backlight_state(self) -> None:
        body = function_body(
            DISPLAY_SOURCE.read_text(encoding="utf-8"), "void Display::init()"
        )

        self.assertLess(body.index("tft.init();"), body.index("backlightOn();"))
        self.assertNotIn("digitalWrite(TFT_BL, HIGH)", body)

    def test_pwm_backlight_restore_reattaches_ledc(self) -> None:
        source = SKETCH_SOURCE.read_text(encoding="utf-8")
        body = function_body(source, "void backlightOn()")

        self.assertLess(body.index("BL_SETUP();"), body.index("BL_SET("))


if __name__ == "__main__":
    unittest.main()
