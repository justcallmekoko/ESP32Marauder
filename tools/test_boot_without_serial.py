#!/usr/bin/env python3
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class BootWithoutSerialTests(unittest.TestCase):
    def test_setup_does_not_wait_for_a_serial_client(self) -> None:
        sketch = (ROOT / "esp32_marauder" / "esp32_marauder.ino").read_text(
            encoding="utf-8"
        )
        setup = sketch[sketch.index("void setup()") : sketch.index("void loop()")]
        self.assertNotIn("while(!Serial)", setup)
        self.assertNotIn("while (!Serial)", setup)

    def test_cardputer_targets_use_a_dedicated_hspi_port(self) -> None:
        for name in (
            "User_Setup_marauder_m5cardputer.h",
            "User_Setup_marauder_m5cardputer_adv.h",
        ):
            setup = (ROOT / name).read_text(encoding="utf-8")
            self.assertIn("#define USE_HSPI_PORT", setup)
            self.assertNotIn("#define USE_FSPI_PORT", setup)

    def test_xiao_nonblocking_startup_restores_runtime_cdc_timeouts(self) -> None:
        sketch = (ROOT / "esp32_marauder" / "esp32_marauder.ino").read_text(
            encoding="utf-8"
        )
        setup = sketch[sketch.index("void setup()") : sketch.index("void loop()")]
        self.assertLess(
            setup.index("Serial.setTxTimeoutMs(0);"),
            setup.index("Serial.begin(115200);"),
        )
        cli_ready = setup.index("cli_obj.RunSetup();")
        self.assertLess(cli_ready, setup.index("Serial.setTxTimeoutMs(100);"))
        self.assertLess(cli_ready, setup.index("Serial.setTxTimeoutMs(250);"))
        self.assertRegex(
            setup[cli_ready:],
            r"(?s)#if defined\(XIAO_ESP32_S3_TOUCH\) && ARDUINO_USB_CDC_ON_BOOT"
            r".*?#if ARDUINO_USB_MODE\s*"
            r"Serial\.setTxTimeoutMs\(100\);[^\n]*\n\s*#else\s*"
            r"Serial\.setTxTimeoutMs\(250\);[^\n]*\n\s*#endif\s*#endif\s*}\s*$",
        )


if __name__ == "__main__":
    unittest.main()
