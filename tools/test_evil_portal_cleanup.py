#!/usr/bin/env python3
"""Keep Evil Portal teardown reachable on headless targets."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WIFI_SCAN = (ROOT / "esp32_marauder" / "WiFiScan.cpp").read_text(encoding="utf-8")
EVIL_PORTAL = (ROOT / "esp32_marauder" / "EvilPortal.cpp").read_text(encoding="utf-8")
CONFIGS = (ROOT / "esp32_marauder" / "configs.h").read_text(encoding="utf-8")


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


class EvilPortalCleanupTests(unittest.TestCase):
    def test_stop_scan_cleanup_is_not_screen_guarded(self) -> None:
        stop_scan = function_body(WIFI_SCAN, "void WiFiScan::StopScan")
        self.assertEqual(stop_scan.count("evil_portal_obj.cleanup();"), 1)

        screen_depth = 0
        cleanup_depth = None
        for line in stop_scan.splitlines():
            stripped = line.strip()
            if stripped == "#ifdef HAS_SCREEN":
                screen_depth += 1
            elif stripped == "#endif" and screen_depth:
                screen_depth -= 1
            elif "evil_portal_obj.cleanup();" in stripped:
                cleanup_depth = screen_depth

        self.assertEqual(cleanup_depth, 0)

    def test_cleanup_stops_dns_and_soft_ap(self) -> None:
        cleanup = function_body(EVIL_PORTAL, "void EvilPortal::cleanup")
        self.assertIn("dnsServer.stop();", cleanup)
        self.assertIn("WiFi.softAPdisconnect(true);", cleanup)

    def test_c5_devkit_remains_a_headless_target(self) -> None:
        c5 = CONFIGS[CONFIGS.index("#ifdef MARAUDER_C5") :]
        c5 = c5[: c5.index("#endif")]
        self.assertIn("//#define HAS_SCREEN", c5)
        active_defines = [
            line.strip() for line in c5.splitlines()
            if not line.strip().startswith("//")
        ]
        self.assertNotIn("#define HAS_SCREEN", active_defines)


if __name__ == "__main__":
    unittest.main()
