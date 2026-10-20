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
        self.assertIn("Wire.requestFrom(0x08, 1)", keyboard)
        self.assertIn("not detected; using front buttons", keyboard)
        self.assertIn("raw_y >= 240 && raw_y < 280", display)
        self.assertIn("SELECT_BUTTON", display)


if __name__ == "__main__":
    unittest.main()
