import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class M5StackHardwareTests(unittest.TestCase):
    def test_board_contracts(self):
        config = (ROOT / "esp32_marauder/configs.h").read_text()
        for flag in ("MARAUDER_M5STACK_FIRE", "MARAUDER_M5STACK_CORE2_AWS"):
            self.assertIn(flag, config)
        for contract in (
            "#define U_BTN 39",
            "#define C_BTN 38",
            "#define D_BTN 37",
            "#define HAS_CORE2_BUTTONS",
            "#define GPS_TX 36",
            "#define GPS_RX 26",
            "#define GPS_TX 13",
            "#define GPS_RX 14",
            "#define HAS_FACES_KEYBOARD",
        ):
            self.assertIn(contract, config)

    def test_build_and_installer_registration(self):
        registry = json.loads((ROOT / "installer/targets.json").read_text())
        ids = {target["id"] for target in registry["targets"]}
        self.assertIn("m5stack-fire", ids)
        self.assertIn("m5stack-core2-aws", ids)
        for workflow in ("build_parallel.yml", "nightly_build.yml"):
            contents = (ROOT / ".github/workflows" / workflow).read_text()
            self.assertIn("User_Setup_m5stack_fire.h", contents)
            self.assertIn("User_Setup_m5stack_core2_aws.h", contents)

    def test_faces_and_core2_touch_paths(self):
        keyboard = (ROOT / "esp32_marauder/Keyboard.cpp").read_text()
        display = (ROOT / "esp32_marauder/Display.cpp").read_text()
        menu = (ROOT / "esp32_marauder/MenuFunctions.cpp").read_text()
        touch_keyboard = (ROOT / "esp32_marauder/TouchKeyboard.cpp").read_text()
        self.assertIn("Wire.requestFrom(0x08, 1)", keyboard)
        self.assertIn("not detected; using front buttons", keyboard)
        self.assertIn("raw_y >= 240 && raw_y < 280", display)
        self.assertIn("SELECT_BUTTON", display)
        self.assertIn("if (!core2_full_touch_input) return 0", display)
        self.assertIn("millis() - core2_front_button_seen < 60", display)
        for key in ("'k'", "'m'", "'n'", "'s'"):
            self.assertIn(f"isKeyPressed({key})", menu)
        self.assertIn("M5CardputerKeyboard.available()", menu)
        self.assertIn("setFullTouchInput(true)", touch_keyboard)
        self.assertIn("setFullTouchInput(false)", touch_keyboard)
        self.assertIn("display_obj.tft.width()", touch_keyboard)
        self.assertIn("display_obj.tft.height()", touch_keyboard)

    def test_m5_layout_reserves_status_and_title_rows(self):
        config = (ROOT / "esp32_marauder/configs.h").read_text()
        menu = (ROOT / "esp32_marauder/MenuFunctions.cpp").read_text()
        self.assertIn("#define STATUS_BAR_WIDTH 16", config)
        self.assertIn("#define KEY_Y 44", config)
        self.assertIn("!defined(MARAUDER_M5STACK_CORE2_AWS)", menu)

    def test_core2_power_rails_match_board_wiring(self):
        source = (ROOT / "esp32_marauder/esp32_marauder.ino").read_text()
        axp = (ROOT / "esp32_marauder/AXP192.cpp").read_text()
        self.assertIn("core2AxpWrite(0x28, 0xf2)", source)
        self.assertIn("core2AxpWrite(0x27", source)
        self.assertIn("core2AxpRead(0x12) & ~(1 << 3)", source)
        self.assertIn("outputs & ~(1 << 1)", source)
        self.assertIn("#define AXP192_WIRE Wire", axp)
        self.assertIn("legacy\n    // sequence below is for M5StickC", axp)

    def test_fire_display_uses_m5stack_safe_bus_contract(self):
        setup = (ROOT / "User_Setup_m5stack_fire.h").read_text()
        self.assertIn("#define TFT_BL   32", setup)
        self.assertIn("#define TFT_BACKLIGHT_ON HIGH", setup)
        self.assertIn("#define SPI_FREQUENCY       27000000", setup)
        self.assertIn("#define SPI_READ_FREQUENCY   5000000", setup)


if __name__ == "__main__":
    unittest.main()
