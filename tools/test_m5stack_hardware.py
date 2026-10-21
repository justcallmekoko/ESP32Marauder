import json
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class M5StackHardwareTests(unittest.TestCase):
    def test_board_contracts(self):
        config = (ROOT / "esp32_marauder/configs.h").read_text()
        self.assertIn("MARAUDER_M5STACK_CORE2_AWS", config)
        self.assertNotIn("MARAUDER_M5STACK_FIRE", config)
        for contract in (
            "#define HAS_CORE2_BUTTONS",
            "#define GPS_TX 13",
            "#define GPS_RX 14",
            "#define HAS_FACES_KEYBOARD",
        ):
            self.assertIn(contract, config)

    def test_build_and_installer_registration(self):
        registry = json.loads((ROOT / "installer/targets.json").read_text())
        ids = {target["id"] for target in registry["targets"]}
        self.assertNotIn("m5stack-fire", ids)
        self.assertIn("m5stack-core2-aws", ids)
        for workflow in ("build_parallel.yml", "nightly_build.yml"):
            contents = (ROOT / ".github/workflows" / workflow).read_text()
            self.assertNotIn("m5stack_fire", contents)
            self.assertIn("User_Setup_m5stack_core2_aws.h", contents)

    def test_faces_and_core2_touch_paths(self):
        keyboard = (ROOT / "esp32_marauder/Keyboard.cpp").read_text()
        display = (ROOT / "esp32_marauder/Display.cpp").read_text()
        menu = (ROOT / "esp32_marauder/MenuFunctions.cpp").read_text()
        touch_keyboard = (ROOT / "esp32_marauder/TouchKeyboard.cpp").read_text()
        wifi_scan = (ROOT / "esp32_marauder/WiFiScan.cpp").read_text()
        recon = (ROOT / "esp32_marauder/ReconMission.cpp").read_text()
        self.assertIn("Wire.requestFrom(0x08, 1)", keyboard)
        self.assertIn("not detected; using front buttons", keyboard)
        self.assertIn("raw_y >= 240 && raw_y < 280", display)
        self.assertIn("SELECT_BUTTON", display)
        self.assertIn("if (!core2_full_touch_input) return 0", display)
        self.assertIn("touch_state < 0", display)
        self.assertIn("millis() - core2_front_button_seen < 250", display)
        self.assertIn("core2_release_debounce_ms = 200", menu)
        self.assertIn("menu_touch_up_repeat.updateDebounced", menu)
        self.assertIn("menu_touch_down_repeat.updateDebounced", menu)
        self.assertIn("MenuInputRepeat menu_touch_up_repeat", (ROOT / "esp32_marauder/MenuFunctions.h").read_text())
        for key in ("'k'", "'m'", "'n'", "'s'"):
            self.assertIn(f"isKeyPressed({key})", menu)
        self.assertIn("marauder_keyboard.available()", menu)
        self.assertIn("setFullTouchInput(true)", touch_keyboard)
        self.assertIn("setFullTouchInput(false)", touch_keyboard)
        self.assertIn("display_obj.tft.width()", touch_keyboard)
        self.assertIn("display_obj.tft.height()", touch_keyboard)
        self.assertIn("key == KEY_ENTER) return EXIT_BUTTON_INDEX", wifi_scan)
        self.assertIn("core2_button == SELECT_BUTTON", wifi_scan)
        self.assertIn("core2_button == UP_BUTTON) return CHAN_MINUS_INDEX", wifi_scan)
        self.assertIn("core2_button == DOWN_BUTTON) return CHAN_PLUS_INDEX", wifi_scan)
        self.assertIn("defined(MARAUDER_M5STACK_CORE2_AWS)", recon)
        self.assertIn("fillRect(0,16,SCREEN_WIDTH,16", wifi_scan)

    def test_m5_layout_reserves_status_and_title_rows(self):
        config = (ROOT / "esp32_marauder/configs.h").read_text()
        menu = (ROOT / "esp32_marauder/MenuFunctions.cpp").read_text()
        self.assertIn("#define STATUS_BAR_WIDTH 16", config)
        self.assertIn("#define KEY_Y 44", config)
        self.assertIn("!defined(MARAUDER_M5STACK_CORE2_AWS)", menu)

    def test_fire_port_is_fully_removed(self):
        self.assertFalse((ROOT / "User_Setup_m5stack_fire.h").exists())
        for path in (
            ROOT / "esp32_marauder/configs.h",
            ROOT / "installer/targets.json",
            ROOT / ".github/workflows/build_parallel.yml",
            ROOT / ".github/workflows/nightly_build.yml",
        ):
            self.assertNotIn("M5STACK_FIRE", path.read_text())

    def test_core2_power_rails_match_board_wiring(self):
        source = (ROOT / "esp32_marauder/esp32_marauder.ino").read_text()
        axp = (ROOT / "esp32_marauder/AXP192.cpp").read_text()
        self.assertIn("core2AxpWrite(0x28, 0xf2)", source)
        self.assertIn("core2AxpWrite(0x27", source)
        self.assertIn("core2AxpRead(0x12) & ~(1 << 3)", source)
        self.assertIn("outputs & ~(1 << 1)", source)
        self.assertIn("#define AXP192_WIRE Wire", axp)
        self.assertIn("legacy\n    // sequence below is for M5StickC", axp)

if __name__ == "__main__":
    unittest.main()
