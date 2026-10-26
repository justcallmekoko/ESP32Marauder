#pragma once

#ifndef GpsLc76gI2c_h
#define GpsLc76gI2c_h

#include "configs.h"

#ifdef HAS_GPS_LC76G_I2C

#include <stdint.h>

// Quectel LC76G GNSS over I2C (Waveshare's LC76G_RPI_I2C protocol), for boards
// whose GPS can only be reached on the shared I2C bus. Ported from the
// ESP32-C5-BSP driver that was verified on the Waveshare ESP32-C5 3.5".
namespace marauder {
namespace lc76g {

// True if the module answers on the bus. Call once from GpsInterface::begin().
bool begin();

// One read cycle (self-throttled to ~2 Hz): pulls pending NMEA text from the
// module and hands each byte to `sink`. Call from the main loop only; the cycle
// is a multi-frame transaction that must not interleave with other I2C traffic.
void service(void (*sink)(char c));

uint32_t bytesRead();
uint32_t failedCycles();

}  // namespace lc76g
}  // namespace marauder

#endif  // HAS_GPS_LC76G_I2C
#endif  // GpsLc76gI2c_h
