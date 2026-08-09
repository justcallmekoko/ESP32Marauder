#!/usr/bin/env python3
"""Build a deterministic offline IEEE assignment database for Marauder.

The converter accepts the four IEEE CSV registries from local files. Network
access is disabled by default and is only used when ``--download`` is passed.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import os
import struct
import sys
import tempfile
import unicodedata
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Mapping, Sequence


MAGIC = b"MROUI001"
FORMAT_VERSION = 1
HEADER_SIZE = 24
PREFIX_SIZE = 5
NAME_SIZE = 24
RECORD_SIZE = PREFIX_SIZE + NAME_SIZE
MAX_NAME_BYTES = NAME_SIZE - 1
MAX_DOWNLOAD_BYTES = 64 * 1024 * 1024
MAX_CONFLICT_WARNINGS = 20
DEFAULT_OUTPUT = Path("marauder_oui.bin")

# 8-byte magic, four one-byte format fields, then /24, /28 and /36 counts.
# The three counts are little-endian. Prefixes in records are big-endian.
HEADER_STRUCT = struct.Struct("<8sBBBBIII")


class OuiDatabaseError(RuntimeError):
    """Raised when source data cannot be converted without ambiguity."""


@dataclass(frozen=True)
class RegistrySpec:
    key: str
    registry_name: str
    prefix_bits: int
    url: str


REGISTRY_SPECS: tuple[RegistrySpec, ...] = (
    RegistrySpec(
        key="ma_l",
        registry_name="MA-L",
        prefix_bits=24,
        url="https://standards-oui.ieee.org/oui/oui.csv",
    ),
    RegistrySpec(
        key="ma_m",
        registry_name="MA-M",
        prefix_bits=28,
        url="https://standards-oui.ieee.org/oui28/mam.csv",
    ),
    RegistrySpec(
        key="ma_s",
        registry_name="MA-S",
        prefix_bits=36,
        url="https://standards-oui.ieee.org/oui36/oui36.csv",
    ),
    RegistrySpec(
        key="iab",
        registry_name="IAB",
        prefix_bits=36,
        url="https://standards-oui.ieee.org/iab/iab.csv",
    ),
)
REGISTRY_BY_KEY = {spec.key: spec for spec in REGISTRY_SPECS}


@dataclass(frozen=True)
class CsvSource:
    spec: RegistrySpec
    label: str
    payload: bytes


@dataclass(frozen=True, order=True)
class OuiRecord:
    prefix_bits: int
    prefix_value: int
    organization_name: str


@dataclass(frozen=True)
class OuiConflict:
    prefix_bits: int
    prefix_value: int
    # Each choice is (normalized organization name, sorted source labels).
    choices: tuple[tuple[str, tuple[str, ...]], ...]

    @property
    def assignment(self) -> str:
        return f"{self.prefix_value:0{self.prefix_bits // 4}X}"

    def describe(self) -> str:
        organizations = "; ".join(
            f"{name!r} ({', '.join(labels)})" for name, labels in self.choices
        )
        return f"/{self.prefix_bits} {self.assignment}: {organizations}"


@dataclass(frozen=True)
class BuildSummary:
    output: Path
    count_24: int
    count_28: int
    count_36: int
    conflicts: tuple[OuiConflict, ...]
    sha256: str

    @property
    def record_count(self) -> int:
        return self.count_24 + self.count_28 + self.count_36

    @property
    def conflict_count(self) -> int:
        return len(self.conflicts)


def normalize_organization_name(value: str) -> str:
    """Return deterministic printable ASCII without applying truncation."""

    normalized = unicodedata.normalize("NFKD", value)
    output: list[str] = []
    pending_space = False

    for character in normalized:
        if unicodedata.combining(character):
            continue
        if character.isspace():
            pending_space = bool(output)
            continue
        if pending_space:
            output.append(" ")
            pending_space = False
        codepoint = ord(character)
        if 0x21 <= codepoint <= 0x7E:
            output.append(character)
        elif codepoint == 0x20:
            output.append(" ")
        else:
            output.append("?")

    result = "".join(output).strip()
    if not result:
        raise OuiDatabaseError("Organization name is empty after ASCII normalization.")
    return result


def encode_organization_name(value: str) -> bytes:
    """Encode a normalized name as 23 printable bytes plus NUL padding."""

    normalized = normalize_organization_name(value)
    truncated = normalized[:MAX_NAME_BYTES].rstrip()
    if not truncated:
        raise OuiDatabaseError("Organization name is empty after truncation.")
    encoded = truncated.encode("ascii", errors="strict")
    if any(byte < 0x20 or byte > 0x7E for byte in encoded):
        raise OuiDatabaseError("Organization name contains non-printable ASCII.")
    return encoded + bytes(NAME_SIZE - len(encoded))


def canonical_field_name(value: str | None) -> str:
    if value is None:
        return ""
    return "".join(character.lower() for character in value if character.isalnum())


def resolve_csv_fields(fieldnames: Sequence[str | None] | None, label: str) -> dict[str, str]:
    if not fieldnames:
        raise OuiDatabaseError(f"{label}: CSV header is missing.")

    canonical = {canonical_field_name(name): name for name in fieldnames if name is not None}
    required = {
        "assignment": "Assignment",
        "organizationname": "Organization Name",
    }
    missing = [display for key, display in required.items() if key not in canonical]
    if missing:
        raise OuiDatabaseError(f"{label}: missing CSV column(s): {', '.join(missing)}.")

    fields = {
        "assignment": canonical["assignment"],
        "organization": canonical["organizationname"],
    }
    if "registry" in canonical:
        fields["registry"] = canonical["registry"]
    return fields


def parse_assignment(value: str, spec: RegistrySpec, label: str, row_number: int) -> int:
    assignment = value.strip()
    expected_digits = spec.prefix_bits // 4
    if len(assignment) != expected_digits:
        raise OuiDatabaseError(
            f"{label}:{row_number}: {spec.registry_name} assignment must contain "
            f"exactly {expected_digits} hexadecimal digits."
        )
    try:
        parsed = int(assignment, 16)
    except ValueError as error:
        raise OuiDatabaseError(
            f"{label}:{row_number}: assignment is not hexadecimal: {assignment!r}."
        ) from error
    if parsed >= (1 << spec.prefix_bits):
        raise OuiDatabaseError(f"{label}:{row_number}: assignment exceeds prefix width.")
    return parsed


def parse_csv_source(source: CsvSource) -> list[OuiRecord]:
    try:
        text = source.payload.decode("utf-8-sig")
    except UnicodeDecodeError as error:
        raise OuiDatabaseError(f"{source.label}: CSV is not valid UTF-8.") from error

    reader = csv.DictReader(io.StringIO(text, newline=""))
    fields = resolve_csv_fields(reader.fieldnames, source.label)
    records: list[OuiRecord] = []

    for row_number, row in enumerate(reader, start=2):
        if row is None:
            continue
        if any(isinstance(value, list) for value in row.values()):
            raise OuiDatabaseError(
                f"{source.label}:{row_number}: CSV row has more values than header columns."
            )
        if not any((value or "").strip() for value in row.values()):
            continue

        if "registry" in fields:
            registry = (row.get(fields["registry"]) or "").strip()
            if registry and registry.casefold() != source.spec.registry_name.casefold():
                raise OuiDatabaseError(
                    f"{source.label}:{row_number}: expected registry "
                    f"{source.spec.registry_name}, got {registry!r}."
                )

        assignment = parse_assignment(
            row.get(fields["assignment"]) or "",
            source.spec,
            source.label,
            row_number,
        )
        try:
            organization = normalize_organization_name(
                row.get(fields["organization"]) or ""
            )
        except OuiDatabaseError as error:
            raise OuiDatabaseError(f"{source.label}:{row_number}: {error}") from error

        records.append(
            OuiRecord(
                prefix_bits=source.spec.prefix_bits,
                prefix_value=assignment,
                organization_name=organization,
            )
        )

    if not records:
        raise OuiDatabaseError(f"{source.label}: CSV contains no assignment records.")
    return records


def format_conflicts(conflicts: Sequence[OuiConflict], limit: int) -> str:
    displayed = conflicts[:limit]
    details = "\n".join(f"  - {conflict.describe()}" for conflict in displayed)
    remaining = len(conflicts) - len(displayed)
    if remaining:
        details += f"\n  - ... {remaining} additional conflict(s) omitted"
    return details


def merge_records(
    sources: Iterable[CsvSource], *, strict_conflicts: bool = False
) -> tuple[list[OuiRecord], tuple[OuiConflict, ...]]:
    candidates: dict[tuple[int, int], dict[str, set[str]]] = {}

    for source in sources:
        for record in parse_csv_source(source):
            key = (record.prefix_bits, record.prefix_value)
            organizations = candidates.setdefault(key, {})
            organizations.setdefault(record.organization_name, set()).add(source.label)

    records: list[OuiRecord] = []
    conflicts: list[OuiConflict] = []
    for (prefix_bits, prefix_value), organizations in candidates.items():
        if len(organizations) == 1:
            organization_name = next(iter(organizations))
            records.append(
                OuiRecord(
                    prefix_bits=prefix_bits,
                    prefix_value=prefix_value,
                    organization_name=organization_name,
                )
            )
            continue
        choices = tuple(
            (name, tuple(sorted(labels)))
            for name, labels in sorted(organizations.items())
        )
        conflicts.append(
            OuiConflict(
                prefix_bits=prefix_bits,
                prefix_value=prefix_value,
                choices=choices,
            )
        )

    records.sort(key=lambda record: (record.prefix_bits, record.prefix_value))
    conflicts.sort(key=lambda conflict: (conflict.prefix_bits, conflict.prefix_value))
    frozen_conflicts = tuple(conflicts)
    if strict_conflicts and frozen_conflicts:
        raise OuiDatabaseError(
            f"Found {len(frozen_conflicts)} conflicting assignment(s):\n"
            f"{format_conflicts(frozen_conflicts, MAX_CONFLICT_WARNINGS)}"
        )
    if not records:
        raise OuiDatabaseError("No OUI records were produced.")
    return records, frozen_conflicts


def encode_prefix(record: OuiRecord) -> bytes:
    if record.prefix_bits not in (24, 28, 36):
        raise OuiDatabaseError(f"Unsupported prefix width: /{record.prefix_bits}.")
    shifted = record.prefix_value << (PREFIX_SIZE * 8 - record.prefix_bits)
    return shifted.to_bytes(PREFIX_SIZE, byteorder="big", signed=False)


def build_database_blob(
    sources: Iterable[CsvSource], *, strict_conflicts: bool = False
) -> tuple[bytes, tuple[int, int, int], tuple[OuiConflict, ...]]:
    records, conflicts = merge_records(sources, strict_conflicts=strict_conflicts)
    count_24 = sum(record.prefix_bits == 24 for record in records)
    count_28 = sum(record.prefix_bits == 28 for record in records)
    count_36 = sum(record.prefix_bits == 36 for record in records)

    header = HEADER_STRUCT.pack(
        MAGIC,
        FORMAT_VERSION,
        RECORD_SIZE,
        NAME_SIZE,
        0,
        count_24,
        count_28,
        count_36,
    )
    if len(header) != HEADER_SIZE:
        raise AssertionError("Internal header size mismatch.")

    body = bytearray()
    for record in records:
        body.extend(encode_prefix(record))
        body.extend(encode_organization_name(record.organization_name))

    expected_size = HEADER_SIZE + len(records) * RECORD_SIZE
    blob = header + bytes(body)
    if len(blob) != expected_size:
        raise AssertionError("Internal database size mismatch.")
    return blob, (count_24, count_28, count_36), conflicts


def write_database(
    sources: Iterable[CsvSource],
    output: Path,
    *,
    strict_conflicts: bool = False,
) -> BuildSummary:
    blob, counts, conflicts = build_database_blob(
        sources, strict_conflicts=strict_conflicts
    )
    output = output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)

    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            prefix=f".{output.name}.",
            suffix=".tmp",
            dir=output.parent,
            delete=False,
        ) as temporary:
            temporary.write(blob)
            temporary.flush()
            os.fsync(temporary.fileno())
            temporary_path = Path(temporary.name)
        os.replace(temporary_path, output)
        temporary_path = None
    except OSError as error:
        raise OuiDatabaseError(f"Cannot write {output}: {error}") from error
    finally:
        if temporary_path is not None:
            try:
                temporary_path.unlink()
            except FileNotFoundError:
                pass

    return BuildSummary(
        output=output,
        count_24=counts[0],
        count_28=counts[1],
        count_36=counts[2],
        conflicts=conflicts,
        sha256=hashlib.sha256(blob).hexdigest(),
    )


def read_local_source(spec: RegistrySpec, path: Path) -> CsvSource:
    try:
        payload = path.read_bytes()
    except OSError as error:
        raise OuiDatabaseError(f"Cannot read {path}: {error}") from error
    return CsvSource(spec=spec, label=str(path), payload=payload)


def download_source(spec: RegistrySpec, timeout: float) -> CsvSource:
    request = urllib.request.Request(
        spec.url,
        headers={"User-Agent": "ESP32Marauder-OUI-Builder/1"},
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = response.read(MAX_DOWNLOAD_BYTES + 1)
    except OSError as error:
        raise OuiDatabaseError(f"Cannot download {spec.url}: {error}") from error
    if len(payload) > MAX_DOWNLOAD_BYTES:
        raise OuiDatabaseError(f"Download exceeds {MAX_DOWNLOAD_BYTES} bytes: {spec.url}")
    return CsvSource(spec=spec, label=spec.url, payload=payload)


def sources_from_paths(paths: Mapping[str, Path]) -> list[CsvSource]:
    return [read_local_source(spec, paths[spec.key]) for spec in REGISTRY_SPECS]


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Build Marauder's deterministic offline IEEE OUI database."
    )
    parser.add_argument(
        "--download",
        action="store_true",
        help="explicitly download all four current CSVs from IEEE over HTTPS",
    )
    parser.add_argument("--ma-l", type=Path, help="local IEEE MA-L (/24) CSV")
    parser.add_argument("--ma-m", type=Path, help="local IEEE MA-M (/28) CSV")
    parser.add_argument("--ma-s", type=Path, help="local IEEE MA-S (/36) CSV")
    parser.add_argument("--iab", type=Path, help="local IEEE IAB (/36) CSV")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"output path (default: {DEFAULT_OUTPUT})",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=30.0,
        help="per-download timeout in seconds (default: 30)",
    )
    parser.add_argument(
        "--strict-conflicts",
        action="store_true",
        help="abort instead of omitting prefixes assigned to conflicting names",
    )
    return parser


def validate_arguments(
    parser: argparse.ArgumentParser, args: argparse.Namespace
) -> Mapping[str, Path] | None:
    local_values = {
        "ma_l": args.ma_l,
        "ma_m": args.ma_m,
        "ma_s": args.ma_s,
        "iab": args.iab,
    }
    supplied = {key: value for key, value in local_values.items() if value is not None}

    if args.download and supplied:
        parser.error("--download cannot be combined with local CSV paths")
    if not args.download and len(supplied) != len(REGISTRY_SPECS):
        parser.error("provide all four local CSV paths or use explicit --download")
    if args.timeout <= 0:
        parser.error("--timeout must be greater than zero")
    if args.download:
        return None
    paths = {key: value for key, value in local_values.items() if value is not None}
    output = args.output.resolve()
    if any(path.resolve() == output for path in paths.values()):
        parser.error("--output must not overwrite an input CSV")
    return paths


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    local_paths = validate_arguments(parser, args)

    try:
        if args.download:
            sources = [download_source(spec, args.timeout) for spec in REGISTRY_SPECS]
        else:
            if local_paths is None:
                raise AssertionError("Validated local paths are missing.")
            sources = sources_from_paths(local_paths)
        summary = write_database(
            sources,
            args.output,
            strict_conflicts=args.strict_conflicts,
        )
    except OuiDatabaseError as error:
        parser.exit(1, f"error: {error}\n")

    if summary.conflicts:
        for conflict in summary.conflicts[:MAX_CONFLICT_WARNINGS]:
            print(
                f"warning: omitted conflicting assignment {conflict.describe()}",
                file=sys.stderr,
            )
        remaining = summary.conflict_count - min(
            summary.conflict_count, MAX_CONFLICT_WARNINGS
        )
        if remaining:
            print(
                f"warning: {remaining} additional conflict(s) omitted; "
                f"total conflicts={summary.conflict_count}",
                file=sys.stderr,
            )

    print(
        f"Wrote {summary.record_count} records "
        f"(/24={summary.count_24}, /28={summary.count_28}, /36={summary.count_36}) "
        f"with conflicts={summary.conflict_count} to {summary.output}"
    )
    print(f"SHA-256: {summary.sha256}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
