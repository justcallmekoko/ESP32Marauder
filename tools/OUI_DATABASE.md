# Offline OUI database builder

`build_oui_database.py` converts the four IEEE Registration Authority CSV
registries into a deterministic binary database. Builds with `HAS_OUI_LABELS`
read that database from the SD card only while rendering AP or station details;
the scan, capture and saved-log formats remain unchanged.

The generated database is named `marauder_oui.bin` by default and is ignored by
Git. Do not commit downloaded IEEE CSV files or generated databases to this
repository.

## Data provenance and licensing

IEEE Registration Authority registry data is maintained and published by IEEE.
It is not covered or relicensed by ESP32 Marauder's MIT license. Before
downloading, redistributing, publishing, or bundling IEEE data or a derived
database, review and comply with the current IEEE terms, copyright notices, and
attribution requirements.

The repository does not vendor an IEEE snapshot. A reproducible release process
should retain, outside this repository:

- the four source URLs or local source filenames;
- the retrieval date;
- the original CSV SHA-256 hashes;
- the generated database SHA-256 printed by the converter; and
- the IEEE terms and notices that applied to that snapshot.

The converter's `--download` mode uses these official HTTPS endpoints:

| Registry | Prefix | Endpoint |
| --- | ---: | --- |
| MA-L | /24 | `https://standards-oui.ieee.org/oui/oui.csv` |
| MA-M | /28 | `https://standards-oui.ieee.org/oui28/mam.csv` |
| MA-S | /36 | `https://standards-oui.ieee.org/oui36/oui36.csv` |
| IAB | /36 | `https://standards-oui.ieee.org/iab/iab.csv` |

Network access never occurs implicitly. It requires the explicit `--download`
option.

## Usage

Build from four already downloaded local CSV files:

```sh
python tools/build_oui_database.py \
  --ma-l path/to/oui.csv \
  --ma-m path/to/mam.csv \
  --ma-s path/to/oui36.csv \
  --iab path/to/iab.csv
```

Explicitly download all four current CSV files and convert them in memory:

```sh
python tools/build_oui_database.py --download
```

Select another output path with `--output`:

```sh
python tools/build_oui_database.py --download --output build/marauder_oui.bin
```

Copy or upload the generated file to the SD-card root with this exact path:

```text
/marauder_oui.bin
```

A reboot is not required. The next AP-detail or station menu render opens the
new file. When the file is absent, station menus keep showing the original MAC
address and AP details report `OUI: no database`. A malformed or incompatible
file is rejected and reported as `OUI: database error`.

Supplying only some local registries, mixing local paths with `--download`, or
an invalid assignment is a hard error. Identical duplicate assignments are
emitted once.

The public registries can contain the same prefix with different organization
names. The default behavior is conservative: every such prefix is omitted
completely instead of choosing one name arbitrarily. The converter prints a
bounded list of warnings, the total conflict count, and includes
`conflicts=<count>` in its summary. Use `--strict-conflicts` when a pipeline
should abort if even one conflict exists:

```sh
python tools/build_oui_database.py --download --strict-conflicts
```

Run the synthetic, network-free tests with:

```sh
python -m unittest tools.test_build_oui_database
```

## Binary format, version 1

All integer counts in the header are little-endian. Prefix bytes in records are
big-endian. The file contains one 24-byte header followed by fixed 29-byte
records.

### Header

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 8 | Magic ASCII `MROUI001` |
| 8 | 1 | Format version, currently `1` |
| 9 | 1 | Record size, currently `29` |
| 10 | 1 | Name field size, currently `24` |
| 11 | 1 | Reserved, must be zero |
| 12 | 4 | Number of /24 records, little-endian |
| 16 | 4 | Number of /28 records, little-endian |
| 20 | 4 | Number of /36 records, little-endian |

Records are grouped in that same order: all /24 records, then /28 records, then
/36 records. Each group is sorted numerically by prefix. MA-S and IAB share the
/36 group and are deduplicated together. Conflicting prefixes omitted by the
converter are not included in these counts. A consumer performing
longest-prefix matching must check /36 before /28 before /24.

### Record

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 5 | Prefix, big-endian and left-aligned in 40 bits |
| 5 | 24 | Printable ASCII organization name, NUL-terminated and zero-padded |

Examples of prefix encoding:

| Assignment | Width | Stored bytes |
| --- | ---: | --- |
| `AABBCC` | /24 | `AA BB CC 00 00` |
| `AABBCCD` | /28 | `AA BB CC D0 00` |
| `AABBCCDDE` | /36 | `AA BB CC DD E0` |

Organization names are normalized deterministically with Unicode NFKD.
Combining marks are removed, whitespace is collapsed, remaining non-ASCII
characters become `?`, and the printable ASCII result is truncated to 23 bytes.
At least one NUL byte is therefore always present in the 24-byte name field.
