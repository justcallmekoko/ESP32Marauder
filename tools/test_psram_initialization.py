#!/usr/bin/env python3
"""Regression checks for target-aware PSRAM initialization."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SKETCH = ROOT / "esp32_marauder/esp32_marauder.ino"


class PsramInitializationTests(unittest.TestCase):
    def test_low_level_psram_initializer_is_not_called_directly(self):
        source = SKETCH.read_text(encoding="utf-8")
        self.assertNotIn("esp_spiram_init()", source)

    def test_arduino_psram_initializer_is_guarded_by_board_capability(self):
        source = SKETCH.read_text(encoding="utf-8")
        expected = """#ifdef HAS_PSRAM
    if (!psramInit()) {
      Serial.println(F("PSRAM not available"));
    }
  #endif"""
        self.assertIn(expected, source)


if __name__ == "__main__":
    unittest.main()
