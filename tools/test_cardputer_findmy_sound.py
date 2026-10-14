#!/usr/bin/env python3
"""Guard Cardputer FindMy Sound build capabilities and CI dependencies."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class CardputerFindMySoundTests(unittest.TestCase):
    def test_cardputer_targets_enable_findmy_sound_capabilities(self) -> None:
        configs = (ROOT / "esp32_marauder" / "configs.h").read_text()
        cardputer_block = configs.split(
            "#if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)", 1
        )[1].split("#ifdef MARAUDER_MINI", 1)[0]

        self.assertIn("#define HAS_NIMBLE_2", cardputer_block)
        self.assertIn("#define HAS_IDF_3", cardputer_block)

    def test_cardputer_ci_uses_matching_core_and_nimble_versions(self) -> None:
        for workflow_name in ("build_parallel.yml", "nightly_build.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text()
            for board in ("MARAUDER_CARDPUTER", "MARAUDER_CARDPUTER_ADV"):
                line = next(
                    line
                    for line in workflow.splitlines()
                    if f'flag: "{board}"' in line
                )
                self.assertIn('idf_ver: "3.3.4"', line)
                self.assertIn('nimble_ver: "2.3.8"', line)

    def test_findmy_sound_menu_remains_available(self) -> None:
        menu = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        self.assertIn('"FindMy Sound"', menu)


if __name__ == "__main__":
    unittest.main()
