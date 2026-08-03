# Firmware unit tests

The unit-test environment uses PlatformIO's native platform and Unity test
framework. It compiles selected production sources with the host GCC toolchain,
so tests do not require an ESP32, radios, credentials, or network access at run
time.

Run the suite from the repository root:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install platformio==6.1.19
.venv/bin/platformio test -e native
```

Keep host-testable firmware modules free of Arduino/hardware dependencies,
include each production source in the native environment's `build_src_filter`,
and add a focused test folder under `test`.

This suite complements the existing Arduino matrix compile. Hardware behavior,
radio operations, timing, peripherals, and full-device integration still need
target or hardware-in-loop testing.
