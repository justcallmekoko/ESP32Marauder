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

    def test_t_dongle_led_does_not_send_white_end_frame(self):
        source = (ROOT / "esp32_marauder" / "LedInterface.cpp").read_text()
        writer = re.search(
            r"void LedInterface::writeApa102Color\(.*?\n\}", source, re.S
        ).group(0)
        self.assertNotIn("writeApa102Byte(0xFF)", writer)
        self.assertIn("? 8 : 0", writer)
        self.assertIn("writeApa102Byte(0xE0 | brightness)", writer)
        self.assertIn("digitalWrite(5, LOW)", writer)
        self.assertIn("digitalWrite(4, LOW)", writer)

        header = (ROOT / "esp32_marauder" / "LedInterface.h").read_text()
        self.assertIn("last_t_dongle_mode", header)


if __name__ == "__main__":
    unittest.main()
