# M5Paper v1.1 development validation

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
The artifact record below is retained for its stated earlier revision.
Current source validation and rebuilt-image evidence are recorded in
[the reviewed contribution, PR #1685](https://github.com/justcallmekoko/ESP32Marauder/pull/1685).

The validated source/tooling revision is `eabe3308676d7262ed614d6c1dc3e5e22be2f1d1`.
`pio run -e m5paper_v11 -t factory` passes with Arduino 3.3.4 / ESP-IDF
5.5.1-710-g8410210c9a. The compiler reports 88,196 bytes of static RAM and
2,053,595 bytes against the 4,194,304-byte app slot. IRAM uses 129,555 of
131,072 bytes, leaving 1,517 bytes; retain the pinned toolchain and verify
that margin after changes. These are link-time figures, not runtime heap
measurements. Esptool validates the ESP32 / 16 MB image checksum and hash;
the merged bootloader, partition table, and app regions match their components.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `firmware.bin` | 2,054,096 | `b90555e4e14feb2f205094d2346ddf543ef677e00eb4daea9205c51f63a274d2` |
| `firmware.factory.bin` | 2,119,632 | `cba849fc80673b17f1014a8720d1ae8801e19ba0b9fa85ad506096917a217c2d` |

Startup retains upstream's removal of the serial-host wait. The main power
hold is restored as the first `setup()` action, before serial and peripheral
setup. The original ESP32 M5Paper uses its external USB/UART bridge.
Regression checks cover startup order, absence of serial input waits, both
supported board-selection methods, and the physical packet-monitor exit.
Keyboard tests cover visible layout transitions and touch release; short
center release now cancels the editor even when touch or the panel fails.

The clean build applies a source-hash-checked patch to the pinned M5GFX
IT8951 driver. HRDY failures latch an error, terminate image uploads, and
prevent a failed frame from becoming the previous successful framebuffer.
Regression checks cover unknown input rejection, exact patch contexts,
idempotence, and validation of both files before writing. The eight-second
job budget is checked between calls; individual HRDY waits are bounded at
about 4.1 seconds, while controller polling and SPI transfers also occur
inside the calls. The total can exceed the budget; it is not a hard
end-to-end deadline.

The factory-image helper uses the platform's configured esptool launcher,
including quoted paths with spaces, instead of assuming a package-directory
lookup is available. Tests reject a missing launcher, unexpected image layout,
and a merged image whose application bytes differ from the compiled app.

No M5Paper was connected during this review, and this revision has not been
flashed or tested on battery. The measurements below are historical evidence
from the earlier firmware. To qualify the current revision, disconnect USB,
shut down and power on from a charged battery, then move the selection and
open a menu. E-paper retains its image without power, so a visible menu alone
does not prove a successful boot. Repeat on USB power with all serial monitors
closed and record the exact firmware commit and both results.

## Historical hardware record — 2026-10-27

Recorded 2026-10-27. These results concern the connected M5Paper v1.1. The owner
confirmed readable orientation, aligned touches on both corner targets, normal
touch menu navigation, and jog up/down/short-center selection. Battery shutdown
and restart remain unconfirmed. This is a development port, not a completed
upstream qualification.

## Firmware identity

| Item | Value |
| --- | --- |
| Marauder base | v1.19.2 / `8244eab69b0ddc316ccaf6ded470d9a999cf5b5a` |
| Build environment | `m5paper_v11` |
| Arduino / IDF | 3.3.4 / v5.5.1-710-g8410210c9a |
| Application bytes | 2,052,944 |
| Application SHA-256 | `9313666a4293158746ef9c269cc0711b4c686ce1617d2905e9aa57e23a14ff3f` |
| Factory image bytes | 2,118,480 |
| Factory SHA-256 | `48f2106efa64dabe81d6a6ce2c44275511c5519f50b334e90833910beb6bd6c4` |
| Upload | Full initial install; final application write verified by esptool |

The full 16 MB previous firmware was saved before the first write. It is kept
separately and must not be included in a public review package.

## Completed checks

- PR preparation: the full native suite passed all 174 cases with GCC 16.2.0
  in C++17 mode, and all 81 Python tooling/regression checks passed on Windows.
  This includes both e-paper models; GitHub execution is tracked separately.
- Seven M5Paper host-model tests passed, including exhaustive display/touch
  coordinate consistency for all 2,073,600 pixel/rotation combinations.
- Sixteen existing Sticky input/model tests passed.
- The Sticky hardware firmware also builds successfully with the shared UI
  changes. The disconnected Sticky was not reflashed or physically retested.
- The dedicated hardware firmware builds and boots. The final linker map places
  calendar routines inside the flash instruction region and retains `memcpy`
  and `memset` in IRAM; no orphan-section warnings remain. IRAM ends at
  `0x4009fa14`, within the `0x400a0000` limit.
- The device reports a 540 × 960 UI, 240 MHz CPU, and 4,194,304 bytes of mapped
  PSRAM. The board has 8 MB physical PSRAM; the current classic-ESP32 mapping
  exposes 4 MB to the allocator.
- The GT911 responds at `0x5d` with a 540 × 960 coordinate range. Bus diagnostics
  reported zero I2C errors and zero rejected reports during the refresh test.
  The owner separately confirmed that text reads normally and dots appear under
  their finger at the upper-left and lower-right targets ("Text and both taps
  line up").
- The owner confirmed the main-menu touch navigation sequence and physical jog
  up/down and short center selection: "Touch navigation and jog controls all
  work". This does not establish long-center shutdown or activity-view exit.
- The SD card mounts, and the UI reports `SD OK`. Sustained simultaneous SD
  recording and display refresh have not been qualified.
- Thirty-five small partial updates completed in **140–145 ms**, mean **140.3 ms**,
  with zero reported display errors. The test rectangle's transfer window was
  88 × 48 pixels. The automatic clean occurred after 32 partial updates.
- Warm full cleaning updates took **777–788 ms**. Initial full-screen updates
  after reset/wake took **1,078–1,084 ms**. These are firmware-observed completion
  times, not measurements of finger-to-visible response or optical ghosting.
- USB shutdown displayed the off page, released the power controls, entered
  deep sleep, and woke by a diagnostic five-second timer (wake cause 4).
- The display's factory VCOM read was 2,800 mV after the final startup, timed
  wake, and three additional hardware resets. Read clock was reduced to 4 MHz
  after an intermittent zero read at 20 MHz. VCOM was never reprogrammed.
- The settings file loads on subsequent resets without the default-settings
  replacement message observed on the earlier Sticky build.
- Software framebuffer previews cover the main menu and large activity titles.
  They do not establish the appearance of the physical panel.

## Still required before calling the port fully validated

- Short center exit from individual activity views, long center shutdown, and
  battery-powered restart. The automated USB timer wake does not test these.
- Long scrolling sessions, ghosting, temperature changes, and battery current.
- Sustained SD recording, keyboard and landscape interactions, and every
  Marauder activity view. No radio feature acceptance tests were run here.
- Upstream maintainer review and CI; see the pull request for current results.

The owner completed the normal navigation and jog selection check after the
device was returned from the touch test to the Marauder menu.
