from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class FreenoveFNK0104BHardwareTests(unittest.TestCase):
    def test_target_is_registered_in_stable_and_nightly_builds(self) -> None:
        target = next(
            target
            for target in json.loads(
                (ROOT / "installer" / "targets.json").read_text(encoding="utf-8")
            )["targets"]
            if target["buildFlag"] == "MARAUDER_FREENOVE_FNK0104B"
        )
        self.assertEqual(target["id"], "freenove-fnk0104b")
        self.assertEqual(target["assetSuffix"], "freenove_fnk0104b")
        self.assertEqual(target["chipFamily"], "ESP32-S3")
        self.assertEqual(target["esptoolChip"], "esp32s3")

        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(
                encoding="utf-8"
            )
            self.assertIn('flag: "MARAUDER_FREENOVE_FNK0104B"', workflow)
            self.assertIn("FlashSize=16M", workflow)
            self.assertIn("PartitionScheme=app3M_fat9M_16MB", workflow)
            self.assertIn("PSRAM=opi", workflow)
            self.assertIn("CDCOnBoot=cdc", workflow)
            self.assertIn('file_name: "freenove_fnk0104b"', workflow)
            self.assertIn('tft_file: "User_Setup_freenove_fnk01044b_28.h"', workflow)

    def test_board_contract_keeps_csi_disabled_and_gps_optional(self) -> None:
        config = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")
        feature_block = re.search(
            r"#ifdef MARAUDER_FREENOVE_FNK0104B(?P<body>.*?)#endif", config, re.S
        ).group("body")
        for expected in (
            "#define HAS_TOUCH",
            "#define HAS_CAP_TOUCH",
            "#define HAS_SCREEN",
            "#define HAS_SD",
            "#define USE_SD",
            "#define HAS_GPS",
            "#define HAS_PSRAM",
            "#define HAS_NIMBLE_2",
            "#define HAS_IDF_3",
        ):
            self.assertIn(expected, feature_block)
        self.assertNotIn("#define HAS_CSI_MOTION", feature_block)

        gps_block = re.search(
            r"#elif defined\(MARAUDER_FREENOVE_FNK0104B\)\s*"
            r"// GPS pins exposed.*?#endif",
            config,
            re.S,
        ).group(0)
        self.assertIn("module optional", gps_block)
        self.assertIn("#define GPS_SERIAL_INDEX 1", gps_block)
        self.assertIn("#define GPS_TX 43", gps_block)
        self.assertIn("#define GPS_RX 44", gps_block)

        touch_block = re.search(
            r"// FT6336U capacitive touch \(I2C\)(?P<body>.*?)#endif",
            config,
            re.S,
        ).group("body")
        for expected in (
            "#define TOUCH_SDA 16",
            "#define TOUCH_SCL 15",
            "#define TOUCH_INT 17",
            "#define TOUCH_RST 18",
        ):
            self.assertIn(expected, touch_block)

    def test_tft_setup_matches_config_and_uses_sdmmc(self) -> None:
        tft = (ROOT / "User_Setup_freenove_fnk01044b_28.h").read_text(encoding="utf-8")
        self.assertIn("#define ILI9341_DRIVER", tft)
        for pattern in (
            r"#define TFT_MISO\s+13",
            r"#define TFT_MOSI\s+11",
            r"#define TFT_SCLK\s+12",
            r"#define TFT_CS\s+10",
            r"#define TFT_DC\s+46",
            r"#define TFT_BL\s+45",
            r"#define TOUCH_CS\s+-1",
        ):
            self.assertRegex(tft, pattern)

        sd = (ROOT / "esp32_marauder" / "SDInterface.cpp").read_text(encoding="utf-8")
        self.assertIn("#ifdef MARAUDER_FREENOVE_FNK0104B", sd)
        self.assertIn("SD_MMC.setPins(38, 40, 39)", sd)
        self.assertIn('SD_MMC.begin("/sdcard", true)', sd)

        header = (ROOT / "esp32_marauder" / "SDInterface.h").read_text(encoding="utf-8")
        self.assertIn('#include "SD_MMC.h"', header)
        self.assertIn("#define SD SD_MMC", header)


if __name__ == "__main__":
    unittest.main()
