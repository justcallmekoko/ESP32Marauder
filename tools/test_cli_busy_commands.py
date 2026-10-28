"""Keeps CliBusy's gated-command list in sync with CommandLine::runCommand()."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "esp32_marauder"


def _gate_block(source: str) -> str:
    marker = "//// WiFi/Bluetooth Scan/Attack commands"
    start = source.index("if (!wifi_scan_obj.scanning()) {", source.index(marker))
    depth = 0
    for index in range(start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError("scan gate block is not closed")


class CliBusyCommandListTests(unittest.TestCase):
    def test_gated_list_matches_the_scan_gate_block(self) -> None:
        cli_cpp = (SRC / "CommandLine.cpp").read_text(encoding="utf-8")
        cli_h = (SRC / "CommandLine.h").read_text(encoding="utf-8")
        busy_cpp = (SRC / "CliBusy.cpp").read_text(encoding="utf-8")

        constants = dict(
            re.findall(r'const char PROGMEM (\w+)\[\] = "([^"]*)";', cli_h)
        )
        names = set(
            re.findall(r"cmd_args\.get\(0\) == (\w+)", _gate_block(cli_cpp))
        )
        expected = {constants[name] for name in names}

        listed_block = busy_cpp[busy_cpp.index("kScanGatedCommands[]") :]
        listed_block = listed_block[: listed_block.index("};")]
        listed = set(re.findall(r'"([^"]+)"', listed_block))

        self.assertEqual(expected, listed)

    def test_refused_commands_are_reported(self) -> None:
        cli_cpp = (SRC / "CommandLine.cpp").read_text(encoding="utf-8")
        self.assertIn(
            "else if (marauder::cliIsScanGatedCommand(cmd_args.get(0).c_str()))",
            cli_cpp,
        )
        self.assertIn("CliBusyReason::WifiConnected", cli_cpp)
        self.assertIn("CliBusyReason::ShowInfo", cli_cpp)


if __name__ == "__main__":
    unittest.main()
