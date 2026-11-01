# reTerminal Sticky validation record

## Current software review — 2026-10-31

The contribution is rebased onto Marauder v1.20.0, upstream commit
`511bb3694d0c4dc064888e762bde44fcc9b7f48a`. The current software checks pass
**188 native test cases across 32 suites** and **103 Python checks** using GCC
16.2.0 / C++17 and Python 3.12 on Windows.

Automatic installer CI exposed a legacy C++11 compilation issue in the new
model sources, which Arduino compiles even on other board profiles. `Area`
now has an equivalent C++11-compatible constructor. A strict C++11 syntax
check compiles both model implementation files without e-paper board flags.
Manual board selection also reaches the physical-switch implementation;
preprocessing checks cover both profiles with Arduino's normal build macro.
The artifact record below belongs to the stated earlier revision; target
images are being rebuilt after this compatibility correction.

The validated source/tooling revision is `eabe3308676d7262ed614d6c1dc3e5e22be2f1d1`.
`pio run -e reterminal_sticky` passes with Arduino 3.3.12 / ESP-IDF 5.5.5.
The compiler reports 84,260 bytes of static RAM and 1,814,947 bytes against
the 6,291,456-byte app slot. These are link-time figures, not runtime heap
measurements. Esptool validates the ESP32-S3 / 32 MB image checksum and hash;
the merged bootloader, partition table, and app regions match their components.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `firmware.bin` | 1,867,024 | `e0f4f53cfba1ebc56d548a4f4efe721ce3d1cf60beb2f6c9e7c3416fdc6a4133` |
| `firmware.factory.bin` | 1,932,560 | `16911513ecabb1a4dba92f06ad23a4dcab755c06fb31ca5bdaf03389211564de` |

Startup retains upstream's removal of the serial-host wait. Board power is
latched as the first `setup()` action, before serial and peripheral setup.
The pinned Sticky build uses its CH343 UART bridge with USB CDC disabled.
Regression checks cover startup order, absence of serial input waits, both
supported board-selection methods, and the physical packet-monitor exit.
Keyboard regression cases cover the initial panel refresh, release of the
opening touch, layout changes, and text-only redraws. Short center release
now cancels the keyboard even if touch or the display fails.

The previous 20 MiB SPIFFS partition exceeded the pinned SDK's 16-bit page
index limit, explaining the startup settings-format failure in the historical
logs. The revised table keeps both app slots and the filesystem start, caps
SPIFFS at `0xff0000` bytes, and reserves the upper 4 MiB. Boundary checks cover
both e-paper layouts. Install the corrected partition table with the app;
current-device formatting and settings persistence still require physical
verification. First formatting can take time without a serial host.

No Sticky was connected during this review, and this revision has not been
flashed or tested on battery. All measurements and owner confirmations below
belong to the earlier firmware. For the current cold-boot check, disconnect
USB completely, power on from a charged battery, then move the selection and
open a menu. A retained e-paper menu alone does not prove successful boot.
Repeat with USB power and all serial monitors closed, recording the firmware
commit and both results.

## Historical hardware record

Development port against Marauder commit
8244eab69b0ddc316ccaf6ded470d9a999cf5b5a. This supersedes the earlier
corner-test report, which did not establish working menu navigation.

PR preparation on 2026-10-27 passed all 174 native cases using GCC 16.2.0 in
C++17 mode and all 81 Python tooling/regression checks on Windows. Both e-paper
models participate in the existing native test target. Hardware workflows are
manual, following the repository's development workflow.

## Touch and navigation

The initial driver could read product ID 911, but the controller returned
zero configuration/resolution registers and no ready touch reports. Settling
the power rail and adding the Goodix 50 ms INT-low synchronization phase
restored configuration version 0x41, 480x800 resolution, and touch events.
No controller configuration blob or EEPROM rewrite was needed.

Controls now follow CYD across the full width: top third Up, middle third
Select on release, bottom third Down. The side strip and direct row actions
are removed, as are hidden power-button shortcuts that disabled touch.

Live serial evidence includes Down actions at several positions including
an edge tap at (4,555), and a user-identified top-left tap at (9,127) that
produced Up. Moving Up from the first item wraps to the last. The owner
reports that middle selection opens Device; that particular transition was
not independently captured. Recorded status samples report zero I2C errors
and rejected coordinates.

## Physical buttons and shutdown

Sixteen host model tests pass, including contact bounce, selection only on
release, one shutdown event per hold with no selection afterward, ignoring
the boot press, and timing rollover/delayed polling. GPIO5/6 feed the existing
Marauder menu repeat logic. GPIO4 short release selects/exits; holding for
three seconds initiates shutdown.

The owner subsequently confirmed the full physical sequence: Up, Down,
short power press to select, holding power for three seconds and releasing
to shut down, then pressing power to restart. All buttons and power-off/on
worked. The power source was not specified in that confirmation, so it does
not independently establish battery-only power removal or standby current.

The latch sequence follows Seeed's schematic, page 6: GPIO45 drives U3 D and
GPIO46 drives its positive-edge clock. USB bypasses the latch-controlled
power path, so shutdown also disables peripheral rails and enters deep
sleep. Battery-only power removal and standby current still require
verification with the power source explicitly recorded.

The USB diagnostic ran the production shutdown path with a five-second
timer wake: the final full refresh completed in 1,719 ms, serial recorded
entry into deep sleep, and boot reported `rst:0x5 (DSLEEP)` and wake cause 4
(timer). After wake, GT911 reported 480x800 at 0x14, the display refreshed
successfully, and diagnostic counters reported zero display/I2C errors.
This verifies USB sleep and reinitialization, not physical button wake or
battery current. The startup settings-format warning in this log and the
earlier `final-boot.log` used the invalid 20 MiB SPIFFS layout. The current
review corrects that layout as described above; settings persistence on the
revised firmware remains a physical qualification item.

## Display and performance

Menus use proportional fonts, aligned margins, eight rows per page, stable
labels, and a small triangular cursor. The page counter stays unchanged
while moving within a page. Framebuffer exports were inspected for layout;
these are software previews, not camera photographs.

The owner confirmed that only one arrow remains and the old black selection
rows are gone during the latest navigation check. Extended ghosting checks
across temperature and longer sessions remain outstanding.

Activity screens now share an 18-point bold heading with wrapping, a separate
control row, and scrolling output starting below the controls. Recon has a
portrait dashboard with 16-pixel labels, 24-pixel counts, separate positive
and negative churn traces, associations, events, and channel activity.
Both detector and Recon previews were exported from the flashed device and
visually inspected without starting a wireless scan. Headings, counters,
controls, and footer fit without overlap in these empty-state previews.
The device was returned to its menu afterward. Populated/live layouts and
the owner's physical readability check remain outstanding.

The previous full-row inversion left visible stale dark rows although the
framebuffer contained only one selection. The current driver supplies full
old/new comparison planes on every partial activation, following Seeed's
guidance. It uses 20 MHz writes within SSD1677 table 12-1 and removes the
redundant post-refresh plane transfer. Waveforms and voltages are unchanged.

| Check | Current result |
| --- | --- |
| Build and flash | Successful; esptool verifies the image |
| Application file | 1,865,520 bytes |
| Host model tests | Sixteen pass with warnings treated as errors |
| Partial refresh | 664-666 ms including complete comparison transfers |
| Full cleaning | 1,719-1,721 ms; cold refresh 1,765 ms |
| Cleaning threshold | Observed after 24 consecutive partials |
| Display failures | Zero through the captured 30-refresh sample |
| Free heap | 229,340 bytes at start and end of that sample |

The attempted idle assertion was interrupted by a real touch. Its extra
refresh was expected, so this run is not a passed idle test. The prior build
passed an unchanged-frame check. The stock waveform still takes roughly
two-thirds of a second; do not claim LCD-like scrolling. The owner's bounded
navigation check passed; this is not a long-duration panel qualification.

## Remaining release checks

- Verify the larger activity layouts on the physical display, including
  live data, long titles, exits, and legacy graph views.
- Test middle selection/release, held navigation, keyboard entry, landscape
  paths, physical buttons, and all menu exits.
- Check shared-bus SD operations, battery-only use, charging, recovery, and
  long-duration ghosting across operating temperatures.
- Run the supplied GitHub CI and upstream-required regression jobs.

Battery telemetry, RTC, IMU, microphone, and automatic idle sleep remain outside this
initial display/input port. CPU is 240 MHz. Keep this as a review build
until the remaining hardware checks pass.
