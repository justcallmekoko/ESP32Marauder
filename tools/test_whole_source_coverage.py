import tempfile
import unittest
from pathlib import Path

from whole_source_coverage import source_lines


class SourceLinesTests(unittest.TestCase):
    def test_honors_gcovr_line_and_block_exclusions(self):
        source_text = """counted();
excluded_line(); // GCOVR_EXCL_LINE
// GCOVR_EXCL_START
excluded_block();
// GCOVR_EXCL_STOP
counted_again();
"""
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "sample.cpp"
            source.write_text(source_text, encoding="utf-8")
            self.assertEqual(source_lines(source), [1, 6])


if __name__ == "__main__":
    unittest.main()
