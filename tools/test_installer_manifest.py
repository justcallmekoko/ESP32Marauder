from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.installer_manifest import (
    ManifestError,
    combine_manifests,
    generate_target_manifest,
    load_normal_build_matrix,
    load_registry,
)

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
REGISTRY = REPOSITORY_ROOT / "installer" / "targets.json"
NORMAL_WORKFLOW = REPOSITORY_ROOT / ".github/workflows/build_parallel.yml"
INSTALLER_WORKFLOW = REPOSITORY_ROOT / ".github/workflows/build_installer_manifests.yml"


class InstallerManifestTests(unittest.TestCase):
    @staticmethod
    def make_fake_build(root: Path) -> Path:
        build = root / "build"
        build.mkdir()
        (build / "bootloader.bin").write_bytes(b"boot")
        (build / "partitions.bin").write_bytes(b"partitions")
        (build / "boot_app0.bin").write_bytes(b"ota")
        (build / "esp32_marauder.ino.bin").write_bytes(b"application")
        (build / "flash_args").write_text(
            "--flash_mode dio --flash_freq 40m --flash_size 4MB "
            "0x1000 bootloader.bin 0x8000 partitions.bin "
            "0xe000 boot_app0.bin 0x10000 esp32_marauder.ino.bin\n",
            encoding="utf-8",
        )
        return build

    @staticmethod
    def make_fake_properties_build(root: Path) -> Path:
        build = InstallerManifestTests.make_fake_build(root)
        (build / "flash_args").unlink()
        (build / "arduino-build-properties.txt").write_text(
            "build.flash_mode=dio\n"
            "build.flash_freq=80m\n"
            "build.flash_size=4MB\n"
            "tools.esptool_py.upload.pattern_args=--chip esp32 --flash-mode keep "
            "--flash-freq keep --flash-size keep 0x1000 bootloader.bin "
            "0x8000 partitions.bin 0xe000 boot_app0.bin "
            "0x10000 esp32_marauder.ino.bin\n",
            encoding="utf-8",
        )
        return build

    def test_registry_contains_unique_complete_build_targets(self) -> None:
        registry = load_registry(REGISTRY)
        boards = load_normal_build_matrix(NORMAL_WORKFLOW, REGISTRY)
        workflow_flags = {board["flag"] for board in boards}
        registry_flags = {target["buildFlag"] for target in registry["targets"]}
        normal_workflow = NORMAL_WORKFLOW.read_text(encoding="utf-8")
        installer_workflow = INSTALLER_WORKFLOW.read_text(encoding="utf-8")

        self.assertNotIn("installer_manifest.py", normal_workflow)
        self.assertIn("--export-build-matrix", installer_workflow)
        self.assertIn("set-build-path: true", installer_workflow)
        self.assertIn("--show-properties=expanded", installer_workflow)
        self.assertIn("github.event_name == 'release'", installer_workflow)
        self.assertEqual(len(registry["targets"]), 22)
        self.assertEqual(len(boards), 22)
        self.assertEqual(registry_flags, workflow_flags)
        self.assertEqual(
            len(registry_flags),
            len(registry["targets"]),
        )

    def test_build_matrix_parser_fails_closed_on_unsupported_syntax(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            workflow = Path(temporary) / "build_parallel.yml"
            contents = NORMAL_WORKFLOW.read_text(encoding="utf-8")
            workflow.write_text(contents.replace("fbqn:", "fbqn=", 1), encoding="utf-8")
            with self.assertRaisesRegex(ManifestError, "Unsupported build matrix syntax"):
                load_normal_build_matrix(workflow, REGISTRY)

    def test_generates_hashed_update_and_factory_segments_from_flash_args(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = self.make_fake_build(root)
            output = root / "output"

            path = generate_target_manifest(
                REGISTRY,
                "MARAUDER_V6",
                build,
                "v1.2.3",
                "20260731",
                "a" * 40,
                output,
            )
            manifest = json.loads(path.read_text(encoding="utf-8"))

            self.assertEqual(manifest["channel"], "stable")
            self.assertEqual(manifest["metadataStatus"], "authoritative")
            self.assertEqual(manifest["target"]["id"], "marauder-v6")
            self.assertEqual(manifest["flash"]["sizeBytes"], 4 * 1024 * 1024)
            self.assertEqual(len(manifest["flash"]["update"]["segments"]), 1)
            self.assertEqual(len(manifest["flash"]["factory"]["segments"]), 4)
            for segment in manifest["flash"]["factory"]["segments"]:
                self.assertTrue(
                    segment["fileName"].startswith("esp32_marauder_installer_")
                )
                self.assertRegex(segment["sha256"], r"^[0-9a-f]{64}$")
                self.assertTrue((output / segment["fileName"]).is_file())

    def test_fails_closed_without_actual_flash_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ManifestError, "No Arduino build properties"):
                generate_target_manifest(
                    REGISTRY,
                    "MARAUDER_V6",
                    Path(temporary),
                    "v1.2.3",
                    "20260731",
                    "a" * 40,
                    Path(temporary) / "output",
                )

    def test_generates_from_expanded_arduino_upload_properties(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = self.make_fake_properties_build(root)
            output = root / "output"

            path = generate_target_manifest(
                REGISTRY,
                "MARAUDER_FLIPPER",
                build,
                "v1.2.3",
                "20260731",
                "a" * 40,
                output,
            )
            manifest = json.loads(path.read_text(encoding="utf-8"))

            self.assertEqual(manifest["flash"]["mode"], "dio")
            self.assertEqual(manifest["flash"]["frequency"], "80m")
            self.assertEqual(manifest["flash"]["sizeBytes"], 4 * 1024 * 1024)
            self.assertEqual(
                [segment["offset"] for segment in manifest["flash"]["factory"]["segments"]],
                [0x1000, 0x8000, 0xE000, 0x10000],
            )

    def test_combiner_requires_complete_registry_coverage(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ManifestError, "coverage mismatch"):
                combine_manifests(REGISTRY, root, root / "firmware-manifest.json")

    def test_combines_complete_stable_release(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = self.make_fake_build(root)
            output = root / "output"
            registry = load_registry(REGISTRY)
            for target in registry["targets"]:
                generate_target_manifest(
                    REGISTRY,
                    target["buildFlag"],
                    build,
                    "v1.2.3",
                    "20260731",
                    "a" * 40,
                    output,
                )

            release_path = combine_manifests(
                REGISTRY, output, output / "firmware-manifest.json"
            )
            release = json.loads(release_path.read_text(encoding="utf-8"))

            self.assertEqual(release["metadataStatus"], "authoritative")
            self.assertEqual(release["channel"], "stable")
            self.assertEqual(release["sourceCommit"], "a" * 40)
            self.assertEqual(len(release["targets"]), 22)
            self.assertIn("/" + "a" * 40 + "/", release["$schema"])


if __name__ == "__main__":
    unittest.main()
