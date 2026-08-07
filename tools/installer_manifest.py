#!/usr/bin/env python3
"""Generate fail-closed installer metadata from Arduino's actual flash arguments."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
import shutil
from pathlib import Path
from typing import Any

SCHEMA_VERSION = 1
SOURCE_REPOSITORY = "justcallmekoko/ESP32Marauder"
REQUIRED_TARGET_KEYS = {
    "id",
    "displayName",
    "aliases",
    "buildFlag",
    "assetSuffix",
    "chipFamily",
    "esptoolChip",
}
REQUIRED_BUILD_MATRIX_KEYS = {
    "name",
    "flag",
    "fbqn",
    "file_name",
    "tft",
    "tft_file",
    "build_dir",
    "addr",
    "idf_ver",
    "nimble_ver",
    "esp_async",
    "esp_async_ver",
}
OPTIONAL_BUILD_MATRIX_KEYS = {"tft_repo", "tft_ref"}
CHIP_FAMILIES = {"ESP32", "ESP32-S2", "ESP32-S3", "ESP32-C5", "ESP32-C6"}
SHA_PATTERN = re.compile(r"^[0-9a-f]{40}$")
OFFSET_PATTERN = re.compile(r"^0x[0-9a-fA-F]+$")
SIZE_PATTERN = re.compile(r"^(?P<size>\d+)(?P<unit>MB|M|KB|K|B)?$", re.IGNORECASE)
FLOW_ENTRY_PATTERN = re.compile(r"^\s*-\s*\{(?P<body>.*)\}\s*$")
FLOW_FIELD_PATTERN = re.compile(
    r'\s*(?P<key>[a-z_][a-z0-9_]*)\s*:\s*'
    r'(?:(?:"(?P<string>[^"]*)")|(?P<boolean>true|false))\s*'
    r"(?P<separator>,|$)"
)


class ManifestError(RuntimeError):
    pass


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ManifestError(f"Cannot read valid JSON from {path}: {error}") from error
    if not isinstance(value, dict):
        raise ManifestError(f"{path} must contain a JSON object.")
    return value


def load_registry(path: Path) -> dict[str, Any]:
    registry = read_json(path)
    if registry.get("schemaVersion") != SCHEMA_VERSION:
        raise ManifestError("Unsupported target registry schemaVersion.")
    targets = registry.get("targets")
    if not isinstance(targets, list) or not targets:
        raise ManifestError("Target registry must contain at least one target.")

    ids: set[str] = set()
    flags: set[str] = set()
    suffixes: set[str] = set()
    for target in targets:
        if not isinstance(target, dict) or set(target) != REQUIRED_TARGET_KEYS:
            raise ManifestError("Every target must contain exactly the required registry fields.")
        if target["chipFamily"] not in CHIP_FAMILIES:
            raise ManifestError(f"Unsupported chip family for {target['id']}.")
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", target["id"]):
            raise ManifestError(f"Invalid target id: {target['id']}.")
        if not isinstance(target["aliases"], list):
            raise ManifestError(f"Aliases must be an array for {target['id']}.")
        for value, seen, label in (
            (target["id"], ids, "target id"),
            (target["buildFlag"], flags, "build flag"),
            (target["assetSuffix"], suffixes, "asset suffix"),
        ):
            if value in seen:
                raise ManifestError(f"Duplicate {label}: {value}.")
            seen.add(value)

    return registry


def target_for_flag(registry: dict[str, Any], build_flag: str) -> dict[str, Any]:
    matches = [target for target in registry["targets"] if target["buildFlag"] == build_flag]
    if len(matches) != 1:
        raise ManifestError(f"Expected exactly one registry target for {build_flag}.")
    return matches[0]


def parse_workflow_flow_mapping(body: str, line_number: int) -> dict[str, Any]:
    values: dict[str, Any] = {}
    position = 0
    while position < len(body):
        match = FLOW_FIELD_PATTERN.match(body, position)
        if match is None:
            raise ManifestError(
                f"Unsupported build matrix syntax on workflow line {line_number}."
            )
        key = match.group("key")
        if key in values:
            raise ManifestError(f"Duplicate build matrix field {key} on line {line_number}.")
        values[key] = (
            match.group("string")
            if match.group("string") is not None
            else match.group("boolean") == "true"
        )
        position = match.end()
    return values


def load_normal_build_matrix(
    workflow_path: Path, registry_path: Path
) -> list[dict[str, Any]]:
    try:
        workflow_lines = workflow_path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise ManifestError(f"Cannot read normal build workflow {workflow_path}: {error}") from error

    boards: list[dict[str, Any]] = []
    allowed_keys = REQUIRED_BUILD_MATRIX_KEYS | OPTIONAL_BUILD_MATRIX_KEYS
    for line_number, line in enumerate(workflow_lines, start=1):
        if "flag:" not in line or not line.lstrip().startswith("- {"):
            continue
        match = FLOW_ENTRY_PATTERN.fullmatch(line)
        if match is None:
            raise ManifestError(f"Malformed build matrix entry on workflow line {line_number}.")
        board = parse_workflow_flow_mapping(match.group("body"), line_number)
        missing = REQUIRED_BUILD_MATRIX_KEYS - set(board)
        extra = set(board) - allowed_keys
        if missing or extra:
            raise ManifestError(
                f"Build matrix entry on line {line_number} has missing={sorted(missing)}, "
                f"extra={sorted(extra)}."
            )
        boards.append(board)

    if not boards:
        raise ManifestError(f"No hardware matrix entries found in {workflow_path}.")
    for key in ("name", "flag", "file_name"):
        values = [board[key] for board in boards]
        if len(values) != len(set(values)):
            raise ManifestError(f"Normal build matrix contains duplicate {key} values.")

    registry = load_registry(registry_path)
    registry_flags = {target["buildFlag"] for target in registry["targets"]}
    matrix_flags = {board["flag"] for board in boards}
    if matrix_flags != registry_flags:
        raise ManifestError(
            "Installer target registry does not match the normal build workflow; "
            f"missing={sorted(matrix_flags - registry_flags)}, "
            f"extra={sorted(registry_flags - matrix_flags)}."
        )
    return boards


def parse_size(value: str) -> int:
    match = SIZE_PATTERN.fullmatch(value)
    if not match:
        raise ManifestError(f"Unsupported flash size: {value}.")
    multiplier = {
        None: 1,
        "B": 1,
        "K": 1024,
        "KB": 1024,
        "M": 1024 * 1024,
        "MB": 1024 * 1024,
    }[match.group("unit").upper() if match.group("unit") else None]
    return int(match.group("size")) * multiplier


def role_for_file(path: Path) -> str:
    name = path.name.lower()
    if "bootloader" in name:
        return "bootloader"
    if "partition" in name:
        return "partition-table"
    if "boot_app0" in name or "ota_data" in name:
        return "ota-data"
    if name.endswith(".ino.bin"):
        return "application"
    return "auxiliary"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_flash_args(build_dir: Path) -> tuple[dict[str, str], list[tuple[int, Path]]]:
    properties_path = build_dir / "arduino-build-properties.txt"
    if properties_path.is_file():
        properties: dict[str, str] = {}
        for line in properties_path.read_text(encoding="utf-8").splitlines():
            key, separator, value = line.partition("=")
            if separator:
                properties[key] = value
        try:
            tokens = shlex.split(properties["tools.esptool_py.upload.pattern_args"])
            options = {
                "--flash_mode": properties["build.flash_mode"],
                "--flash_freq": properties["build.flash_freq"],
                "--flash_size": properties["build.flash_size"],
            }
        except KeyError as error:
            raise ManifestError(
                f"{properties_path} is missing required ESP32 upload properties: {error.args[0]}."
            ) from error
        source_description = properties_path
    else:
        candidates = [build_dir / "flash_args", build_dir / "flash_args.txt"]
        flash_args = next((candidate for candidate in candidates if candidate.is_file()), None)
        if flash_args is None:
            raise ManifestError(
                f"No Arduino build properties or flash_args found in {build_dir}."
            )
        tokens = shlex.split(flash_args.read_text(encoding="utf-8"))
        options = {}
        for normalized, spellings in (
            ("--flash_mode", ("--flash_mode", "--flash-mode")),
            ("--flash_freq", ("--flash_freq", "--flash-freq")),
            ("--flash_size", ("--flash_size", "--flash-size")),
        ):
            option = next((spelling for spelling in spellings if spelling in tokens), None)
            try:
                if option is None:
                    raise ValueError
                options[normalized] = tokens[tokens.index(option) + 1]
            except (ValueError, IndexError) as error:
                raise ManifestError(f"{flash_args} does not declare {normalized}.") from error
        source_description = flash_args

    segments: list[tuple[int, Path]] = []
    for index, token in enumerate(tokens[:-1]):
        if not OFFSET_PATTERN.fullmatch(token):
            continue
        raw_path = Path(tokens[index + 1])
        source = raw_path if raw_path.is_absolute() else build_dir / raw_path
        source = source.resolve()
        if not source.is_file():
            raise ManifestError(f"Flash segment does not exist: {source}.")
        segments.append((int(token, 16), source))

    if not segments:
        raise ManifestError(f"{source_description} does not contain any flash segments.")
    if len({offset for offset, _ in segments}) != len(segments):
        raise ManifestError("Flash arguments contain duplicate offsets.")
    if len([path for _, path in segments if role_for_file(path) == "application"]) != 1:
        raise ManifestError("Flash arguments must contain exactly one application image.")
    return options, sorted(segments)


def generate_target_manifest(
    registry_path: Path,
    build_flag: str,
    build_dir: Path,
    version: str,
    release_date: str,
    source_commit: str,
    output_dir: Path,
) -> Path:
    if not SHA_PATTERN.fullmatch(source_commit):
        raise ManifestError("sourceCommit must be a full lowercase Git SHA.")
    if not re.fullmatch(r"v[0-9A-Za-z._-]+", version):
        raise ManifestError("Version must begin with v and contain filename-safe characters.")
    if not re.fullmatch(r"\d{8}", release_date):
        raise ManifestError("Release date must use YYYYMMDD.")

    registry = load_registry(registry_path)
    target = target_for_flag(registry, build_flag)
    options, source_segments = parse_flash_args(build_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    version_slug = version.replace(".", "_")
    application_stem = (
        f"esp32_marauder_installer_{version_slug}_{release_date}_{target['assetSuffix']}"
    )
    copied_segments: list[dict[str, Any]] = []
    used_names: set[str] = set()

    for offset, source in source_segments:
        role = role_for_file(source)
        destination_name = (
            f"{application_stem}.bin"
            if role == "application"
            else f"{application_stem}.{role}.bin"
        )
        if destination_name in used_names:
            raise ManifestError(f"Multiple segments resolve to {destination_name}.")
        used_names.add(destination_name)
        destination = output_dir / destination_name
        shutil.copyfile(source, destination)
        if destination.stat().st_size == 0:
            raise ManifestError(f"Flash segment is empty: {source}.")
        copied_segments.append(
            {
                "role": role,
                "offset": offset,
                "size": destination.stat().st_size,
                "sha256": sha256(destination),
                "fileName": destination.name,
            }
        )

    flash_size_bytes = parse_size(options["--flash_size"])
    for index, segment in enumerate(copied_segments):
        end = segment["offset"] + segment["size"]
        if end > flash_size_bytes:
            raise ManifestError(f"{segment['role']} exceeds the declared flash size.")
        next_segment = (
            copied_segments[index + 1] if index + 1 < len(copied_segments) else None
        )
        if next_segment and end > next_segment["offset"]:
            raise ManifestError(
                f"{segment['role']} overlaps {next_segment['role']} in flash arguments."
            )

    application = [segment for segment in copied_segments if segment["role"] == "application"]
    target_manifest = {
        "schemaVersion": SCHEMA_VERSION,
        "kind": "esp32-marauder-installer-target",
        "metadataStatus": "authoritative",
        "sourceRepository": SOURCE_REPOSITORY,
        "sourceCommit": source_commit,
        "channel": "stable",
        "version": version,
        "target": target,
        "flash": {
            "sizeBytes": flash_size_bytes,
            "mode": options["--flash_mode"],
            "frequency": options["--flash_freq"],
            "update": {
                "erase": False,
                "preservesUserData": True,
                "segments": application,
            },
            "factory": {
                "erase": True,
                "preservesUserData": False,
                "segments": copied_segments,
            },
        },
    }
    output_path = output_dir / f"{target['id']}.installer.json"
    output_path.write_text(json.dumps(target_manifest, indent=2) + "\n", encoding="utf-8")
    return output_path


def combine_manifests(registry_path: Path, input_dir: Path, output_path: Path) -> Path:
    registry = load_registry(registry_path)
    paths = sorted(input_dir.glob("*.installer.json"))
    manifests = [read_json(path) for path in paths]
    expected_ids = {target["id"] for target in registry["targets"]}
    actual_ids = {
        manifest.get("target", {}).get("id")
        for manifest in manifests
        if isinstance(manifest.get("target"), dict)
    }
    if actual_ids != expected_ids:
        missing = sorted(expected_ids - actual_ids)
        extra = sorted(actual_ids - expected_ids)
        raise ManifestError(
            f"Release target coverage mismatch; missing={missing}, extra={extra}."
        )

    versions = {manifest.get("version") for manifest in manifests}
    commits = {manifest.get("sourceCommit") for manifest in manifests}
    channels = {manifest.get("channel") for manifest in manifests}
    kinds = {manifest.get("kind") for manifest in manifests}
    statuses = {manifest.get("metadataStatus") for manifest in manifests}
    if len(versions) != 1 or len(commits) != 1 or channels != {"stable"}:
        raise ManifestError("Target manifests disagree on version, commit, or stable channel.")
    if kinds != {"esp32-marauder-installer-target"}:
        raise ManifestError("Unexpected target manifest kind.")
    if statuses != {"authoritative"}:
        raise ManifestError("Target metadata is not authoritative.")

    release_manifest = {
        "$schema": (
            "https://raw.githubusercontent.com/"
            f"{SOURCE_REPOSITORY}/{next(iter(commits))}/"
            "installer/firmware-manifest.schema.json"
        ),
        "schemaVersion": SCHEMA_VERSION,
        "kind": "esp32-marauder-installer-release",
        "metadataStatus": "authoritative",
        "sourceRepository": SOURCE_REPOSITORY,
        "sourceCommit": commits.pop(),
        "channel": "stable",
        "version": versions.pop(),
        "targets": sorted(
            (manifest["target"] | {"flash": manifest["flash"]} for manifest in manifests),
            key=lambda target: target["id"],
        ),
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(release_manifest, indent=2) + "\n", encoding="utf-8")
    return output_path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--registry", type=Path, default=Path("installer/targets.json"))
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--validate-registry", action="store_true")
    action.add_argument("--generate-target", action="store_true")
    action.add_argument("--combine", action="store_true")
    action.add_argument("--export-build-matrix", action="store_true")
    parser.add_argument(
        "--workflow",
        type=Path,
        default=Path(".github/workflows/build_parallel.yml"),
    )
    parser.add_argument("--build-flag")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--version")
    parser.add_argument("--release-date")
    parser.add_argument("--source-commit")
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--input-dir", type=Path)
    parser.add_argument("--output", type=Path)
    return parser


def require(value: Any, name: str) -> Any:
    if value is None:
        raise ManifestError(f"{name} is required for this operation.")
    return value


def main() -> None:
    args = build_parser().parse_args()
    if args.validate_registry:
        registry = load_registry(args.registry)
        print(f"Validated {len(registry['targets'])} installer targets.")
        return
    if args.export_build_matrix:
        boards = load_normal_build_matrix(args.workflow, args.registry)
        print(json.dumps({"include": boards}, separators=(",", ":")))
        return
    if args.generate_target:
        output = generate_target_manifest(
            args.registry,
            require(args.build_flag, "--build-flag"),
            require(args.build_dir, "--build-dir"),
            require(args.version, "--version"),
            require(args.release_date, "--release-date"),
            require(args.source_commit, "--source-commit"),
            require(args.output_dir, "--output-dir"),
        )
        print(output)
        return
    output = combine_manifests(
        args.registry,
        require(args.input_dir, "--input-dir"),
        require(args.output, "--output"),
    )
    print(output)


if __name__ == "__main__":
    try:
        main()
    except ManifestError as error:
        raise SystemExit(f"installer manifest error: {error}") from error
