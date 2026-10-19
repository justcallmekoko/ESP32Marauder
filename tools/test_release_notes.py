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
        changes = (
            "## What's Changed\n\n"
            "* Add CSI motion monitor by @contributor in https://example.com/pull/1\n"
            "* Upgrade wardrive logs to WiGLE 1.6 by @contributor in https://example.com/pull/2\n\n"
            "**Full Changelog**: https://example.com/compare/v1.18.0...v1.19.0"
        )
        notes = render_release_notes(targets, "stable", changes)
        table = notes.split("## Firmware assets\n\n", 1)[1].split("\n\n", 1)[0]

        self.assertEqual(len(targets), 28)
        for target in targets:
            self.assertEqual(table.count(f"`_{target['assetSuffix']}.bin`"), 1)
            self.assertIn(target["displayName"], table)

        self.assertNotIn("flipperzero-firmware-with-wifi-marauder-companion", notes)
        self.assertNotIn("`_new_hardware.bin`", table)
        self.assertNotIn("| `_v7.bin` |", table)
        self.assertNotIn("| `_lddb.bin` |", table)
        self.assertNotIn("| `_esp32c5_devkit.bin` |", table)
        self.assertTrue(notes.startswith("## What's Changed"))
        self.assertIn(
            "[CSI motion monitor](https://github.com/justcallmekoko/ESP32Marauder/wiki/CSI-Motion-Monitor)",
            notes,
        )
        self.assertIn(
            "[wardrive logs](https://github.com/justcallmekoko/ESP32Marauder/wiki/Wardrive)",
            notes,
        )
        self.assertLess(notes.index("## What's Changed"), notes.index("## Before installing"))
        self.assertLess(notes.index("## Before installing"), notes.index("## Firmware assets"))
        self.assertLess(notes.index("## Firmware assets"), notes.index("## Additional resources"))

    def test_nightly_notes_are_clearly_marked(self) -> None:
        notes = render_release_notes(load_targets(REGISTRY), "nightly")
        self.assertIn("**Nightly build:**", notes)
        self.assertIn("for testing and evaluation", notes)

    def test_stable_notes_fail_closed_without_generated_changes(self) -> None:
        with self.assertRaisesRegex(ValueError, "GitHub-generated changes"):
            render_release_notes(load_targets(REGISTRY), "stable")

    def test_stable_notes_do_not_link_unrelated_changes(self) -> None:
        changes = (
            "## What's Changed\n\n"
            "* Fix an internal parser by @contributor in https://example.com/pull/1\n\n"
            "**Full Changelog**: https://example.com/compare/v1.18.0...v1.19.0"
        )
        notes = render_release_notes(load_targets(REGISTRY), "stable", changes)
        self.assertIn("* Fix an internal parser by @contributor", notes)
        self.assertNotIn("wiki/", notes.split("## Before installing", 1)[0])

    def test_release_workflows_use_generated_body(self) -> None:
        stable = STABLE_WORKFLOW.read_text(encoding="utf-8")
        nightly = NIGHTLY_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn("python3 tools/release_notes.py", stable)
        self.assertIn("--channel stable", stable)
        self.assertIn("releases/generate-notes", stable)
        self.assertIn("--changes-file generated-changes.md", stable)
        self.assertIn("body_path: release-notes.md", stable)
        self.assertIn("tools/release_notes.py --channel nightly", nightly)
        self.assertIn("body_path: release-notes.md", nightly)
        self.assertNotIn("| Hardware | Binary Version |", stable)
        self.assertNotIn("| Hardware | Binary Version |", nightly)


if __name__ == "__main__":
    unittest.main()
