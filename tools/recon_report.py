#!/usr/bin/env python3
"""Convert an ESP32 Marauder Recon mission into portable report files."""

from __future__ import annotations

import argparse
import csv
import html
import json
import struct
import zipfile
from dataclasses import asdict, dataclass
from pathlib import Path

MAGIC = b"RCN1"
RECORD = struct.Struct("<Iii6sbBc")
PROBE_MAGIC = b"PRB1"
PROBE_RECORD = struct.Struct("<Iii6sbBB24s")
RELATIONSHIP_MAGIC = b"REL1"
RELATIONSHIP_RECORD = struct.Struct("<6s6s")
TYPE_NAMES = {"a": "access-point", "s": "station", "b": "ble"}
GPS_COORDINATE_SCALE = 1_000_000


class ReconReportError(ValueError):
    """Raised when a mission cannot be safely converted."""


@dataclass(frozen=True)
class Observation:
    elapsed_ms: int
    latitude: float | None
    longitude: float | None
    mac: str
    rssi: int
    channel: int
    type: str
    event: str
    ssid: str | None


@dataclass(frozen=True)
class Relationship:
    source: str
    target: str
    kind: str
    confidence: str


def _mac(value: bytes) -> str:
    return ":".join(f"{octet:02X}" for octet in value)


def read_observations(path: Path) -> list[Observation]:
    data = path.read_bytes()
    if not data.startswith(MAGIC):
        raise ReconReportError(f"{path} is not an RCN1 observation log")
    payload = data[len(MAGIC) :]
    if len(payload) % RECORD.size:
        raise ReconReportError(f"{path} ends with an incomplete observation")

    observations = []
    for values in RECORD.iter_unpack(payload):
        elapsed, latitude, longitude, mac, rssi, channel, raw_type = values
        kind = raw_type.decode("ascii", errors="replace")
        base_kind = kind.lower()
        if base_kind not in TYPE_NAMES:
            raise ReconReportError(f"Unknown observation type {kind!r}")
        has_position = latitude != 0 or longitude != 0
        observations.append(
            Observation(
                elapsed_ms=elapsed,
                latitude=latitude / GPS_COORDINATE_SCALE if has_position else None,
                longitude=longitude / GPS_COORDINATE_SCALE if has_position else None,
                mac=_mac(mac),
                rssi=rssi,
                channel=channel,
                type=TYPE_NAMES[base_kind],
                event="repeat" if kind.isupper() else "new",
                ssid=None,
            )
        )
    return observations


def read_probes(path: Path) -> list[Observation]:
    data = path.read_bytes()
    if not data.startswith(PROBE_MAGIC):
        raise ReconReportError(f"{path} is not a PRB1 probe log")
    payload = data[len(PROBE_MAGIC) :]
    if len(payload) % PROBE_RECORD.size:
        raise ReconReportError(f"{path} ends with an incomplete probe")
    probes = []
    for values in PROBE_RECORD.iter_unpack(payload):
        elapsed, latitude, longitude, mac, rssi, channel, length, raw_name = values
        if length > len(raw_name):
            raise ReconReportError("Probe name length exceeds record capacity")
        has_position = latitude != 0 or longitude != 0
        probes.append(
            Observation(
                elapsed_ms=elapsed,
                latitude=latitude / GPS_COORDINATE_SCALE if has_position else None,
                longitude=longitude / GPS_COORDINATE_SCALE if has_position else None,
                mac=_mac(mac),
                rssi=rssi,
                channel=channel,
                type="probe-request",
                event="probe",
                ssid=raw_name[:length].decode("utf-8", errors="replace"),
            )
        )
    return probes


def read_relationships(path: Path) -> list[Relationship]:
    data = path.read_bytes()
    if not data.startswith(RELATIONSHIP_MAGIC):
        raise ReconReportError(f"{path} is not a REL1 relationship log")
    payload = data[len(RELATIONSHIP_MAGIC) :]
    if len(payload) % RELATIONSHIP_RECORD.size:
        raise ReconReportError(f"{path} ends with an incomplete relationship")
    unique = {
        (_mac(station), _mac(access_point))
        for station, access_point in RELATIONSHIP_RECORD.iter_unpack(payload)
    }
    return [
        Relationship(station, access_point, "station-to-access-point", "observed")
        for station, access_point in sorted(unique)
    ]


def load_mission(directory: Path) -> tuple[dict, list[Observation], list[Relationship]]:
    manifest_path = directory / "session.json"
    log_path = directory / "obs.rlog"
    if not manifest_path.is_file() or not log_path.is_file():
        raise ReconReportError("Mission must contain session.json and obs.rlog")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as error:
        raise ReconReportError(f"Invalid session.json: {error}") from error
    if manifest.get("schema") != 1:
        raise ReconReportError("Unsupported Recon manifest schema")
    observations = read_observations(log_path)
    probe_path = directory / manifest.get("probes", "probes.rlog")
    if probe_path.is_file():
        observations.extend(read_probes(probe_path))
        observations.sort(key=lambda item: item.elapsed_ms)
    relation_path = directory / manifest.get("relationships", "relations.rlog")
    relationships = read_relationships(relation_path) if relation_path.is_file() else []
    relationships.extend(
        Relationship(item.mac, item.ssid, "client-to-probed-ssid", "observed")
        for item in observations
        if item.event == "probe" and item.ssid
    )
    relationships = list(dict.fromkeys(relationships))
    return manifest, observations, relationships


def write_csv(path: Path, observations: list[Observation]) -> None:
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=Observation.__dataclass_fields__)
        writer.writeheader()
        writer.writerows(asdict(observation) for observation in observations)


def write_json(path: Path, manifest: dict, observations: list[Observation],
               relationships: list[Relationship]) -> None:
    payload = {
        "manifest": manifest,
        "observations": [asdict(item) for item in observations],
        "relationships": [asdict(item) for item in relationships],
    }
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def _route_points(observations: list[Observation]) -> str:
    points = [item for item in observations if item.latitude is not None]
    if not points:
        return '<div class="empty">No GPS fixes were recorded.</div>'
    latitudes = [item.latitude for item in points]
    longitudes = [item.longitude for item in points]
    lat_span = max(max(latitudes) - min(latitudes), 0.0001)
    lon_span = max(max(longitudes) - min(longitudes), 0.0001)
    coordinates = []
    for item in points:
        x = 20 + ((item.longitude - min(longitudes)) / lon_span) * 560
        y = 180 - ((item.latitude - min(latitudes)) / lat_span) * 160
        coordinates.append(f"{x:.1f},{y:.1f}")
    return (
        '<svg viewBox="0 0 600 200" role="img" aria-label="GPS sighting plot">'
        '<defs><filter id="glow"><feGaussianBlur stdDeviation="3" result="b"/>'
        '<feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>'
        '</filter></defs><polyline class="route" points="'
        + " ".join(coordinates)
        + '"/></svg>'
    )


def write_html(path: Path, manifest: dict, observations: list[Observation],
               relationships: list[Relationship]) -> None:
    counts = {name: 0 for name in TYPE_NAMES.values()}
    for item in observations:
        if item.event == "new":
            counts[item.type] += 1
    probe_count = sum(item.event == "probe" for item in observations)
    repeat_count = sum(item.event == "repeat" for item in observations)
    duration = int(manifest.get("duration_ms", 0)) // 1000
    rows = "".join(
        f'<tr data-time="{item.elapsed_ms}">'
        f"<td>{item.elapsed_ms / 1000:.1f}s</td><td>{html.escape(item.event)}</td><td>{html.escape(item.type)}</td>"
        f"<td>{item.mac}</td><td>{html.escape(item.ssid or '-')}</td><td>{item.rssi}</td><td>{item.channel or '-'}</td>"
        f"<td>{item.latitude:.6f}, {item.longitude:.6f}</td></tr>" if item.latitude is not None else
        f'<tr data-time="{item.elapsed_ms}">'
        f"<td>{item.elapsed_ms / 1000:.1f}s</td><td>{html.escape(item.event)}</td><td>{html.escape(item.type)}</td>"
        f"<td>{item.mac}</td><td>{html.escape(item.ssid or '-')}</td><td>{item.rssi}</td><td>{item.channel or '-'}</td><td>-</td></tr>"
        for item in observations
    )
    relationship_rows = "".join(
        "<tr>"
        f"<td>{html.escape(item.kind)}</td><td>{html.escape(item.source)}</td>"
        f"<td>{html.escape(item.target)}</td><td>{html.escape(item.confidence)}</td></tr>"
        for item in relationships
    ) or '<tr><td colspan="4" class="empty">No relationships were recorded.</td></tr>'
    replay_max = max((item.elapsed_ms for item in observations), default=0)
    document = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>Marauder Recon Report</title><style>
:root{{--bg:#05070b;--panel:#101520;--line:#273247;--text:#eaf6ff;--muted:#91a4b8;--cyan:#28e7ff;--mag:#ff3fca}}
*{{box-sizing:border-box}} body{{margin:0;background:radial-gradient(circle at top,#12213a,var(--bg) 45%);color:var(--text);font:14px system-ui,sans-serif}}
main{{max-width:1100px;margin:auto;padding:28px}} h1{{letter-spacing:.12em;margin:0;color:var(--cyan)}} .sub{{color:var(--muted);margin:6px 0 24px}}
.grid{{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px}} .card,.panel{{background:#101520dd;border:1px solid var(--line);border-radius:10px;box-shadow:0 0 20px #0008}}
.card{{padding:15px}} .value{{font-size:25px;color:var(--mag)}} .label{{color:var(--muted);text-transform:uppercase;font-size:11px}}
.panel{{margin-top:16px;padding:16px;overflow:auto}} h2{{font-size:14px;letter-spacing:.1em;color:var(--cyan)}} svg{{width:100%;height:200px;background:linear-gradient(#09111d 1px,transparent 1px),linear-gradient(90deg,#09111d 1px,transparent 1px);background-size:30px 30px}}
.route{{fill:none;stroke:var(--mag);stroke-width:3;filter:url(#glow)}} table{{width:100%;border-collapse:collapse;white-space:nowrap}} th,td{{padding:8px;border-bottom:1px solid var(--line);text-align:left}} th{{color:var(--cyan)}} .empty{{color:var(--muted);padding:28px;text-align:center}}
.replay{{display:flex;gap:14px;align-items:center}} .replay input{{width:100%;accent-color:var(--mag)}} .hidden{{display:none}}
</style></head><body><main><h1>RECON MISSION</h1>
<div class="sub">{html.escape(path.parent.parent.name)} · {html.escape(str(manifest.get('mode', 'unknown')).upper())} · {html.escape(str(manifest.get('state', 'unknown')).upper())}</div>
<section class="grid"><div class="card"><div class="value">{len(observations)}</div><div class="label">Sightings</div></div>
<div class="card"><div class="value">{counts['access-point']}</div><div class="label">Access points</div></div>
<div class="card"><div class="value">{counts['station']}</div><div class="label">Stations</div></div>
<div class="card"><div class="value">{counts['ble']}</div><div class="label">BLE devices</div></div>
<div class="card"><div class="value">{probe_count}</div><div class="label">Probe requests</div></div>
<div class="card"><div class="value">{repeat_count}</div><div class="label">Changed / returned</div></div>
<div class="card"><div class="value">{len(relationships)}</div><div class="label">Relationships</div></div>
<div class="card"><div class="value">{duration // 60}:{duration % 60:02d}</div><div class="label">Duration</div></div></section>
<section class="panel"><h2>GPS SIGHTING PLOT</h2>{_route_points(observations)}</section>
<section class="panel"><h2>MISSION REPLAY</h2><div class="replay"><button id="play">PLAY</button><input id="scrub" type="range" min="0" max="{replay_max}" value="{replay_max}" step="100"><output id="clock"></output></div></section>
<section class="panel"><h2>OBSERVED RELATIONSHIPS</h2><table><thead><tr><th>Kind</th><th>Source</th><th>Target</th><th>Confidence</th></tr></thead><tbody>{relationship_rows}</tbody></table></section>
<section class="panel"><h2>OBSERVATION TIMELINE</h2><table><thead><tr><th>Elapsed</th><th>Event</th><th>Type</th><th>MAC</th><th>SSID</th><th>RSSI</th><th>Channel</th><th>Position</th></tr></thead><tbody>{rows}</tbody></table></section>
</main><script>
const scrub=document.getElementById('scrub'),clock=document.getElementById('clock'),play=document.getElementById('play');let timer;
function render(){{const now=Number(scrub.value);clock.value=(now/1000).toFixed(1)+'s';document.querySelectorAll('tr[data-time]').forEach(row=>row.classList.toggle('hidden',Number(row.dataset.time)>now));}}
scrub.addEventListener('input',render);play.addEventListener('click',()=>{{if(timer){{clearInterval(timer);timer=null;play.textContent='PLAY';return;}}scrub.value=0;play.textContent='PAUSE';timer=setInterval(()=>{{scrub.value=Math.min(Number(scrub.max),Number(scrub.value)+250);render();if(scrub.value==scrub.max){{clearInterval(timer);timer=null;play.textContent='PLAY';}}}},50);}});render();
</script></body></html>"""
    path.write_text(document, encoding="utf-8")


def convert(directory: Path, output: Path | None = None, make_zip: bool = False) -> Path:
    directory = directory.resolve()
    manifest, observations, relationships = load_mission(directory)
    output = (output or directory / "report").resolve()
    output.mkdir(parents=True, exist_ok=True)
    write_csv(output / "observations.csv", observations)
    write_json(output / "mission.json", manifest, observations, relationships)
    write_html(output / "index.html", manifest, observations, relationships)
    if make_zip:
        archive = output.parent / f"{directory.name}-report.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
            for item in sorted(output.iterdir()):
                bundle.write(item, item.name)
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mission", type=Path, help="Mission directory containing session.json")
    parser.add_argument("--output", type=Path, help="Report output directory")
    parser.add_argument("--zip", action="store_true", help="Also create a portable ZIP")
    arguments = parser.parse_args()
    try:
        output = convert(arguments.mission, arguments.output, arguments.zip)
    except (OSError, ReconReportError) as error:
        parser.error(str(error))
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
