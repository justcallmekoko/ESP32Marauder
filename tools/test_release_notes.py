from __future__ import annotations

import unittest
from pathlib import Path

from tools.release_notes import load_targets, render_release_notes


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
REGISTRY = REPOSITORY_ROOT / "installer" / "targets.json"
STABLE_WORKFLOW = REPOSITORY_ROOT / ".github" / "workflows" / "build_parallel.yml"
NIGHTLY_WORKFLOW = REPOSITORY_ROOT / ".github" / "workflows" / "nightly_build.yml"


class ReleaseNotesTests(unittest.TestCase):
    def test_stable_notes_cover_every_registered_target(self) -> None:
        targets = load_targets(REGISTRY)
        notes = render_release_notes(targets, "stable")
        table = notes.split("## Firmware assets\n\n", 1)[1].split("\n\n", 1)[0]

        self.assertEqual(len(targets), 27)
        for target in targets:
            self.assertEqual(table.count(f"`_{target['assetSuffix']}.bin`"), 1)
            self.assertIn(target["displayName"], table)

        self.assertNotIn("flipperzero-firmware-with-wifi-marauder-companion", notes)
        self.assertNotIn("`_new_hardware.bin`", table)
        self.assertNotIn("| `_v7.bin` |", table)
        self.assertNotIn("| `_lddb.bin` |", table)
        self.assertNotIn("| `_esp32c5_devkit.bin` |", table)

    def test_nightly_notes_are_clearly_marked(self) -> None:
        notes = render_release_notes(load_targets(REGISTRY), "nightly")
        self.assertIn("**Nightly build:**", notes)
        self.assertIn("for testing and evaluation", notes)

    def test_release_workflows_use_generated_body(self) -> None:
        stable = STABLE_WORKFLOW.read_text(encoding="utf-8")
        nightly = NIGHTLY_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn("tools/release_notes.py --channel stable", stable)
        self.assertIn("body_path: release-notes.md", stable)
        self.assertIn("tools/release_notes.py --channel nightly", nightly)
        self.assertIn("body_path: release-notes.md", nightly)
        self.assertNotIn("| Hardware | Binary Version |", stable)
        self.assertNotIn("| Hardware | Binary Version |", nightly)


if __name__ == "__main__":
    unittest.main()
