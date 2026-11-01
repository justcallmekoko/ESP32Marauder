"""Guard the flash layouts against unsupported SPIFFS page counts."""
import csv
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


def partitions(name):
    path = ROOT / "installer/partitions" / (name + ".csv")
    rows = csv.reader(line for line in path.read_text(encoding="utf-8").splitlines()
                      if line.strip() and not line.lstrip().startswith("#"))
    return {row[0].strip(): (row[2].strip(), int(row[3].strip(), 0),
                            int(row[4].strip(), 0)) for row in rows}


class EpaperPartitionTests(unittest.TestCase):
    def test_spiffs_fits_the_pinned_sdk_page_index(self):
        # ESP-IDF's pinned configs use u16_t spiffs_page_ix, 256-byte pages,
        # and reserve 0xffff as an index sentinel. The old Sticky 0x13f0000
        # filesystem exceeded this limit and could not mount or format.
        for target in ("reterminal_sticky", "m5paper_v11"):
            with self.subTest(target=target):
                _, _, size = partitions(target)["spiffs"]
                self.assertEqual(size % 256, 0)
                self.assertLess(size // 256, 0xffff)

    def test_partitions_are_aligned_nonoverlapping_and_within_flash(self):
        for target, flash_size in (("reterminal_sticky", 32 << 20),
                                   ("m5paper_v11", 16 << 20)):
            previous_end = 0x9000
            for name, (kind, offset, size) in sorted(partitions(target).items(),
                                                     key=lambda item: item[1][1]):
                with self.subTest(target=target, partition=name):
                    self.assertGreater(size, 0)
                    self.assertEqual(offset % 0x1000, 0)
                    self.assertEqual(size % 0x1000, 0)
                    self.assertGreaterEqual(offset, previous_end)
                    self.assertLessEqual(offset + size, flash_size)
                    if kind.startswith("ota_") and name.startswith("app"):
                        self.assertEqual(offset % 0x10000, 0)
                    previous_end = offset + size

    def test_sticky_fix_preserves_boot_slots_and_filesystem_start(self):
        layout = partitions("reterminal_sticky")
        self.assertEqual(layout["app0"], ("ota_0", 0x10000, 0x600000))
        self.assertEqual(layout["app1"], ("ota_1", 0x610000, 0x600000))
        self.assertEqual(layout["spiffs"][1], 0xc10000)
        self.assertLessEqual(layout["spiffs"][1] + layout["spiffs"][2], 0x1c00000)


if __name__ == "__main__":
    unittest.main()
