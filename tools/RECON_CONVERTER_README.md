# Marauder Recon Converter

The converter turns one Marauder `/recon/m####/` mission directory into a portable report. It runs on your computer; it is not installed on the Marauder or copied to its SD card.

## Desktop app

1. Copy the complete `m####` directory from the Marauder SD card to your computer.
2. Open `Marauder-Recon-Converter` for your operating system.
3. Select the copied mission directory and choose **Build Report**.
4. The converter opens `report/index.html` and creates `m####-report.zip` beside it.

You can also drag a mission directory onto the executable on operating systems that pass dropped folders as command-line arguments.

## Python fallback

Python 3.10 or newer is sufficient; there are no third-party runtime dependencies.

```text
python3 tools/recon_report.py /path/to/m0042 --zip
```

## Inputs

- `session.json`: mission manifest and completion state.
- `obs.rlog`: AP, station, BLE, repeat, and change observations.
- `probes.rlog`: Wi-Fi probe-request observations, when present.
- `relations.rlog`: observed station-to-AP relationships, when present.
- `.pcap`: packet capture for separate inspection in Wireshark.

Keep the input files together and retain them as the authoritative raw mission data.

## Outputs

- `report/index.html`: self-contained interactive report with timeline scrubbing.
- `report/observations.csv`: spreadsheet-friendly observations.
- `report/mission.json`: expanded observations and relationships.
- `m####-report.zip`: portable copy of the generated report.

Relationship entries are observations, not identity claims. A client probing for an SSID does not prove ownership or a current association. Interrupted missions marked `active` can be converted if their complete records are intact; final counters and duration may be incomplete.
