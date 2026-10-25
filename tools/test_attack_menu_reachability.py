#!/usr/bin/env python3
"""Guard the guided attack menu from dropping an existing attack path."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
MENU = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text(encoding="utf-8")


class AttackMenuReachabilityTests(unittest.TestCase):
    def test_every_existing_wifi_attack_mode_remains_reachable(self):
        expected_guided_modes = {
            "WIFI_ATTACK_BEACON_LIST",
            "WIFI_ATTACK_BEACON_SPAM",
            "WIFI_ATTACK_FUNNY_BEACON",
            "WIFI_ATTACK_RICK_ROLL",
            "WIFI_ATTACK_AUTH",
            "WIFI_ATTACK_DEAUTH",
            "WIFI_ATTACK_AP_SPAM",
            "WIFI_ATTACK_BAD_MSG",
            "WIFI_ATTACK_SLEEP",
            "WIFI_ATTACK_SAE_COMMIT",
            "WIFI_ATTACK_CSA",
            "WIFI_ATTACK_QUIET",
        }
        guided_flow = MENU[MENU.index("void MenuFunctions::beginAttackPlan") :]
        for mode in expected_guided_modes:
            self.assertIn(mode, guided_flow, f"guided flow lost {mode}")

        engine = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text(encoding="utf-8")
        for legacy_mode in (
            "WIFI_ATTACK_DEAUTH_TARGETED",
            "WIFI_ATTACK_BAD_MSG_TARGETED",
            "WIFI_ATTACK_SLEEP_TARGETED",
        ):
            self.assertIn(legacy_mode, engine, f"legacy/CLI path lost {legacy_mode}")

    def test_special_attack_configuration_paths_remain_available(self):
        for label in ("Evil Portal", "AP Config", "Access Points", "User SSIDs", "Karma"):
            self.assertIn(f'"{label}"', MENU)
        self.assertIn("buildAttackReviewMenu();", MENU)
        self.assertIn('"Start Attack"', MENU)

    def test_station_attacks_build_per_ap_profiles(self):
        flow = MENU[MENU.index("void MenuFunctions::buildAttackModeMenu") :]
        for label in ("Broadcast", "All Stations", "Pick Stations", "Add Another AP"):
            self.assertIn(f'"{label}"', flow)
        self.assertIn("buildAttackStationApPicker();", flow)
        self.assertIn("attack_active_ap_index = i;", flow)
        self.assertIn("setAttackApScope(attack_active_ap_index", flow)

        engine = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text(encoding="utf-8")
        for scope in (
            "ATTACK_AP_SCOPE_BROADCAST",
            "ATTACK_AP_SCOPE_ALL_STATIONS",
            "ATTACK_AP_SCOPE_PICK_STATIONS",
        ):
            self.assertIn(scope, engine)


if __name__ == "__main__":
    unittest.main()
