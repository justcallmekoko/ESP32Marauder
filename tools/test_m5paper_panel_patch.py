"""Ensure the board-local vendor patch fails closed and can be repeated."""
import pathlib
import tempfile
import unittest
from unittest import mock

import m5paper_panel_patch as patcher


class M5PaperPanelPatchTests(unittest.TestCase):
    def setUp(self):
        self.original = {
            name: "\n".join(before for before, _ in replacements)
            for name, replacements in patcher.REPLACEMENTS.items()
        }
        self.patched = {name: patcher.transform(name, text)
                        for name, text in self.original.items()}
        self.original_hash = {name: patcher.digest(text)
                              for name, text in self.original.items()}
        self.patched_hash = {name: patcher.digest(text)
                             for name, text in self.patched.items()}

    def test_unknown_dependency_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "requires the pinned"):
            patcher.patched_text("Panel_IT8951.inl", "unrelated driver")

    def test_checked_transformation_is_idempotent(self):
        with mock.patch.dict(patcher.ORIGINAL_HASH, self.original_hash), \
                mock.patch.dict(patcher.PATCHED_HASH, self.patched_hash):
            for name, text in self.original.items():
                output = patcher.patched_text(name, text)
                self.assertEqual(output, self.patched[name])
                self.assertEqual(patcher.patched_text(name, output), output)
                with self.assertRaises(RuntimeError):
                    patcher.patched_text(name, output + "unexpected edit")

    def test_context_drift_is_rejected(self):
        for name, text in self.original.items():
            with self.subTest(name=name), self.assertRaisesRegex(RuntimeError, "context changed"):
                patcher.transform(name, text + patcher.REPLACEMENTS[name][0][0])

    def test_both_files_are_checked_before_any_write(self):
        with tempfile.TemporaryDirectory() as directory, \
                mock.patch.dict(patcher.ORIGINAL_HASH, self.original_hash), \
                mock.patch.dict(patcher.PATCHED_HASH, self.patched_hash):
            folder = pathlib.Path(directory)
            for name, text in self.original.items():
                (folder / name).write_text(text, encoding="utf-8", newline="\r\n")
            header = folder / "Panel_IT8951.hpp"
            original_bytes = header.read_bytes()
            (folder / "Panel_IT8951.inl").write_text("unknown driver", encoding="utf-8")
            with self.assertRaises(RuntimeError):
                patcher.patch_folder(folder)
            self.assertEqual(header.read_bytes(), original_bytes)

    def test_crlf_checkout_and_repeated_build_are_supported(self):
        with tempfile.TemporaryDirectory() as directory, \
                mock.patch.dict(patcher.ORIGINAL_HASH, self.original_hash), \
                mock.patch.dict(patcher.PATCHED_HASH, self.patched_hash):
            folder = pathlib.Path(directory)
            for name, text in self.original.items():
                (folder / name).write_text(text, encoding="utf-8", newline="\r\n")
            patcher.patch_folder(folder)
            first = {name: (folder / name).read_bytes() for name in self.original}
            patcher.patch_folder(folder)
            for name, content in first.items():
                self.assertEqual((folder / name).read_bytes(), content)


if __name__ == "__main__":
    unittest.main()
