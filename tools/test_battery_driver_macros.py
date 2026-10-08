from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BatteryDriverMacroTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        configs_path = ROOT / "esp32_marauder" / "configs.h"
        cls.configs = configs_path.read_text(encoding="utf-8")

    def test_v6_maintains_ip5306_support(self):
        pattern = re.compile(
            r"#if defined\(MARAUDER_V6\) \|\| defined\(MARAUDER_V6_1\).*?#define HAS_BATTERY.*?#define HAS_IP5306",
            re.DOTALL,
        )
        self.assertRegex(
            self.configs,
            pattern,
            "MARAUDER_V6/MARAUDER_V6_1 must keep IP5306 battery support enabled.",
        )

    def test_v7_selects_max1704x_and_disables_ip5306(self):
        pattern = re.compile(
            r"#elif defined\(MARAUDER_V7\).*?#define HAS_MAX1704X.*?#undef HAS_IP5306",
            re.DOTALL,
        )
        self.assertRegex(
            self.configs,
            pattern,
            "MARAUDER_V7 must select HAS_MAX1704X and undefine HAS_IP5306.",
        )

    def test_battery_driver_cleanup_handles_ip5306(self):
        cleanup_match = re.search(
            r"//  If we know what we have, we can delete what we're not using(?P<cleanup>.*?)#endif\s+// HAS_BATTERY",
            self.configs,
            re.DOTALL,
        )
        self.assertIsNotNone(
            cleanup_match, "Could not find battery driver cleanup section."
        )
        cleanup = cleanup_match.group("cleanup")

        ip5306_match = re.search(
            r"#elif defined\(HAS_IP5306\)(?P<branch>.*?)(?=\r?\n\s*#elif|\r?\n\s*#else|\r?\n\s*#endif)",
            cleanup,
            re.DOTALL,
        )
        self.assertIsNotNone(
            ip5306_match,
            "HAS_IP5306 must have an explicit cleanup branch before the generic fallback.",
        )
        branch = ip5306_match.group("branch")

        self.assertIn(
            "#undef HAS_AXP2101",
            branch,
            "HAS_IP5306 cleanup must disable HAS_AXP2101.",
        )
        self.assertIn(
            "#undef HAS_MAX1704X",
            branch,
            "HAS_IP5306 cleanup must disable HAS_MAX1704X.",
        )
        self.assertIn(
            "#undef HAS_AXP192",
            branch,
            "HAS_IP5306 cleanup must disable HAS_AXP192.",
        )
        self.assertNotIn(
            "#undef HAS_IP5306",
            branch,
            "HAS_IP5306 cleanup must preserve HAS_IP5306 for v6/v6.1 battery support.",
        )

    def test_battery_driver_cleanup_handles_adc_pin(self):
        cleanup_match = re.search(
            r"//  If we know what we have, we can delete what we're not using(?P<cleanup>.*?)#endif\s+// HAS_BATTERY",
            self.configs,
            re.DOTALL,
        )
        self.assertIsNotNone(
            cleanup_match, "Could not find battery driver cleanup section."
        )
        cleanup = cleanup_match.group("cleanup")

        adc_match = re.search(
            r"#ifdef BATTERY_ADC_PIN(?P<branch>.*?)(?=\r?\n\s*#elif|\r?\n\s*#else|\r?\n\s*#endif)",
            cleanup,
            re.DOTALL,
        )
        self.assertIsNotNone(adc_match, "BATTERY_ADC_PIN must have a cleanup branch.")
        branch = adc_match.group("branch")

        self.assertIn("#undef HAS_AXP2101", branch)
        self.assertIn("#undef HAS_IP5306", branch)
        self.assertIn("#undef HAS_MAX1704X", branch)
        self.assertIn("#undef HAS_AXP192", branch)


if __name__ == "__main__":
    unittest.main()
