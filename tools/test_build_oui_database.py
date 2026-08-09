from __future__ import annotations

import contextlib
import io
import struct
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools.build_oui_database import (
    FORMAT_VERSION,
    HEADER_SIZE,
    HEADER_STRUCT,
    MAGIC,
    NAME_SIZE,
    RECORD_SIZE,
    REGISTRY_BY_KEY,
    CsvSource,
    OuiDatabaseError,
    build_database_blob,
    encode_organization_name,
    main,
    normalize_organization_name,
)


def csv_payload(registry: str, rows: list[tuple[str, str]]) -> bytes:
    lines = ["Registry,Assignment,Organization Name,Organization Address"]
    lines.extend(
        f'{registry},{assignment},"{organization}",Test Address'
        for assignment, organization in rows
    )
    return ("\n".join(lines) + "\n").encode("utf-8")


def source(key: str, rows: list[tuple[str, str]]) -> CsvSource:
    spec = REGISTRY_BY_KEY[key]
    return CsvSource(
        spec=spec,
        label=f"synthetic-{key}.csv",
        payload=csv_payload(spec.registry_name, rows),
    )


def decode_records(blob: bytes) -> list[tuple[bytes, bytes]]:
    records: list[tuple[bytes, bytes]] = []
    for offset in range(HEADER_SIZE, len(blob), RECORD_SIZE):
        record = blob[offset : offset + RECORD_SIZE]
        records.append((record[:5], record[5:]))
    return records


class OuiDatabaseBuilderTests(unittest.TestCase):
    def test_builds_documented_header_groups_and_left_aligned_prefixes(self) -> None:
        sources = [
            source(
                "ma_l",
                [
                    ("AABBCC", "Zeta Devices"),
                    ("001122", "Café Devices"),
                    ("001122", "Café Devices"),
                ],
            ),
            source("ma_m", [("0011223", "Middle Devices")]),
            source("ma_s", [("001122334", "Small Devices")]),
            source("iab", [("001122335", "Legacy Devices")]),
        ]

        blob, counts, conflicts = build_database_blob(sources)

        self.assertEqual(counts, (2, 1, 2))
        self.assertEqual(conflicts, ())
        self.assertEqual(len(blob), HEADER_SIZE + 5 * RECORD_SIZE)
        self.assertEqual(
            HEADER_STRUCT.unpack_from(blob),
            (MAGIC, FORMAT_VERSION, RECORD_SIZE, NAME_SIZE, 0, 2, 1, 2),
        )

        records = decode_records(blob)
        self.assertEqual(
            [prefix for prefix, _name in records],
            [
                bytes.fromhex("0011220000"),
                bytes.fromhex("AABBCC0000"),
                bytes.fromhex("0011223000"),
                bytes.fromhex("0011223340"),
                bytes.fromhex("0011223350"),
            ],
        )
        first_name = records[0][1].split(b"\0", 1)[0]
        self.assertEqual(first_name, b"Cafe Devices")

    def test_output_is_deterministic_for_reordered_sources_and_rows(self) -> None:
        forward = [
            source("ma_l", [("001122", "Alpha"), ("AABBCC", "Beta")]),
            source("ma_m", [("0011223", "Gamma")]),
            source("ma_s", [("001122334", "Delta")]),
            source("iab", [("001122335", "Epsilon")]),
        ]
        reverse = [
            source("iab", [("001122335", "Epsilon")]),
            source("ma_s", [("001122334", "Delta")]),
            source("ma_m", [("0011223", "Gamma")]),
            source("ma_l", [("AABBCC", "Beta"), ("001122", "Alpha")]),
        ]

        self.assertEqual(build_database_blob(forward)[0], build_database_blob(reverse)[0])

    def test_identical_ma_s_and_iab_assignment_is_deduplicated(self) -> None:
        sources = [
            source("ma_l", [("001122", "Alpha")]),
            source("ma_m", [("0011223", "Beta")]),
            source("ma_s", [("001122334", "Same Organization")]),
            source("iab", [("001122334", "Same Organization")]),
        ]

        blob, counts, conflicts = build_database_blob(sources)

        self.assertEqual(counts, (1, 1, 1))
        self.assertEqual(conflicts, ())
        self.assertEqual(len(blob), HEADER_SIZE + 3 * RECORD_SIZE)

    def test_conflicting_duplicate_assignment_is_omitted_completely(self) -> None:
        sources = [
            source("ma_l", [("001122", "Alpha")]),
            source("ma_m", [("0011223", "Beta")]),
            source("ma_s", [("001122334", "First Organization")]),
            source("iab", [("001122334", "Second Organization")]),
        ]

        blob, counts, conflicts = build_database_blob(sources)

        self.assertEqual(counts, (1, 1, 0))
        self.assertEqual(len(blob), HEADER_SIZE + 2 * RECORD_SIZE)
        self.assertEqual(len(conflicts), 1)
        self.assertEqual(conflicts[0].prefix_bits, 36)
        self.assertEqual(conflicts[0].assignment, "001122334")
        self.assertNotIn(bytes.fromhex("0011223340"), blob[HEADER_SIZE:])

    def test_strict_conflicts_aborts_without_selecting_an_organization(self) -> None:
        sources = [
            source("ma_l", [("001122", "Alpha")]),
            source("ma_m", [("0011223", "Beta")]),
            source("ma_s", [("001122334", "First Organization")]),
            source("iab", [("001122334", "Second Organization")]),
        ]

        with self.assertRaisesRegex(OuiDatabaseError, "Found 1 conflicting assignment"):
            build_database_blob(sources, strict_conflicts=True)

    def test_rejects_bad_assignment_width_hex_and_registry(self) -> None:
        valid_tail = [
            source("ma_m", [("0011223", "Beta")]),
            source("ma_s", [("001122334", "Gamma")]),
            source("iab", [("001122335", "Delta")]),
        ]

        with self.assertRaisesRegex(OuiDatabaseError, "exactly 6 hexadecimal digits"):
            build_database_blob([source("ma_l", [("1122", "Alpha")]), *valid_tail])
        with self.assertRaisesRegex(OuiDatabaseError, "not hexadecimal"):
            build_database_blob([source("ma_l", [("00GG22", "Alpha")]), *valid_tail])

        wrong_registry = CsvSource(
            spec=REGISTRY_BY_KEY["ma_l"],
            label="wrong-registry.csv",
            payload=csv_payload("MA-M", [("001122", "Alpha")]),
        )
        with self.assertRaisesRegex(OuiDatabaseError, "expected registry MA-L"):
            build_database_blob([wrong_registry, *valid_tail])

    def test_name_normalization_is_printable_nul_terminated_and_bounded(self) -> None:
        normalized = normalize_organization_name(
            "  Müller\t设备  Incorporated With A Very Long Name  "
        )
        self.assertEqual(
            normalized,
            "Muller ?? Incorporated With A Very Long Name",
        )

        encoded = encode_organization_name(normalized)

        self.assertEqual(len(encoded), NAME_SIZE)
        self.assertIn(0, encoded)
        name = encoded.split(b"\0", 1)[0]
        self.assertLessEqual(len(name), NAME_SIZE - 1)
        self.assertTrue(all(0x20 <= byte <= 0x7E for byte in name))
        self.assertEqual(encoded[-1], 0)

    def test_cli_requires_all_local_files_and_never_downloads_implicitly(self) -> None:
        with mock.patch(
            "tools.build_oui_database.urllib.request.urlopen"
        ) as urlopen:
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as context:
                    main(["--ma-l", "only-one.csv"])
            self.assertEqual(context.exception.code, 2)
            urlopen.assert_not_called()

    def test_cli_writes_database_from_four_local_csvs(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths: dict[str, Path] = {}
            assignments = {
                "ma_l": "001122",
                "ma_m": "0011223",
                "ma_s": "001122334",
                "iab": "001122335",
            }
            for key, assignment in assignments.items():
                spec = REGISTRY_BY_KEY[key]
                path = root / f"{key}.csv"
                path.write_bytes(csv_payload(spec.registry_name, [(assignment, key)]))
                paths[key] = path
            output = root / "custom.bin"

            with contextlib.redirect_stdout(io.StringIO()):
                result = main(
                    [
                        "--ma-l",
                        str(paths["ma_l"]),
                        "--ma-m",
                        str(paths["ma_m"]),
                        "--ma-s",
                        str(paths["ma_s"]),
                        "--iab",
                        str(paths["iab"]),
                        "--output",
                        str(output),
                    ]
                )

            self.assertEqual(result, 0)
            blob = output.read_bytes()
            self.assertEqual(blob[:8], MAGIC)
            self.assertEqual(struct.unpack_from("<III", blob, 12), (1, 1, 2))

    def test_cli_refuses_to_overwrite_an_input_csv(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            paths: dict[str, Path] = {}
            assignments = {
                "ma_l": "001122",
                "ma_m": "0011223",
                "ma_s": "001122334",
                "iab": "001122335",
            }
            for key, assignment in assignments.items():
                spec = REGISTRY_BY_KEY[key]
                path = root / f"{key}.csv"
                path.write_bytes(csv_payload(spec.registry_name, [(assignment, key)]))
                paths[key] = path

            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as context:
                    main(
                        [
                            "--ma-l",
                            str(paths["ma_l"]),
                            "--ma-m",
                            str(paths["ma_m"]),
                            "--ma-s",
                            str(paths["ma_s"]),
                            "--iab",
                            str(paths["iab"]),
                            "--output",
                            str(paths["ma_l"]),
                        ]
                    )

            self.assertEqual(context.exception.code, 2)

    def test_cli_warns_reports_and_omits_conflicts(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rows = {
                "ma_l": [("001122", "Alpha")],
                "ma_m": [("0011223", "Beta")],
                "ma_s": [("001122334", "First Organization")],
                "iab": [("001122334", "Second Organization")],
            }
            paths: dict[str, Path] = {}
            for key, registry_rows in rows.items():
                spec = REGISTRY_BY_KEY[key]
                path = root / f"{key}.csv"
                path.write_bytes(csv_payload(spec.registry_name, registry_rows))
                paths[key] = path
            output = root / "with-conflict.bin"
            arguments = [
                "--ma-l",
                str(paths["ma_l"]),
                "--ma-m",
                str(paths["ma_m"]),
                "--ma-s",
                str(paths["ma_s"]),
                "--iab",
                str(paths["iab"]),
                "--output",
                str(output),
            ]
            stdout = io.StringIO()
            stderr = io.StringIO()

            with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                result = main(arguments)

            self.assertEqual(result, 0)
            self.assertIn("conflicts=1", stdout.getvalue())
            self.assertIn("omitted conflicting assignment /36 001122334", stderr.getvalue())
            self.assertEqual(struct.unpack_from("<III", output.read_bytes(), 12), (1, 1, 0))

            strict_output = root / "strict.bin"
            strict_stderr = io.StringIO()
            with contextlib.redirect_stderr(strict_stderr):
                with self.assertRaises(SystemExit) as context:
                    main([*arguments[:-1], str(strict_output), "--strict-conflicts"])
            self.assertEqual(context.exception.code, 1)
            self.assertIn("Found 1 conflicting assignment", strict_stderr.getvalue())
            self.assertFalse(strict_output.exists())


if __name__ == "__main__":
    unittest.main()
