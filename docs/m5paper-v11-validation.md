# M5Paper v1.1 development validation

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
