# Seeed reTerminal Sticky

Development hardware port based on upstream commit
`8244eab69b0ddc316ccaf6ded470d9a999cf5b5a` (Marauder v1.19.2).
Target flag: `MARAUDER_RETERMINAL_STICKY`.

## Hardware and build

ESP32-S3R8, 32 MB quad flash, 8 MB octal PSRAM, SSD1677 800x480
monochrome e-paper, GT911 capacitive touch, three buttons, shared-SPI microSD.
The menu is portrait, 480x800. The CPU runs at its specified maximum
240 MHz; display SPI writes use 20 MHz, within SSD1677 table 12-1. No custom waveform,
forced temperature, elevated voltage, or out-of-spec overclock is used.

Install Python 3.12 and `pioarduino==6.2.0`, then:

```sh
pio run -e reterminal_sticky
pio test -e sticky_native
pio run -e reterminal_sticky -t upload --upload-port COM61
pio device monitor --port COM61 --baud 115200
```

Replace COM61 with the verified Sticky port. This board uses a CH343 USB
UART bridge. UART0 console is enabled; USB CDC on boot is disabled.
Native tests require a host C++ compiler. Run builds and tests sequentially
so PlatformIO's build-directory cleanup cannot collide with another build.

The environment pins Arduino-ESP32 3.3.12 and its libraries. Two small
upstream missing-return fixes allow the compiler's return-type checks to
remain enabled. Other board profiles retain their existing display paths.

The included 32 MB partition table allocates two 6 MB OTA applications and
the remaining space to SPIFFS, with conventional NVS/OTA data offsets.
Back up the entire existing flash before the first installation: installing
a different partition layout does not migrate the old filesystem.

## Pin map

| Function | GPIO |
| --- | --- |
| E-paper SCK / MOSI / MISO | 13 / 14 / 12 |
| E-paper CS / DC / RESET / BUSY / enable | 15 / 16 / 17 / 18 / 47 |
| Touch SDA / SCL / interrupt / reset / enable | 3 / 2 / 21 / 41 / 42 |
| SD CS / enable | 8 / 10 |
| Up / center / down buttons | 5 / 4 / 6 |
| Power hold / lock | 45 / 46 |
| Charger enable, active low | 39 |

## Display behavior

- Three 48,000-byte monochrome buffers live in PSRAM: drawing, in-flight,
  and last successfully displayed frames. The display worker never reads
  a framebuffer being modified by the UI.
- An unchanged framebuffer generates no panel transaction. The driver tracks
  a byte-aligned changed rectangle, but supplies complete old/new comparison
  planes because SSD1677 partial refresh drives the whole panel. Sending
  only the changed window left stale dark selection rows on this device.
- Drawing bursts settle for 35 ms, with a 400 ms bound for continuous
  drawing. Submissions are separated by at least 150 ms. These are
  scheduling limits, not claims about physical panel frame rate.
- The worker waits for BUSY with a timeout, releasing the SPI bus and
  yielding the CPU. The main loop continues processing input. Display and
  SD use the same SPI object and transaction lock.
- The first frame and every 24 partial refreshes use a full cleaning
  refresh. A failed refresh invalidates the previous image and forces a
  full retry with backoff.
- Black backgrounds become white paper. Colored foregrounds become black
  ink; text pairs retain contrast. A small triangular cursor marks selection;
  menu text remains stable when scrolling. Boolean settings
  have a checkbox so their state does not depend on red/green color.
- Continuous menu marquees and backlight gestures are disabled. E-paper
  cannot reproduce LCD animations or distinguish colored graph traces;
  those views need hands-on review before release.
- Activity headings use 18-point bold type and wrap long titles. Portrait
  scan controls have 100x56 touch targets, with log output beginning at y=208.
  Recon uses a dedicated portrait dashboard with readable counters and
  separate incoming/outgoing churn traces for the monochrome display.

## Input

GT911 probing checks both supported addresses and validates the product ID.
The interrupt pin selects the address during reset. After reset rises, the
driver waits 6 ms, drives INT low for the Goodix 50 ms synchronization phase,
then releases it to input mode. The touch power rail is settled for 250 ms.
Omitting synchronization produced a valid ID with zero configuration and
resolution registers on the tested board; the corrected sequence restored
480x800 resolution and actual touch events. Runtime polling does not depend
on an assumed interrupt polarity. Power-button boot/hold shortcuts that
silently disable touch are excluded for Sticky.

The default transform follows Seeed's portrait example: scale raw X to
0..479, reverse and scale raw Y to 0..799, then apply the UI rotation.
The display mapping composes the portrait transform from `demo_touch.cpp`
with `panel_cfg.mirror_x = true` from `demo_epaper.cpp`. Both pixels and
dirty windows use these final controller coordinates. Omitting the panel
mirror or replacing it with a 180-degree rotation reverses the text.
A ready zero-contact report releases a touch;
a not-ready report preserves the current state. All ready reports are
acknowledged, including empty or malformed reports. I2C failures have a
bounded hold grace period.

Menu controls follow Marauder's CYD behavior across the full width: top
third moves up, middle third selects on release, bottom third moves down.
There is no side strip or direct row activation. Held up/down touches use
upstream repeat behavior; keyboards still use their individual key targets.
Physical buttons remain available when the touch controller is absent.

Side buttons provide optional navigation: GPIO5 moves up and GPIO6 moves
down, including held-repeat in menus. Both inputs use 25 ms debounce. A
short GPIO4 power-button press selects on release (or exits an active view);
holding it for three seconds shuts down without selecting. The boot/wake
press is ignored until released.

Shutdown stops active work, flushes the capture buffer, displays a final
power-off page, sleeps the panel, unmounts storage, and disables peripheral
rails. It clears the hardware power latch by clocking GPIO46 with GPIO45
low. On battery this removes the main supply; USB bypasses that switch, so
the processor also enters deep sleep with GPIO4 as its wake source. Startup
releases held GPIOs and clocks the latch back on. The last e-paper image
remains visible while powered off.

## Local diagnostics

The following serial commands do not initiate wireless actions:

| Command | Result |
| --- | --- |
| `sticky status` | Refresh counts, failures, last duration, touch address, free heap |
| `sticky touchreset` | Power-cycle and reinitialize the GT911 |
| `sticky clean` | Request a full cleaning refresh |
| `sticky dump` | Export the native monochrome framebuffer as 48,000 raw bytes |
| `sticky test` | Five-target touch test, only while scans are stopped |
| `sticky preview deauth` | Detector layout preview with no scan started |
| `sticky preview recon` | Recon layout preview with no scan started |
| `sticky tick` | Toggle an 80x40 test rectangle to measure a small partial update |
| `sticky menu` | Return from the test page to the current menu |
| `sticky poweroff` | Run the same shutdown sequence as a three-second power-button hold |
| `sticky powertest` | While stopped and USB-powered, shut down and wake after five seconds to verify reinitialization |

Status also includes raw touch polling counters, configuration bytes, and
UI enable flags. The center button exits the test page. Touch down/up coordinates and
refresh timing are reported over serial. The framebuffer export describes
rendered pixels; it cannot prove physical orientation, contrast, or ghosting.

## Required release validation

See `reterminal-sticky-validation.md` for the current measured results and
the owner's physical orientation/touch observations.

Automated tests cover reference transform endpoints, all four rotations,
all 384,000 pixels per rotation, dirty-window bounds, touch state, invalid
reports, I2C failure handling, and timer rollover.

Before requesting upstream acceptance, record hands-on results for:

1. The five touch targets, all three navigation zones, edge taps, held contacts, and
   on-screen keyboard in portrait and landscape views.
2. Navigation through every menu, physical buttons, selection and settings
   indicators, long labels, and exits from blocking views.
3. Readability and ghosting after repeated partial updates, cold and warm
   operation, and a long idle/active soak. Set the cleaning interval from
   measured panel behavior if 24 partials is unsuitable.
4. SD operations sharing the display bus, absent-card behavior, reboot,
   battery-only operation, charging, and recovery from the backup.

Battery telemetry, RTC, IMU, microphone, and automatic idle sleep are outside
this initial display/input port. Do not describe this build as fully
hardware-qualified until the remaining checks have been performed.

## References

- [Seeed hardware overview](https://www.seeedstudio.com/sticky/docs/en/device-guide/hardware-overview/)
- [Seeed schematic, page 6](https://files.seeedstudio.com/wiki/reterminal_sticky/res/reTerminal_Sticky_Schematic_diagram_260609.pdf):
  U3 power latch and the USB bypass path.
- [Seeed power sequencing reference](https://www.seeedstudio.com/sticky/docs/en/device-guide/esp-refresh/):
  wait for button release, disable/hold peripheral rails, and use GPIO4 wake.
- [Seeed peripheral demo](https://files.seeedstudio.com/wiki/reterminal_sticky/res/Sticky_peripheral_demo.zip):
  `main/demo_touch.cpp`, `main/demo_epaper.cpp`, `main/pin_config.h`,
  `components/seeed_epaper/epaper_panel.c`, and
  `components/seeed_epaper/driver/ssd1677.c` define the touch mapping,
  panel mirror, pins, and controller commands used here.
- [ESP32-S3 datasheet](https://documentation.espressif.com/esp32_s3_datasheet_en.pdf)
- [SSD1677 datasheet, table 12-1](https://files.waveshare.com/upload/2/2a/SSD1677_1.0.pdf)
- [Goodix reset timing reference](https://github.com/torvalds/linux/blob/master/drivers/input/touchscreen/goodix.c)
- [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)
