import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class RetainedDevelopFeatureTests(unittest.TestCase):
    def test_arp_scan_is_exposed_on_dual_band_hardware(self):
        menu = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        cli = (ROOT / "esp32_marauder" / "CommandLine.cpp").read_text()

        menu_entry = menu.index('"ARP Scan", TFTCYAN')
        cli_entry = cli.index("cmd_args.get(0) == ARP_SCAN_CMD")
        self.assertNotIn("#ifndef HAS_DUAL_BAND", menu[menu_entry - 160:menu_entry])
        self.assertNotIn("#ifndef HAS_DUAL_BAND", cli[cli_entry - 160:cli_entry])

    def test_service_scans_remain_arp_first(self):
        source = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text()
        service_scan = source[source.index("void WiFiScan::pingScan") :]
        self.assertIn("this->singleARP(this->current_scan_ip)", service_scan)
        for mode in ("WIFI_SCAN_SSH", "WIFI_SCAN_TELNET", "WIFI_SCAN_SMTP",
                     "WIFI_SCAN_DNS", "WIFI_SCAN_HTTP", "WIFI_SCAN_HTTPS",
                     "WIFI_SCAN_RDP"):
            self.assertIn(mode, service_scan)

    def test_spiffs_backup_and_restore_are_in_device_menu(self):
        source = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        self.assertIn('"Backup SPIFFS"', source)
        self.assertIn('"Restore SPIFFS"', source)
        self.assertIn("cli_obj.runCommand(BACKUP_SPIFFS_CMD)", source)
        self.assertIn("cli_obj.runCommand(RESTORE_SPIFFS_CMD)", source)


if __name__ == "__main__":
    unittest.main()
