from __future__ import annotations

import json
import re
import tempfile
import unittest
from pathlib import Path

from tools.installer_manifest import (
    ManifestError,
    combine_manifests,
    generate_target_manifest,
    load_registry,
)

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
REGISTRY = REPOSITORY_ROOT / "installer" / "targets.json"


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

    def test_registry_contains_unique_complete_build_targets(self) -> None:
        registry = load_registry(REGISTRY)
        workflow = (REPOSITORY_ROOT / ".github/workflows/build_parallel.yml").read_text(
            encoding="utf-8"
        )
        workflow_flags = set(re.findall(r'flag: "([A-Z0-9_]+)"', workflow))
        registry_flags = {target["buildFlag"] for target in registry["targets"]}
        self.assertIn("set-build-path: true", workflow)
        self.assertIn("BUILD_DIR=./esp32_marauder/build", workflow)
        self.assertEqual(len(registry["targets"]), 22)
        self.assertEqual(registry_flags, workflow_flags)
        self.assertEqual(
            len(registry_flags),
            len(registry["targets"]),
        )

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
                self.assertRegex(segment["sha256"], r"^[0-9a-f]{64}$")
                self.assertTrue((output / segment["fileName"]).is_file())

    def test_fails_closed_without_actual_flash_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ManifestError, "No flash_args"):
                generate_target_manifest(
                    REGISTRY,
                    "MARAUDER_V6",
                    Path(temporary),
                    "v1.2.3",
                    "20260731",
                    "a" * 40,
                    Path(temporary) / "output",
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
