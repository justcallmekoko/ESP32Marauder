#include "configs.h"
#ifdef MARAUDER_M5PAPER
// Include vendor headers before the TFT_eSPI compatibility color macros.
// Adafruit owns the global GFXfont type; Lovyan has a separate namespaced type.
#include <Adafruit_GFX.h>
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_IT8951.hpp>
#include <lgfx/v1/platforms/esp32/Bus_SPI.hpp>
#include "M5PaperDisplay.h"
#include "M5PaperPower.h"
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_heap_caps.h>
#include <cstring>

namespace {
class PaperPanel : public lgfx::LGFX_Device {
 public:
  lgfx::Bus_SPI bus;
  lgfx::Panel_IT8951 controller;
  PaperPanel() {
    auto b = bus.config();
    b.spi_host = SPI3_HOST;
    b.spi_mode = 0;
    b.freq_write = 40000000; // M5Stack's standard M5Paper timing, no overclock.
    // Short status reads do not benefit measurably from 20 MHz. This board
    // returned intermittent zero VCOM reads there; use a conservative margin.
    b.freq_read = 4000000;
    b.pin_sclk = 14; b.pin_mosi = 12; b.pin_miso = 13; b.pin_dc = -1;
    b.spi_3wire = false; b.use_lock = true; b.dma_channel = 1;
    bus.config(b);
    controller.bus(&bus);
    auto p = controller.config();
    p.panel_width = p.memory_width = 960;
    p.panel_height = p.memory_height = 540;
    p.pin_cs = 15; p.pin_rst = 23; p.pin_busy = 27;
    p.offset_rotation = 3; p.bus_shared = true;
    controller.config(p);
    setPanel(&controller);
  }
};
PaperPanel panel;
constexpr uint16_t kTransferRows = 32;
constexpr uint8_t kCleanAfter = 32;
}

void M5PaperDisplay::prepareBoard() {
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_2, 1);
  gpio_deep_sleep_hold_dis();
  for (int pin : {2, 5, 23}) gpio_hold_dis(gpio_num_t(pin));
  rtc_gpio_deinit(GPIO_NUM_38);
  for (int pin : {2, 4, 15, 23}) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
  // The external Grove 5 V boost rail is unnecessary for the built-in hardware.
  pinMode(5, OUTPUT); digitalWrite(5, LOW);
  for (int pin : {36, 37, 38, 39, 27}) pinMode(pin, INPUT);
}

void M5PaperDisplay::init() {
  if (initialized_) return;
  prepareBoard();
  const auto releaseBuffers = [this]() {
    free(canvas_); free(snapshot_); free(previous_); free(transfer_);
    canvas_ = snapshot_ = previous_ = transfer_ = nullptr;
  };
  canvas_ = static_cast<uint8_t*>(heap_caps_malloc(m5paper::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  snapshot_ = static_cast<uint8_t*>(heap_caps_malloc(m5paper::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  previous_ = static_cast<uint8_t*>(heap_caps_malloc(m5paper::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  transfer_ = static_cast<uint8_t*>(heap_caps_malloc(m5paper::kWidth * kTransferRows, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!canvas_ || !snapshot_ || !previous_ || !transfer_) {
    Serial.println("M5Paper: PSRAM allocation failed; display disabled");
    releaseBuffers();
    return;
  }
  memset(canvas_, 0xff, m5paper::kFrameBytes);
  memset(snapshot_, 0xff, m5paper::kFrameBytes);
  memset(previous_, 0xff, m5paper::kFrameBytes);
  panel.setAutoDisplay(false);
  if (!panel.init()) {
    Serial.println("M5Paper: IT8951 initialization failed"); releaseBuffers(); return;
  }
  panel.setAutoDisplay(false);
  panel.setRotation(0);
  for (int attempt = 0; attempt < 3; ++attempt) {
    controllerVcom_ = panel.controller.getVCOM();
    if (controllerVcom_ >= 500 && controllerVcom_ <= 5000) break;
    delay(20);
  }
  if (controllerVcom_ < 500 || controllerVcom_ > 5000) {
    Serial.println("M5Paper: invalid controller read; display disabled");
    releaseBuffers(); return;
  }
  initTouch();
  if (xTaskCreate(workerEntry, "m5paper_epd", 6144, this, 1, &worker_) != pdPASS) {
    Serial.println("M5Paper: display task allocation failed"); releaseBuffers(); return;
  }
  initialized_ = true; dirty_ = true;
  Serial.printf("M5Paper: IT8951 UI %dx%d PSRAM=%u CPU=%uMHz VCOM=%umV\n",
                panel.width(), panel.height(), unsigned(ESP.getPsramSize()),
                unsigned(ESP.getCpuFreqMHz()), controllerVcom_);
}

void M5PaperDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!canvas_) return;
  sticky::Point p;
  if (!m5paper::panelPoint(x, y, rotation, p)) return;
  uint8_t& cell = canvas_[p.y * m5paper::kStride + p.x / 8];
  const uint8_t mask = 0x80 >> (p.x & 7);
  const uint8_t value = ink(color) ? cell & ~mask : cell | mask;
  if (cell != value) { cell = value; dirty_ = true; lastDraw_ = millis(); }
}

void M5PaperDisplay::fillScreen(uint16_t color) {
  if (!canvas_) return;
  memset(canvas_, ink(color) ? 0 : 0xff, m5paper::kFrameBytes);
  dirty_ = true; lastDraw_ = millis();
}

void M5PaperDisplay::display(bool force) {
  if (!initialized_ || busy_.load(std::memory_order_acquire)) return;
  const uint32_t now = millis();
  if (!lastOk_) {
    if (uint32_t(now - lastSubmit_) < 15000) return;
    dirty_ = true;
  }
  if (!force && ((!dirty_ && !cleanRequested_ && panelValid_) || uint32_t(now - lastSubmit_) < 100)) return;
  if (!force && uint32_t(now - lastDraw_) < 25 && uint32_t(now - lastSubmit_) < 250) return;
  jobArea_ = m5paper::transferArea(m5paper::difference(canvas_, previous_));
  if (jobArea_.empty() && panelValid_ && !cleanRequested_) { dirty_ = false; return; }
  jobFull_ = !panelValid_ || cleanRequested_ || partialCount_ >= kCleanAfter;
  if (jobFull_) jobArea_ = {0, 0, m5paper::kWidth, m5paper::kHeight};
  memcpy(snapshot_, canvas_, m5paper::kFrameBytes);
  dirty_ = false; cleanRequested_ = false; lastSubmit_ = now;
  busy_.store(true, std::memory_order_release);
  xTaskNotifyGive(worker_);
}

bool M5PaperDisplay::refresh() {
  const auto a = jobArea_;
  const bool small = uint32_t(a.w) * a.h < m5paper::kWidth * m5paper::kHeight / 5;
  panel.setEpdMode(jobFull_ ? lgfx::epd_mode_t::epd_quality :
                   small ? lgfx::epd_mode_t::epd_fastest : lgfx::epd_mode_t::epd_fast);
  // Upload bounded strips. Each pushImage releases the same Arduino SPI lock
  // used by SD; the slow panel waveform runs after the bus has been released.
  for (uint16_t row = 0; row < a.h; row += kTransferRows) {
    const uint16_t rows = min(kTransferRows, uint16_t(a.h - row));
    for (uint16_t y = 0; y < rows; ++y) {
      const auto* source = snapshot_ + (a.y + row + y) * m5paper::kStride;
      for (uint16_t x = 0; x < a.w; ++x) {
        const uint16_t px = a.x + x;
        transfer_[size_t(y) * a.w + x] = source[px / 8] & (0x80 >> (px & 7)) ? 255 : 0;
      }
    }
    panel.pushImage(a.x, a.y + row, a.w, rows, reinterpret_cast<const lgfx::grayscale_t*>(transfer_));
    taskYIELD();
  }
  panel.display();
  const uint32_t started = millis();
  while (panel.displayBusy()) {
    if (uint32_t(millis() - started) >= 8000) return false;
    delay(5);
  }
  return true;
}

void M5PaperDisplay::workerEntry(void* argument) {
  auto* self = static_cast<M5PaperDisplay*>(argument);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    const uint32_t start = millis();
    self->lastOk_ = self->refresh();
    self->lastRefreshMs_ = millis() - start;
    if (self->lastOk_) {
      memcpy(self->previous_, self->snapshot_, m5paper::kFrameBytes);
      self->panelValid_ = true; ++self->refreshes_;
      if (self->jobFull_) self->partialCount_ = 0;
      else { ++self->partialCount_; ++self->partials_; }
    } else { ++self->errors_; self->panelValid_ = false; }
    Serial.printf("M5Paper refresh: %s %ux%u %lums %s\n", self->jobFull_ ? "clean" : "partial",
                  self->jobArea_.w, self->jobArea_.h, (unsigned long)self->lastRefreshMs_, self->lastOk_ ? "OK" : "FAILED");
    self->busy_.store(false, std::memory_order_release);
  }
}

bool M5PaperDisplay::touchRead(uint16_t address, uint8_t* data, size_t len) {
  touchBus_.beginTransmission(touchAddress_);
  touchBus_.write(uint8_t(address >> 8)); touchBus_.write(uint8_t(address));
  if (touchBus_.endTransmission(false)) return false;
  if (touchBus_.requestFrom(touchAddress_, len) != len) return false;
  for (size_t i = 0; i < len; ++i) data[i] = touchBus_.read();
  return true;
}

bool M5PaperDisplay::touchWrite(uint16_t address, uint8_t value) {
  touchBus_.beginTransmission(touchAddress_);
  touchBus_.write(uint8_t(address >> 8)); touchBus_.write(uint8_t(address)); touchBus_.write(value);
  return touchBus_.endTransmission() == 0;
}

void M5PaperDisplay::initTouch() {
  touch_ = {}; touchAddress_ = 0;
  // GPIO36 is input-only. M5Paper has no MCU-controlled GT911 reset pin.
  pinMode(36, INPUT);
  touchBus_.begin(21, 22, 400000); touchBus_.setTimeOut(20);
  uint8_t id[4]{};
  for (uint8_t address : {uint8_t(0x5d), uint8_t(0x14)}) {
    touchAddress_ = address;
    if (touchRead(0x8140, id, 4) && id[0] == '9' && id[1] == '1' && id[2] == '1') break;
    touchAddress_ = 0;
  }
  if (!touchAddress_) { Serial.println("M5Paper: GT911 not found"); return; }
  uint8_t range[4]{};
  if (touchRead(0x8146, range, 4)) {
    const uint16_t x = range[0] | uint16_t(range[1]) << 8;
    const uint16_t y = range[2] | uint16_t(range[3]) << 8;
    if (x && y) { touchMaxX_ = x; touchMaxY_ = y; }
  }
  touchWrite(0x814e, 0);
  Serial.printf("M5Paper: GT911 address=0x%02x range=%ux%u INT=%d\n", touchAddress_, touchMaxX_, touchMaxY_, digitalRead(36));
}

bool M5PaperDisplay::getTouch(uint16_t* x, uint16_t* y, uint16_t) {
  m5paper::servicePowerButton(); display();
  if (!touchAddress_) return false;
  const uint32_t now = millis();
  if (uint32_t(now - lastTouchPoll_) >= 8) {
    lastTouchPoll_ = now; ++touchPolls_;
    uint8_t status = 0, data[7]{};
    bool ok = touchRead(0x814e, &status, 1), valid = false;
    sticky::Point next{};
    const uint8_t count = status & 15;
    lastTouchStatus_ = status;
    if (ok && (status & 0x80)) {
      ++touchReady_;
      if (count > 0 && count <= 5) {
        ok = touchRead(0x814f, data, sizeof(data));
        if (ok) {
          rawTouchX_ = data[1] | uint16_t(data[2]) << 8;
          rawTouchY_ = data[3] | uint16_t(data[4]) << 8;
          valid = m5paper::touchPoint(rawTouchX_, rawTouchY_, touchMaxX_, touchMaxY_, rotation, next);
          if (!valid) ++touchRejected_;
        }
      }
      ok = touchWrite(0x814e, 0) && ok;
    }
    if (!ok) ++touchErrors_;
    if (touch_.apply(ok, status, valid, next, now))
      Serial.printf("M5Paper touch: %s x=%u y=%u\n", touch_.down ? "down" : "up", touch_.point.x, touch_.point.y);
  }
  if (touch_.down) { *x = touch_.point.x; *y = touch_.point.y; }
  return touch_.down;
}

void M5PaperDisplay::diagnostics() {
  if (busy_.load(std::memory_order_acquire)) { Serial.println("M5Paper: refresh active"); return; }
  Serial.printf("M5Paper: refreshes=%lu partials=%lu errors=%lu last_ms=%lu heap=%u psram_free=%u\n",
                (unsigned long)refreshes_, (unsigned long)partials_, (unsigned long)errors_,
                (unsigned long)lastRefreshMs_, ESP.getFreeHeap(), ESP.getFreePsram());
}
void M5PaperDisplay::touchDiagnostics() {
  Serial.printf("M5Paper input: polls=%lu ready=%lu errors=%lu rejected=%lu status=0x%02x raw=%u,%u down=%u INT=%d buttons=%d%d%d\n",
                (unsigned long)touchPolls_, (unsigned long)touchReady_, (unsigned long)touchErrors_,
                (unsigned long)touchRejected_, lastTouchStatus_, rawTouchX_, rawTouchY_, touch_.down,
                digitalRead(36), digitalRead(37), digitalRead(38), digitalRead(39));
}
void M5PaperDisplay::dumpFrame() {
  if (!canvas_ || busy_.load(std::memory_order_acquire)) { Serial.println("M5Paper: retry dump after refresh"); return; }
  Serial.println("PAPER_FRAME_BEGIN 540 960 65280");
  Serial.write(canvas_, m5paper::kFrameBytes);
  Serial.println("\nPAPER_FRAME_END");
}
void M5PaperDisplay::startDiagnostic() {
  diagnostic_ = true; diagnosticDown_ = false;
  setRotation(0); setFreeFont(nullptr); fillScreen(TFT_BLACK);
  drawHeading("M5Paper touch test");
  drawCentreString("Tap the five targets", 270, 140);
  const sticky::Point targets[] = {{40, 220}, {499, 220}, {270, 480}, {40, 780}, {499, 780}};
  for (const auto& p : targets) {
    drawCircle(p.x, p.y, 20, TFT_WHITE);
    drawLine(p.x - 26, p.y, p.x + 26, p.y, TFT_WHITE);
    drawLine(p.x, p.y - 26, p.x, p.y + 26, TFT_WHITE);
  }
  drawCentreString("Center button: return to menu", 270, 920);
  requestClean(); display(true);
}
void M5PaperDisplay::diagnosticTick() {
  if (!diagnostic_) return;
  diagnosticInk_ = !diagnosticInk_;
  fillRect(230, 300, 80, 40, diagnosticInk_ ? TFT_WHITE : TFT_BLACK);
  display(true);
}
void M5PaperDisplay::serviceDiagnostic() {
  uint16_t x = 0, y = 0;
  const bool down = getTouch(&x, &y);
  if (down && !diagnosticDown_) {
    fillCircle(x, y, 6, TFT_WHITE);
    fillRect(60, 870, 420, 24, TFT_BLACK);
    setTextSize(2); setTextColor(TFT_WHITE, TFT_BLACK);
    drawCentreString("Touch " + String(x) + ", " + String(y), 270, 870);
  }
  diagnosticDown_ = down; display();
}
void M5PaperDisplay::showPowerOff() {
  if (!initialized_) return;
  uint32_t started = millis();
  while (busy_.load(std::memory_order_acquire)) {
    if (uint32_t(millis() - started) >= 15000) return;
    delay(5);
  }
  diagnostic_ = false; setRotation(0); fillScreen(TFT_BLACK);
  drawHeading("Powered off");
  drawCentreString("Press center to start", 270, 450);
  drawCentreString("Release the button", 270, 494);
  requestClean(); display(true); started = millis();
  while (busy_.load(std::memory_order_acquire)) {
    if (uint32_t(millis() - started) >= 15000) return;
    delay(5);
  }
  panel.sleep();
}
#endif
