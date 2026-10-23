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

    def test_v8_keeps_complete_cli_help(self):
        source = (ROOT / "esp32_marauder" / "CommandLine.cpp").read_text()
        help_block = source[source.index("if (cmd_args.get(0) == HELP_CMD)"):
                            source.index("// Stop Scan")]
        self.assertNotIn("ESP32 Marauder commands: https://", help_block)
        self.assertNotIn("#ifdef MARAUDER_V8", help_block)
        self.assertIn("Serial.println(HELP_HEAD)", help_block)
        self.assertIn("Serial.println(HELP_PROTOCOL_INFO_CMD)", help_block)
        self.assertIn("Serial.println(HELP_RESTORE_SPIFFS_CMD)", help_block)

    def test_ble_recon_supports_both_nimble_api_generations(self):
        header = (ROOT / "esp32_marauder" / "WiFiScan.h").read_text()
        self.assertIn("using MarauderBLEAdvertisedDevice = const NimBLEAdvertisedDevice;",
                      header)
        self.assertIn("using MarauderBLEAdvertisedDevice = NimBLEAdvertisedDevice;",
                      header)
        self.assertIn("classifyBLEDevice(MarauderBLEAdvertisedDevice*", header)

    def test_station_retention_is_cxx11_compatible(self):
        source = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text()
        self.assertIn("Station sta = {};", source)
        self.assertIn("sta.last_seen_ms = millis();", source)
        self.assertNotIn("Station sta = {\n                    {", source)

    def test_cardputer_packet_graph_explicitly_clears_each_slot(self):
        source = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text()
        renderer = source[source.index("void WiFiScan::drawPacketMonitorGraph"):
                          source.index("void WiFiScan::drawPacketMonitorGraphs")]
        self.assertIn("i * PACKET_MONITOR_COLUMN_STEP", renderer)
        self.assertIn("fillRect(x, plot_top, PACKET_MONITOR_COLUMN_STEP", renderer)
        self.assertLess(
            renderer.index("fillRect(x, plot_top, PACKET_MONITOR_COLUMN_STEP"),
            renderer.index("drawFastHLine(PACKET_MONITOR_GRAPH_LEFT, plot_top"),
        )

    def test_boolean_setting_toggle_updates_the_rendered_menu_row(self):
        source = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        settings_menu = source[source.index("// Settings menu"):
                               source.index("Serial.println(\"Finished settings nodes\")")]
        self.assertIn(
            "const int settings_menu_index = settingsMenu.list->size();",
            settings_menu,
        )
        self.assertIn(
            "[this, settings_menu_index, settingName]()",
            settings_menu,
        )
        self.assertIn(
            "displaySetting(settingName.c_str(), &settingsMenu, settings_menu_index);",
            settings_menu,
        )
        self.assertNotIn(
            "displaySetting(settingName.c_str(), &settingsMenu, i + 1);",
            settings_menu,
        )

    def test_csi_monitor_streams_refined_status_but_not_raw_data(self):
        monitor = (ROOT / "esp32_marauder" / "CsiMotionMonitor.cpp").read_text()
        sketch = (ROOT / "esp32_marauder" / "esp32_marauder.ino").read_text()
        self.assertNotIn("CSI_RAW", monitor)
        self.assertNotIn("CSI_METRIC", monitor)
        self.assertNotIn("CSI_SESSION", monitor)
        self.assertNotIn("CSI_SCHEMA", monitor)
        self.assertNotIn("writeRawRecord", monitor)
        self.assertIn('"CSI motion | state=%s | motion=%u | reliability=%u', monitor)
        self.assertIn("Serial.begin(115200);", sketch)
        self.assertNotIn("Serial.begin(921600);", sketch)

    def test_csi_motion_is_available_on_mini_v3_with_compact_controls(self):
        config = (ROOT / "esp32_marauder" / "configs.h").read_text()
        monitor = (ROOT / "esp32_marauder" / "CsiMotionMonitor.cpp").read_text()
        menu = (ROOT / "esp32_marauder" / "MenuFunctions.cpp").read_text()
        self.assertIn("defined(MARAUDER_MINI_V3)", config)
        self.assertIn('display_obj.tft.print("Up=Cal C=Stop");', monitor)
        self.assertIn(
            "else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CSI_MOTION)",
            menu,
        )
        self.assertIn("csi_motion_obj.recalibrate();", menu)

    def test_csi_motion_has_separate_classic_esp32_path(self):
        config = (ROOT / "esp32_marauder" / "configs.h").read_text()
        monitor = (ROOT / "esp32_marauder" / "CsiMotionMonitor.cpp").read_text()
        model = (ROOT / "esp32_marauder" / "CsiMotionModel.cpp").read_text()
        for target in ("MARAUDER_V4", "MARAUDER_V6", "MARAUDER_V6_1",
                       "MARAUDER_V7", "MARAUDER_V7_1"):
            self.assertIn(f"defined({target})", config)
        self.assertIn("config.lltf_en = true;", monitor)
        self.assertIn("config.htltf_en = false;", monitor)
        self.assertIn("config.stbc_htltf2_en = false;", monitor)
        self.assertIn("config.channel_filter_en = false;", monitor)
        self.assertIn("SampleEncoding::SIGNED_8_IMAG_REAL", monitor)
        self.assertIn("Original ESP32 CSI is emitted as signed int8", model)

    def test_csi_motion_covers_supported_chip_families_and_headless_cli(self):
        config = (ROOT / "esp32_marauder" / "configs.h").read_text()
        monitor = (ROOT / "esp32_marauder" / "CsiMotionMonitor.cpp").read_text()
        cli_h = (ROOT / "esp32_marauder" / "CommandLine.h").read_text()
        cli_cpp = (ROOT / "esp32_marauder" / "CommandLine.cpp").read_text()
        for target in ("MARAUDER_CARDPUTER_ADV", "MARAUDER_MULTIBOARD_S3",
                       "MARAUDER_FLIPPER", "MARAUDER_M5_NANO_C6",
                       "MARAUDER_C5", "MARAUDER_PANCAKE", "MARAUDER_POOM"):
            self.assertIn(f"defined({target})", config)
        self.assertIn("#if !CONFIG_SOC_WIFI_HE_SUPPORT", monitor)
        self.assertIn("#if defined(CONFIG_IDF_TARGET_ESP32C5)", monitor)
        self.assertIn('CSI_MOTION_CMD[] = "csimotion"', cli_h)
        self.assertIn("csi_motion_obj.start(ap.bssid, ap.essid, ap.channel)", cli_cpp)
        self.assertIn("csi_motion_obj.printStatus();", cli_cpp)

if __name__ == "__main__":
    unittest.main()
