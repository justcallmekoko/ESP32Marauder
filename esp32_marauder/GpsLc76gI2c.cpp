#include "GpsLc76gI2c.h"

#ifdef HAS_GPS_LC76G_I2C

#include <Arduino.h>
#include <Wire.h>

// Transaction (Waveshare LC76G_RPI_I2C):
//   0. 1-byte write to 0x50 (wake / presence)
//   1. write {reg 0xAA510008, len 4} (LE) to 0x50  -> "how much is buffered?"
//   2. read 4 bytes from 0x54                      -> length (LE)
//   3. write {reg 0xAA512000, len N} (LE) to 0x50  -> "send N"
//   4. read N bytes from 0x54                      -> NMEA text
// No other I2C device may be addressed mid-transaction (Quectel), which the
// single-threaded main loop guarantees. The module clock-stretches, so the
// cycle uses a long bus timeout and restores the previous one afterwards.

namespace marauder {
namespace lc76g {

namespace {

const uint8_t  CR_ADDR   = 0x50;  // command/register write
const uint8_t  RD_ADDR   = 0x54;  // data read
const uint8_t  WR_ADDR   = 0x58;  // data write (wake)
const uint32_t CHUNK     = 256;   // shorter atomic transaction for shared-bus touch latency
const uint32_t PERIOD_MS = 125;   // preserve 2048 B/s drain capacity (~8 Hz)
const uint32_t STEP_MS   = 3;     // inter-step settle
const uint16_t BUS_TIMEOUT_MS = 300;
const uint32_t EMPTY_WAKE_CYCLES = 4000 / PERIOD_MS;

uint32_t s_bytes = 0, s_fails = 0, s_last = 0, s_empty = 0;

// The C5 I2C controller needs the bus re-opened before a read sequence; do it
// once per cycle, never between the steps of a transaction.
void reopen() {
  Wire.end();
  Wire.begin(I2C_SDA, I2C_SCL, 100000U);
}

void put32le(uint8_t *b, uint32_t v) {
  b[0] = v; b[1] = v >> 8; b[2] = v >> 16; b[3] = v >> 24;
}

uint8_t writeCr(uint32_t reg, uint32_t len) {
  uint8_t b[8];
  put32le(b, reg);
  put32le(b + 4, len);
  Wire.beginTransmission(CR_ADDR);
  Wire.write(b, 8);
  return Wire.endTransmission();
}

size_t readBytes(uint8_t *buf, size_t n) {
  size_t got = Wire.requestFrom(RD_ADDR, n);
  for (size_t i = 0; i < got; i++) buf[i] = Wire.read();
  return got;
}

uint8_t touchAddr(uint8_t addr) {  // 1-byte write; 0 = ACKed
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)0);
  return Wire.endTransmission();
}

uint32_t read32(uint32_t reg) {  // 0xFFFFFFFF on failure
  if (writeCr(reg, 4) != 0) return 0xFFFFFFFFUL;
  delay(STEP_MS);
  uint8_t b[4];
  if (readBytes(b, 4) != 4) return 0xFFFFFFFFUL;
  return b[0] | (b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

class TimeoutGuard {
 public:
  TimeoutGuard() : prev_(Wire.getTimeOut()) { Wire.setTimeOut(BUS_TIMEOUT_MS); }
  ~TimeoutGuard() { Wire.setTimeOut(prev_); }
 private:
  uint16_t prev_;
};

// One cycle: bytes delivered, 0 if nothing buffered, -1 on a bus failure.
int poll(void (*sink)(char)) {
  static uint8_t buf[CHUNK];
  TimeoutGuard to;
  reopen();
  delay(STEP_MS);
  if (touchAddr(CR_ADDR) != 0) {
    touchAddr(RD_ADDR);  // recovery nudge, as in the vendor demo
    touchAddr(WR_ADDR);
    return -1;
  }
  delay(STEP_MS);
  uint32_t len = read32(0xAA510008UL);
  if (len == 0xFFFFFFFFUL || len >= 35 * 1024) return -1;
  if (len == 0) return 0;
  if (len > CHUNK) len = CHUNK;  // the rest drains on later cycles
  delay(STEP_MS);
  if (writeCr(0xAA512000UL, len) != 0) return -1;
  delay(STEP_MS);
  size_t got = readBytes(buf, len);
  if (got != len) return -1;
  for (size_t i = 0; i < got; i++) sink((char)buf[i]);
  return (int)got;
}

// Vendor demo's wake: nudge the module's I2C transmitter after it idles.
void wake() {
  TimeoutGuard to;
  reopen();
  delay(STEP_MS);
  if (touchAddr(CR_ADDR) != 0) return;
  delay(STEP_MS);
  uint32_t freeLen = read32(0xAA510004UL);
  if (freeLen == 0xFFFFFFFFUL || freeLen < 1) return;
  delay(STEP_MS);
  if (writeCr(0xAA531000UL, 1) != 0) return;
  delay(STEP_MS);
  touchAddr(WR_ADDR);
}

}  // namespace

bool begin() {
  Wire.setBufferSize(CHUNK + 16);
  TimeoutGuard to;
  reopen();
  delay(STEP_MS);
  return touchAddr(CR_ADDR) == 0;
}

void service(void (*sink)(char c)) {
  const uint32_t now = millis();
  if (s_last && now - s_last < PERIOD_MS) return;
  s_last = now ? now : 1;
  const int r = poll(sink);
  if (r < 0) {
    s_fails++;
  } else if (r == 0) {
    if (++s_empty >= EMPTY_WAKE_CYCLES) { s_empty = 0; wake(); }
  } else {
    s_empty = 0;
    s_bytes += r;
  }
}

uint32_t bytesRead() { return s_bytes; }
uint32_t failedCycles() { return s_fails; }

}  // namespace lc76g
}  // namespace marauder

#endif  // HAS_GPS_LC76G_I2C
