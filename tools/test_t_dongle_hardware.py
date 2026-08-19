import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class TDongleHardwareTests(unittest.TestCase):
    def test_t_dongle_enables_uart_gps_header(self):
        configs = (ROOT / "esp32_marauder" / "configs.h").read_text()
        feature_block = re.search(
            r"#ifdef MARAUDER_T_DONGLE_C5(?P<body>.*?)#endif", configs, re.S
        ).group("body")
        self.assertIn("#define HAS_GPS", feature_block)

        gps_block = next(
            body for body in re.findall(
                r"#elif defined\(MARAUDER_T_DONGLE_C5\)(.*?)#elif", configs, re.S
            ) if "GPS_SERIAL_INDEX" in body
        )
        self.assertIn("#define GPS_SERIAL_INDEX 1", gps_block)
        self.assertIn("#define GPS_TX 12", gps_block)
        self.assertIn("#define GPS_RX 11", gps_block)

    def test_t_dongle_led_uses_guarded_reference_library(self):
        source = (ROOT / "esp32_marauder" / "LedInterface.cpp").read_text()
        writer = re.search(
            r"void LedInterface::writeApa102Color\(.*?\n\}", source, re.S
        ).group(0)
        self.assertIn("t_dongle_led.startFrame()", writer)
        self.assertIn("t_dongle_led.sendColor(red, green, blue, brightness)", writer)
        self.assertIn("t_dongle_led.endFrame(1)", writer)
        self.assertIn("? 10 : 0", writer)

        header = (ROOT / "esp32_marauder" / "LedInterface.h").read_text()
        guarded_include = re.search(
            r"#ifdef HAS_T_DONGLE_LED\s+#include <APA102.h>\s+#endif", header
        )
        self.assertIsNotNone(guarded_include)
        self.assertIn("APA102<5, 4> t_dongle_led", header)
        self.assertIn("last_t_dongle_mode", header)

        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text()
            install_step = re.search(
                r"- name: Install APA102 for LilyGo T-Dongle C5(?P<body>.*?)(?=\n\s+- name:)",
                workflow,
                re.S,
            ).group("body")
            self.assertIn("if: matrix.board.flag == 'MARAUDER_T_DONGLE_C5'", install_step)
            self.assertIn("repository: pololu/apa102-arduino", install_step)


if __name__ == "__main__":
    unittest.main()
