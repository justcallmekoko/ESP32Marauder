# Enclosure STL set

This directory contains the slicer-ready exports for the project-specific
Waveshare 3.5-inch / ESP32-C5 enclosure. It intentionally contains no editable
FreeCAD or STEP source, manufacturer CAD, combined reference assembly, or old
V1 design-history parts.

## Required printed parts

| File | Purpose | Suggested print orientation |
| --- | --- | --- |
| `waveshare-35-front-housing-v2.stl` | Display front housing | Visible front face on the bed, rear cavity upward |
| `waveshare-35-display-retainer-v2.stl` | Rear display retainer | Exported flat orientation, contact pads upward |
| `waveshare-35-rear-adapter-v2.stl` | Interface between front and rear body | Main 4 mm frame on the bed, side lugs upward |
| `waveshare-35-rear-shell-v2.stl` | Rear shell with two USB-C openings and SMA D-hole | Outside rear wall on the bed, open front upward |
| `esp32-c5-carrier-v2.stl` | ESP32-C5 carrier | Rear face on the bed, PCB locators upward |
| `esp32-c5-clamp-v2.stl` | ESP32-C5 clamp | Exported flat orientation |
| `waveshare-35-stylus-holder-v1.stl` | Removable holder for the recorded 9 mm stylus | Use the exported flat print orientation |

## Optional fit coupons

- `esp32-c5-port-fit-test-v2.stl` checks both USB openings and the SMA D-hole
  before printing the full rear shell.
- `waveshare-35-stylus-clip-fit-test-v1.stl` checks the 9.3 mm clip fit in the
  intended filament before printing the complete holder.

## Initial print settings

- 0.20 mm layers, 0.4 mm nozzle, four walls
- PETG or ABS/ASA around heat-set inserts
- verify the real insert diameter and length before heat-setting
- use soft pads at the display standoffs and PCB clamp; never hard-clamp the
  cover glass or PCB

The models passed the original STEP/STL solid and collision checks. Printer,
material, cable-overmould, antenna-bulkhead, and fastener tolerances still need
to be checked on the actual parts. See `PARTS.md` for the recorded hardware and
`SHA256SUMS.txt` for exact file identities.
