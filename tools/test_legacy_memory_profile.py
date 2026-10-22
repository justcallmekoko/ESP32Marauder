#!/usr/bin/env python3
"""Regression checks for the legacy ESP32 wardriving memory budget."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class LegacyMemoryProfileTests(unittest.TestCase):
    def test_profile_is_capability_based_and_excludes_c5(self):
        configs = (ROOT / "esp32_marauder/configs.h").read_text(encoding="utf-8")
        profile = configs[configs.index("Conserve internal RAM"):
                          configs.index("//// END BOARD FEATURES")]
        self.assertIn("!defined(HAS_PSRAM)", profile)
        self.assertIn("!defined(MARAUDER_C5)", profile)
        self.assertIn("!defined(MARAUDER_T_DONGLE_C5)", profile)
        self.assertIn("!defined(XIAO_ESP32_C5)", profile)
        self.assertIn("!defined(DUAL_MINI_C5)", profile)
        self.assertIn("MARAUDER_NO_PSRAM_MEMORY_PROFILE", profile)

    def test_c5_targets_keep_full_csi_and_ble_capacities(self):
        csi = (ROOT / "esp32_marauder/CsiMotionMonitor.h").read_text(encoding="utf-8")
        wifi = (ROOT / "esp32_marauder/WiFiScan.cpp").read_text(encoding="utf-8")
        self.assertIn("#if defined(MARAUDER_NO_PSRAM_MEMORY_PROFILE)", csi)
        self.assertIn("kQueueSize = 44", csi)
        self.assertIn("kQueueSize = 64", csi)
        self.assertIn("MAX_RETAINED_BLE_DEVICES = 64", wifi)
        self.assertIn("MAX_RETAINED_BLE_DEVICES = 128", wifi)
        self.assertIn("NIMBLE_DUPLICATE_CACHE_SIZE = 200", wifi)

    def test_direct_upload_remains_enabled_on_v6_v61_and_v7(self):
        configs = (ROOT / "esp32_marauder/configs.h").read_text(encoding="utf-8")
        wifi_header = (ROOT / "esp32_marauder/WiFiScan.h").read_text(encoding="utf-8")
        v7 = configs[configs.index("#ifdef MARAUDER_V7"):
                     configs.index("#ifdef MARAUDER_V7_1")]
        v6_start = configs.index(
            "#if defined(MARAUDER_V6) || defined(MARAUDER_V6_1)",
            configs.index("#ifdef MARAUDER_V4"),
        )
        v6 = configs[v6_start:configs.index("#ifdef MARAUDER_CYD_MICRO", v6_start)]
        self.assertIn("#define HAS_DIRECT_UPLOAD", v7)
        self.assertIn("#define HAS_DIRECT_UPLOAD", v6)
        self.assertIn("#define HAS_DIRECT_UPLOAD", configs[configs.index("#ifdef MARAUDER_C5"):])
        self.assertIn("#include <WiFiClientSecure.h>", wifi_header)
        self.assertNotIn("#include <HTTPClient.h>", wifi_header)
        self.assertNotIn('mbedtls/sha256.h', wifi_header)

    def test_portal_html_is_allocated_only_when_used(self):
        header = (ROOT / "esp32_marauder/EvilPortal.h").read_text(encoding="utf-8")
        source = (ROOT / "esp32_marauder/EvilPortal.cpp").read_text(encoding="utf-8")
        self.assertIn("extern char* index_html;", header)
        self.assertNotIn("char index_html[MAX_HTML_SIZE]", header)
        self.assertIn("replacePortalHtml", source)
        self.assertIn("free(index_html);", source)
        self.assertIn("index_html = nullptr;", source)

    def test_csi_queue_is_allocated_only_while_active(self):
        header = (ROOT / "esp32_marauder/CsiMotionMonitor.h").read_text(
            encoding="utf-8"
        )
        source = (ROOT / "esp32_marauder/CsiMotionMonitor.cpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("Sample* queue_ = nullptr;", header)
        self.assertNotIn("Sample queue_[kQueueSize]", header)
        self.assertIn("bool CsiMotionMonitor::allocateQueue()", source)
        self.assertIn("queue_ = static_cast<Sample*>(ps_malloc(bytes));", source)
        self.assertIn("RuntimeMemoryGuard::instance().allow(bytes)", source)
        self.assertIn("queue_ = static_cast<Sample*>(malloc(bytes));", source)
        self.assertIn("void CsiMotionMonitor::releaseQueue()", source)
        self.assertIn("free(queue_);", source)
        self.assertIn("releaseQueue();", source[source.index("void CsiMotionMonitor::stop()"):])

    def test_portal_feedback_survives_dynamic_html_pressure(self):
        portal = (ROOT / "esp32_marauder/EvilPortal.cpp").read_text(encoding="utf-8")
        display = (ROOT / "esp32_marauder/Display.cpp").read_text(encoding="utf-8")
        self.assertIn("display_obj.queueCriticalLine(display_string)", portal)
        critical = display.split("bool Display::queueCriticalLine", 1)[1]
        critical = critical.split("void Display::displayBuffer", 1)[0]
        self.assertIn("display_queue_pressure.allow", critical)
        self.assertNotIn("RuntimeMemoryGuard", critical)

    def test_watch_model_lookup_cannot_overrun_table(self):
        source = (ROOT / "esp32_marauder/WiFiScan.cpp").read_text(encoding="utf-8")
        self.assertIn("WATCH_MODEL_COUNT", source)
        self.assertIn("rand() % WATCH_MODEL_COUNT", source)
        self.assertNotIn("rand() % 25", source)
        self.assertNotIn("new WatchModel", source)


if __name__ == "__main__":
    unittest.main()
