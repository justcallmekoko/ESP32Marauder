#!/usr/bin/env python3
"""Regression checks for TFT_eSPI setup selection in firmware workflows."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = (
    ROOT / ".github/workflows/build_parallel.yml",
    ROOT / ".github/workflows/nightly_build.yml",
)

class TftWorkflowConfigurationTests(unittest.TestCase):
    def test_workflows_fail_if_requested_setup_is_not_selected(self):
        for workflow in WORKFLOWS:
            text = workflow.read_text(encoding="utf-8")
            with self.subTest(workflow=workflow.name):
                self.assertIn('setup_file="$GITHUB_WORKSPACE/CustomTFT_eSPI/User_Setup_Select.h"', text)
                self.assertIn('setup_header="$GITHUB_WORKSPACE/CustomTFT_eSPI/${{ matrix.board.tft_file }}"', text)
                self.assertIn('test -f "$setup_file"', text)
                self.assertIn('test -f "$setup_header"', text)
                self.assertIn("grep -Fqx '#include <${{ matrix.board.tft_file }}>' \"$setup_file\"", text)

    def test_invalid_placeholder_path_is_not_used(self):
        for workflow in WORKFLOWS:
            with self.subTest(workflow=workflow.name):
                self.assertNotIn(".../User_Setup_Select.h", workflow.read_text(encoding="utf-8"))

if __name__ == "__main__":
    unittest.main()
