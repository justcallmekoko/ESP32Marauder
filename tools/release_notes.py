from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REGISTRY = REPOSITORY_ROOT / "installer" / "targets.json"


def load_targets(registry_path: Path) -> list[dict[str, Any]]:
    registry = json.loads(registry_path.read_text(encoding="utf-8"))
    targets = registry.get("targets")
    if not isinstance(targets, list) or not targets:
        raise ValueError("Target registry does not contain a non-empty targets list")

    required = {"displayName", "assetSuffix"}
    for target in targets:
        missing = required - target.keys()
        if missing:
            raise ValueError(
                f"Target {target.get('id', '<unknown>')} is missing {sorted(missing)}"
            )
    return targets


def hardware_label(target: dict[str, Any]) -> str:
    aliases = target.get("aliases") or []
    return target["displayName"] + (f" ({', '.join(aliases)})" if aliases else "")


def render_asset_table(targets: list[dict[str, Any]]) -> str:
    rows = [
        "| Hardware | Release binary |",
        "| --- | --- |",
    ]
    rows.extend(
        f"| {hardware_label(target)} | `_{target['assetSuffix']}.bin` |"
        for target in targets
    )
    return "\n".join(rows)


def render_release_notes(targets: list[dict[str, Any]], channel: str) -> str:
    sections = [
        "[justcallmekokollc.com](https://justcallmekokollc.com)",
    ]
    if channel == "nightly":
        sections.append(
            "> **Nightly build:** This automated prerelease is for testing and "
            "evaluation. Features and stored data may change before a stable release."
        )

    sections.extend(
        [
            "## Before installing\n\n"
            "- Match the complete hardware name and binary suffix. Do not substitute "
            "a binary merely because two devices use the same ESP32 family.\n"
            "- See [Install/Update Instructions](https://github.com/justcallmekoko/ESP32Marauder/wiki/update-firmware) "
            "for the supported flashing methods.\n"
            "- See [GPS Modification](https://github.com/justcallmekoko/ESP32Marauder/wiki/gps-modification) "
            "when adding GPS to compatible hardware.\n"
            "- Flipper Zero users should install the current "
            "[Marauder Companion](https://github.com/0xchocolate/flipperzero-wifi-marauder/releases/latest).",
            "> **Flipper hardware:** Flipper Zero WiFi Dev Board and Marauder Dev "
            "Board Pro/BFFB use different firmware targets.",
            "> **Marauder v6:** Check the revision printed on the screen side of the "
            "PCB. Use `_v6.bin` for v6 and `_v6_1.bin` for v6.1/v6.2.",
            "## Firmware assets",
            render_asset_table(targets),
            "ESP32-C5 targets require their documented flash layout. See the "
            "[ESP32-C5 guide](https://github.com/justcallmekoko/ESP32Marauder/wiki/ESP32%E2%80%90C5%E2%80%90DevKitC%E2%80%901) "
            "and [Supported Hardware](https://github.com/justcallmekoko/ESP32Marauder/wiki/Supported-Hardware).",
            "## Additional resources\n\n"
            "- [Project issues](https://github.com/justcallmekoko/ESP32Marauder/issues)\n"
            "- [Marauder wiki](https://github.com/justcallmekoko/ESP32Marauder/wiki)\n"
            "- [JustCallMeKoko Discord](https://discord.com/servers/willstunforfood-776211399918878760)",
        ]
    )
    return "\n\n".join(sections) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate release documentation from the target registry"
    )
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    parser.add_argument("--channel", choices=("stable", "nightly"), default="stable")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    notes = render_release_notes(load_targets(args.registry), args.channel)
    args.output.write_text(notes, encoding="utf-8")


if __name__ == "__main__":
    main()
