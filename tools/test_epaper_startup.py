"""Regression contracts for unattended boot of the two e-paper boards."""

import configparser
import pathlib
import re
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class EpaperStartupTests(unittest.TestCase):
    def setUp(self):
        source = (ROOT / "esp32_marauder/esp32_marauder.ino").read_text(encoding="utf-8")
        self.setup = source.split("void setup()", 1)[1].split("void loop()", 1)[0]

    def test_power_latch_precedes_serial_and_peripheral_initialization(self):
        latch = self.setup.index("TFT_eSPI::prepareBoard();")
        for operation in ("randomSeed(", "Serial.begin(", "sharedSPI.begin(",
                          "display_obj.RunSetup();", "settings_obj.begin();"):
            self.assertLess(latch, self.setup.index(operation), operation)

    def test_boot_does_not_wait_for_a_serial_host_or_input(self):
        # A host-connect or input wait here makes power-bank/battery boot stall.
        setup = re.sub(r"//[^\n]*|/\*.*?\*/", "", self.setup, flags=re.S)
        self.assertNotRegex(setup, r"\b(?:while|for)\s*\([^)]*\bSerial\b")
        self.assertNotIn("Serial.available(", setup)
        self.assertNotIn("Serial.flush(", setup)
        self.assertNotIn("Serial.read", setup)

    def test_pinned_builds_use_the_boards_uart_bridge(self):
        config = configparser.ConfigParser(interpolation=None)
        config.read(ROOT / "platformio.ini", encoding="utf-8")
        self.assertIn("-DARDUINO_USB_CDC_ON_BOOT=0",
                      config["env:reterminal_sticky"]["build_flags"])
        # Original M5Paper uses a classic ESP32 and an external USB/UART bridge.
        self.assertEqual(config["env:m5paper_v11"]["board"], "esp32dev")
        self.assertNotIn("-DARDUINO_USB_CDC_ON_BOOT=1",
                         config["env:m5paper_v11"]["build_flags"])

    def test_either_target_selection_method_enables_epaper_safety_paths(self):
        source = (ROOT / "esp32_marauder/configs.h").read_text(encoding="utf-8")
        for target in ("MARAUDER_RETERMINAL_STICKY", "MARAUDER_M5PAPER"):
            for manual in (False, True):
                with self.subTest(target=target, manual=manual):
                    selected = source.replace("//#define " + target,
                                              "#define " + target, 1) if manual else source
                    command = ["g++", "-E", "-dM", "-x", "c++",
                               "-I", str(ROOT / "esp32_marauder")]
                    if not manual:
                        command.append("-D" + target)
                    result = subprocess.run(command + ["-"], input=selected,
                                            text=True, capture_output=True, check=True)
                    self.assertIn("#define MARAUDER_EPAPER_UI", result.stdout)
                    self.assertIn("#define HAS_SCREEN", result.stdout)

    def test_packet_monitor_keeps_a_physical_exit(self):
        source = (ROOT / "esp32_marauder/WiFiScan.cpp").read_text(encoding="utf-8")
        analyzer = source.split("int8_t WiFiScan::checkAnalyzerButtons(", 1)[1]
        analyzer = analyzer.split("boolean pressed", 1)[0]
        self.assertIn("#ifdef MARAUDER_EPAPER_UI", analyzer)
        self.assertIn("if (epaper::takePowerSelect()) return EXIT_BUTTON_INDEX;", analyzer)

    def test_manual_selection_reaches_physical_switch_implementation(self):
        config = (ROOT / 'esp32_marauder/configs.h').read_text(encoding='utf-8')
        for target in ('MARAUDER_RETERMINAL_STICKY', 'MARAUDER_M5PAPER'):
            with self.subTest(target=target), tempfile.TemporaryDirectory() as directory:
                folder = pathlib.Path(directory)
                for name in ('Switches.cpp', 'Switches.h', 'EpaperPower.h',
                             'M5PaperPower.h', 'StickyPower.h'):
                    (folder / name).write_text((ROOT / 'esp32_marauder' / name)
                                               .read_text(encoding='utf-8'), encoding='utf-8')
                (folder / 'configs.h').write_text(config.replace('//#define ' + target,
                                                               '#define ' + target, 1),
                                                encoding='utf-8')
                result = subprocess.run(['g++', '-E', '-P', '-DARDUINO=10819',
                                         '-I', str(ROOT / 'test/support'),
                                         '-I', str(ROOT / 'esp32_marauder'),
                                         str(folder / 'Switches.cpp')],
                                        check=True, capture_output=True, text=True)
                self.assertIn('epaper::servicePowerButton();', result.stdout)
                self.assertIn('epaper::navigationDown(this->pin)', result.stdout)


if __name__ == "__main__":
    unittest.main()
