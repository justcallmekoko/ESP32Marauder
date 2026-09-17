#!/usr/bin/env python3
"""Add uninstrumented firmware sources to a Cobertura coverage report.

The native unit-test target deliberately builds only host-testable modules.
This script preserves gcovr's line results for those modules and records each
non-blank line in every other firmware implementation file as uncovered.  The
result is an honest whole-source baseline: it does not imply that hardware
dependent firmware was compiled or exercised on the host.
"""

from __future__ import annotations

import argparse
import os
import xml.etree.ElementTree as ET
from pathlib import Path


def source_lines(path: Path) -> list[int]:
    """Return physical source lines counted for an uninstrumented file.

    Blank lines are excluded.  Comments and preprocessor lines intentionally
    remain included so every non-blank source line has a visible zero-hit
    baseline instead of being silently omitted.
    """

    included_lines: list[int] = []
    excluded = False
    with path.open(encoding="utf-8") as source:
        for number, line in enumerate(source, start=1):
            if "GCOVR_EXCL_START" in line:
                excluded = True
                continue
            if "GCOVR_EXCL_STOP" in line:
                excluded = False
                continue
            if line.strip() and not excluded and "GCOVR_EXCL_LINE" not in line:
                included_lines.append(number)
    return included_lines


def rate(covered: int, valid: int) -> str:
    return f"{covered / valid:.6f}" if valid else "0.0"


def class_filename(class_element: ET.Element) -> str:
    return class_element.attrib.get("filename", "").replace("\\", "/")


def update_metrics(element: ET.Element) -> tuple[int, int]:
    """Recalculate line totals for a Cobertura package or report root."""

    lines = element.findall(".//line")
    valid = len(lines)
    covered = sum(int(line.attrib.get("hits", "0")) > 0 for line in lines)
    element.set("line-rate", rate(covered, valid))
    element.set("lines-covered", str(covered))
    element.set("lines-valid", str(valid))
    return covered, valid


def add_missing_sources(report: Path, source_dir: Path) -> tuple[int, int]:
    tree = ET.parse(report)
    root = tree.getroot()
    classes = root.findall(".//class")
    included = {class_filename(class_element) for class_element in classes}
    package = root.find("./packages/package")
    if package is None:
        raise ValueError("Cobertura report has no package to hold source files")
    classes_element = package.find("classes")
    if classes_element is None:
        classes_element = ET.SubElement(package, "classes")

    added_files = 0
    added_lines = 0
    root_dir = report.parent.resolve()
    for source_path in sorted(source_dir.glob("*.cpp")):
        relative = source_path.resolve().relative_to(root_dir).as_posix()
        if relative in included:
            continue
        class_element = ET.SubElement(
            classes_element,
            "class",
            {
                "name": source_path.stem,
                "filename": relative,
                "line-rate": "0.0",
                "branch-rate": "0.0",
                "complexity": "0.0",
            },
        )
        lines_element = ET.SubElement(class_element, "lines")
        line_numbers = source_lines(source_path)
        for line_number in line_numbers:
            ET.SubElement(lines_element, "line", {"number": str(line_number), "hits": "0"})
        added_files += 1
        added_lines += len(line_numbers)

    for package_element in root.findall("./packages/package"):
        update_metrics(package_element)
    update_metrics(root)
    ET.indent(tree, space="  ")
    tree.write(report, encoding="utf-8", xml_declaration=True)
    return added_files, added_lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path, help="gcovr-generated Cobertura XML report")
    parser.add_argument(
        "--source-dir", type=Path, default=Path("esp32_marauder"), help="firmware source directory"
    )
    args = parser.parse_args()
    added_files, added_lines = add_missing_sources(args.report, args.source_dir)
    print(f"Added {added_files} source files and {added_lines} uncovered source lines to {args.report}")


if __name__ == "__main__":
    main()
