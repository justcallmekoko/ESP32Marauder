from __future__ import annotations

import csv
import json
import struct
import tempfile
import unittest
import zipfile
from pathlib import Path

from tools.recon_report import ReconReportError, convert, read_observations, read_probes, read_relationships


class ReconReportTests(unittest.TestCase):
    @staticmethod
    def make_mission(root: Path) -> Path:
        mission = root / "m0042"
        mission.mkdir()
        (mission / "session.json").write_text(
            json.dumps(
                {
                    "schema": 1,
                    "state": "complete",
                    "mode": "wifi",
                    "duration_ms": 65000,
                }
            ),
            encoding="utf-8",
        )
        records = [
            struct.pack(
                "<Iii6sbBc",
                1500,
                38856850,
                -77225850,
                bytes.fromhex("001122334455"),
                -42,
                6,
                b"a",
            ),
            struct.pack(
                "<Iii6sbBc",
                2500,
                0,
                0,
                bytes.fromhex("AABBCCDDEEFF"),
                -70,
                0,
                b"s",
            ),
        ]
        (mission / "obs.rlog").write_bytes(b"RCN1" + b"".join(records))
        probe = struct.pack(
            "<Iii6sbBB24s",
            2000,
            38856860,
            -77225860,
            bytes.fromhex("AABBCCDDEEFF"),
            -55,
            6,
            8,
            b"Marauder".ljust(24, b"\0"),
        )
        (mission / "probes.rlog").write_bytes(b"PRB1" + probe)
        relation = struct.pack("<6s6s", bytes.fromhex("AABBCCDDEEFF"), bytes.fromhex("001122334455"))
        (mission / "relations.rlog").write_bytes(b"REL1" + relation)
        return mission

    def test_decodes_packed_records(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            mission = self.make_mission(Path(temporary))
            observations = read_observations(mission / "obs.rlog")
            self.assertEqual(len(observations), 2)
            self.assertEqual(observations[0].mac, "00:11:22:33:44:55")
            self.assertEqual(observations[0].latitude, 38.85685)
            self.assertEqual(observations[0].longitude, -77.22585)
            self.assertEqual(observations[0].type, "access-point")
            self.assertEqual(observations[0].event, "new")
            self.assertIsNone(observations[1].latitude)

    def test_decodes_probe_names_and_source(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            mission = self.make_mission(Path(temporary))
            probes = read_probes(mission / "probes.rlog")
            self.assertEqual(probes[0].ssid, "Marauder")
            self.assertEqual(probes[0].event, "probe")
            self.assertEqual(probes[0].mac, "AA:BB:CC:DD:EE:FF")
            self.assertEqual(probes[0].latitude, 38.85686)
            self.assertEqual(probes[0].longitude, -77.22586)

    def test_decodes_and_deduplicates_relationships(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            mission = self.make_mission(Path(temporary))
            relation = (mission / "relations.rlog").read_bytes()[4:]
            (mission / "relations.rlog").write_bytes(b"REL1" + relation + relation)
            relationships = read_relationships(mission / "relations.rlog")
            self.assertEqual(len(relationships), 1)
            self.assertEqual(relationships[0].source, "AA:BB:CC:DD:EE:FF")
            self.assertEqual(relationships[0].target, "00:11:22:33:44:55")

    def test_generates_portable_report_and_zip(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            mission = self.make_mission(Path(temporary))
            output = convert(mission, make_zip=True)
            self.assertEqual(
                {item.name for item in output.iterdir()},
                {"observations.csv", "mission.json", "index.html"},
            )
            with (output / "observations.csv").open(encoding="utf-8") as source:
                rows = list(csv.DictReader(source))
            self.assertEqual(rows[1]["type"], "probe-request")
            report = (output / "index.html").read_text(encoding="utf-8")
            self.assertIn("RECON MISSION", report)
            self.assertIn("GPS SIGHTING PLOT", report)
            self.assertIn("MISSION REPLAY", report)
            self.assertIn("OBSERVED RELATIONSHIPS", report)
            self.assertIn("38.856850, -77.225850", report)
            payload = json.loads((output / "mission.json").read_text(encoding="utf-8"))
            self.assertEqual(len(payload["relationships"]), 2)
            archive = mission / "m0042-report.zip"
            self.assertTrue(archive.is_file())
            with zipfile.ZipFile(archive) as bundle:
                self.assertEqual(
                    set(bundle.namelist()),
                    {"observations.csv", "mission.json", "index.html"},
                )

    def test_rejects_truncated_or_unknown_records(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "obs.rlog"
            path.write_bytes(b"RCN1broken")
            with self.assertRaisesRegex(ReconReportError, "incomplete"):
                read_observations(path)
            path.write_bytes(b"NOPE")
            with self.assertRaisesRegex(ReconReportError, "not an RCN1"):
                read_observations(path)


if __name__ == "__main__":
    unittest.main()
