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
        self.assertIn("#define T_DONGLE_LED_DATA_PIN 2", feature_block)
        self.assertIn("#define T_DONGLE_LED_CLOCK_PIN 6", feature_block)
        self.assertIn("#define T_DONGLE_SPI_SCLK_PIN 6", feature_block)
        self.assertIn("#define T_DONGLE_SPI_MISO_PIN 7", feature_block)
        self.assertIn("#define T_DONGLE_SPI_MOSI_PIN 2", feature_block)

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
        self.assertNotIn("SPI.begin", writer)

        header = (ROOT / "esp32_marauder" / "LedInterface.h").read_text()
        guarded_include = re.search(
            r"#ifdef HAS_T_DONGLE_LED\s+"
            r"#include <APA102.h>\s+"
            r"#endif",
            header,
        )
        self.assertIsNotNone(guarded_include)
        self.assertIn(
            "APA102<T_DONGLE_LED_DATA_PIN, T_DONGLE_LED_CLOCK_PIN> t_dongle_led",
            header,
        )
        self.assertIn("last_t_dongle_mode", header)

        sketch = (ROOT / "esp32_marauder" / "esp32_marauder.ino").read_text()
        final_writer = re.search(
            r"#elif defined\(HAS_T_DONGLE_LED\)(?P<body>.*?)"
            r"#elif defined\(HAS_NEOPIXEL_LED\)",
            sketch,
            re.S,
        ).group("body")
        self.assertIn("led_obj.refresh();", final_writer)
        self.assertLess(sketch.index("buffer_obj.save();"), sketch.index(final_writer))

        display_header = (ROOT / "esp32_marauder" / "TDongleDisplay.h").read_text()
        self.assertIn("bool update(uint32_t now, const WiFiScan& scan);", display_header)

        display_source = (ROOT / "esp32_marauder" / "TDongleDisplay.cpp").read_text()
        restore_index = display_source.index("SPI.begin(T_DONGLE_SPI_SCLK_PIN")
        draw_index = display_source.index('drawValue(0, "WiFi AP"')
        self.assertLess(restore_index, draw_index)

        for workflow_name in (
            "build_parallel.yml",
            "nightly_build.yml",
            "build_installer_manifests.yml",
        ):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text()
            install_step = re.search(
                r"- name: Install APA102 for LilyGo T-Dongle C5(?P<body>.*?)(?=\n\s+- name:)",
                workflow,
                re.S,
            ).group("body")
            self.assertIn("if: matrix.board.flag == 'MARAUDER_T_DONGLE_C5'", install_step)
            self.assertIn("repository: pololu/apa102-arduino", install_step)
            partition_step = re.search(
                r"- name: Configure LilyGo T-Dongle C5 partition table"
                r"(?P<body>.*?)(?=\n\s+- name:)",
                workflow,
                re.S,
            ).group("body")
            self.assertIn("if: matrix.board.flag == 'MARAUDER_T_DONGLE_C5'", partition_step)
            self.assertIn(
                "cp installer/partitions/t_dongle_c5.csv esp32_marauder/partitions.csv",
                partition_step,
            )

        self.assertFalse((ROOT / "esp32_marauder" / "partitions.csv").exists())
        self.assertTrue((ROOT / "installer" / "partitions" / "t_dongle_c5.csv").is_file())


if __name__ == "__main__":
    unittest.main()
