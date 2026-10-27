#include "configs.h"
#ifdef MARAUDER_RETERMINAL_STICKY
#include "StickyDisplay.h"
#include "StickyPower.h"
#include <esp_heap_caps.h>
#include <driver/gpio.h>
#include <cstring>

extern SPIClass sharedSPI;
namespace {
constexpr int kCs = 15, kDc = 16, kReset = 17, kBusy = 18;
constexpr int kTouchReset = 41, kTouchInt = 21;
// SSD1677 datasheet table 12-1 allows 20 MHz writes. The panel OTP waveform
// and voltages are unchanged; SD supplies its own transaction clock.
constexpr uint32_t kSpiHz = 20000000;
constexpr uint8_t kCleanAfter = 24;
}

void StickyDisplay::prepareBoard() {
  // Restore retained pins after USB-powered deep sleep, then clock the
  // hardware power latch on. GPIO46 is a clock, not a static enable.
  gpio_set_direction(GPIO_NUM_45, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_45, 1);
  gpio_set_direction(GPIO_NUM_46, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_46, 0);
  gpio_deep_sleep_hold_dis();
  for (int pin : {45, 46, 4, 47, 42, 41, 38, 10, 48}) gpio_hold_dis(gpio_num_t(pin));
  digitalWrite(45, HIGH);
  digitalWrite(46, LOW);
  delayMicroseconds(10);
  digitalWrite(46, HIGH);
  // Power latches, SD rail, display rail, touch rail. Do not use a TFT
  // backlight pin on this board: power removal would invalidate panel RAM.
  const int highPins[] = {45, 46, 10, 47, 42, 8, kCs};
  for (int pin : highPins) { pinMode(pin, OUTPUT); digitalWrite(pin, HIGH); }
  pinMode(39, OUTPUT); digitalWrite(39, LOW); // BQ25616 charging enable
  pinMode(38, OUTPUT); digitalWrite(38, LOW); // unused microphone rail
}

void StickyDisplay::init() {
  if (initialized_) return; // Marauder calls init again on menu transitions.
  prepareBoard();
  pinMode(kDc, OUTPUT); pinMode(kReset, OUTPUT); pinMode(kBusy, INPUT);
  digitalWrite(kReset, HIGH);
  canvas_ = static_cast<uint8_t*>(heap_caps_malloc(sticky::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  snapshot_ = static_cast<uint8_t*>(heap_caps_malloc(sticky::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  previous_ = static_cast<uint8_t*>(heap_caps_malloc(sticky::kFrameBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!canvas_ || !snapshot_ || !previous_) {
    Serial.println("Sticky: framebuffer allocation failed");
    free(canvas_); free(snapshot_); free(previous_);
    canvas_ = snapshot_ = previous_ = nullptr;
    return;
  }
  memset(canvas_, 0xff, sticky::kFrameBytes);
  memset(snapshot_, 0xff, sticky::kFrameBytes);
  memset(previous_, 0xff, sticky::kFrameBytes);
  initTouch();
  if (xTaskCreate(workerEntry, "sticky_epd", 4096, this, 1, &worker_) != pdPASS) {
    Serial.println("Sticky: display task allocation failed");
    free(canvas_); free(snapshot_); free(previous_);
    canvas_ = snapshot_ = previous_ = nullptr;
    return;
  }
  initialized_ = true;
  dirty_ = true;
  Serial.printf("Sticky: SSD1677 800x480, UI 480x800, PSRAM=%u, CPU=%uMHz\n",
                unsigned(ESP.getPsramSize()), unsigned(ESP.getCpuFreqMHz()));
}

void StickyDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!canvas_) return;
  sticky::Point p;
  if (!sticky::panelPoint(x, y, rotation, p)) return;
  uint8_t& cell = canvas_[p.y * sticky::kStride + p.x / 8];
  const uint8_t mask = 0x80 >> (p.x & 7);
  // Marauder's black background becomes white paper; colored foregrounds
  // become black ink, preserving text contrast regardless of RGB luminance.
  const uint8_t value = ink(color) ? cell & ~mask : cell | mask;
  if (cell != value) { cell = value; dirty_ = true; lastDraw_ = millis(); }
}

void StickyDisplay::fillScreen(uint16_t color) {
  if (!canvas_) return;
  memset(canvas_, ink(color) ? 0 : 0xff, sticky::kFrameBytes);
  dirty_ = true;
  lastDraw_ = millis();
}

void StickyDisplay::display(bool force) {
  if (!initialized_ || busy_.load(std::memory_order_acquire)) return;
  const uint32_t now = millis();
  if (!lastOk_) {
    if (uint32_t(now - lastSubmit_) < 15000) return;
    dirty_ = true;
  }
  if (!force && ((!dirty_ && !cleanRequested_ && panelValid_) || uint32_t(now - lastSubmit_) < 150)) return;
  // Coalesce a burst of primitive draws, with a 400ms bound for live views.
  if (!force && uint32_t(now - lastDraw_) < 35 && uint32_t(now - lastSubmit_) < 400) return;
  jobArea_ = sticky::difference(canvas_, previous_);
  if (jobArea_.empty() && panelValid_ && !cleanRequested_) { dirty_ = false; return; }
  jobFull_ = !panelValid_ || cleanRequested_ || partialCount_ >= kCleanAfter;
  if (jobFull_) jobArea_ = {0, 0, 800, 480};
  memcpy(snapshot_, canvas_, sticky::kFrameBytes);
  dirty_ = false;
  cleanRequested_ = false;
  lastSubmit_ = now;
  busy_.store(true, std::memory_order_release);
  xTaskNotifyGive(worker_);
}

void StickyDisplay::workerEntry(void* argument) {
  auto* self = static_cast<StickyDisplay*>(argument);
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    const uint32_t start = millis();
    self->lastOk_ = self->refresh();
    self->lastRefreshMs_ = millis() - start;
    if (self->lastOk_) {
      memcpy(self->previous_, self->snapshot_, sticky::kFrameBytes);
      self->panelValid_ = true;
      ++self->refreshes_;
      if (self->jobFull_) self->partialCount_ = 0;
      else { ++self->partialCount_; ++self->partials_; }
    } else {
      ++self->errors_;
      self->panelValid_ = false;
    }
    Serial.printf("Sticky refresh: %s %ux%u %lums %s\n", self->jobFull_ ? "full" : "partial",
                  self->jobArea_.w, self->jobArea_.h,
                  static_cast<unsigned long>(self->lastRefreshMs_), self->lastOk_ ? "OK" : "FAILED");
    // Publish completion only after all job state and previous plane updates.
    self->busy_.store(false, std::memory_order_release);
  }
}

void StickyDisplay::command(uint8_t cmd, const uint8_t* data, size_t len) {
  sharedSPI.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  digitalWrite(kCs, LOW); digitalWrite(kDc, LOW);
  sharedSPI.transfer(cmd);
  digitalWrite(kDc, HIGH);
  if (len) sharedSPI.writeBytes(data, len);
  digitalWrite(kCs, HIGH);
  sharedSPI.endTransaction();
}

bool StickyDisplay::waitReady(uint32_t timeout) {
  const uint32_t start = millis();
  delay(2);
  while (digitalRead(kBusy) == HIGH) {
    if (uint32_t(millis() - start) >= timeout) return false;
    delay(2); // SPI bus is released; touch, SD and main loop remain runnable.
  }
  return true;
}

void StickyDisplay::setWindow(const sticky::Area& a) {
  const uint16_t xe = a.x + a.w - 1, ye = a.y + a.h - 1;
  const uint8_t xs[] = {uint8_t(a.x), uint8_t(a.x >> 8), uint8_t(xe), uint8_t(xe >> 8)};
  const uint8_t ys[] = {uint8_t(a.y), uint8_t(a.y >> 8), uint8_t(ye), uint8_t(ye >> 8)};
  command(0x44, xs, 4); command(0x45, ys, 4);
  command(0x4e, xs, 2); command(0x4f, ys, 2);
}

void StickyDisplay::writePlane(uint8_t cmd, const uint8_t* frame, const sticky::Area& a) {
  setWindow(a);
  sharedSPI.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
  digitalWrite(kCs, LOW); digitalWrite(kDc, LOW); sharedSPI.transfer(cmd);
  digitalWrite(kDc, HIGH);
  for (uint16_t y = a.y; y < a.y + a.h; ++y)
    sharedSPI.writeBytes(frame + size_t(y) * sticky::kStride + a.x / 8, a.w / 8);
  digitalWrite(kCs, HIGH);
  sharedSPI.endTransaction();
}

bool StickyDisplay::refresh() {
  if (!waitReady()) return false;
  if (!panelValid_) {
    digitalWrite(kReset, LOW); delay(20); digitalWrite(kReset, HIGH); delay(20);
    if (!waitReady()) return false;
    command(0x12);
    if (!waitReady()) return false;
    reg(0x18, 0x80); // internal temperature sensor, vendor OTP waveform
    const uint8_t softstart[] = {0xae, 0xc7, 0xc3, 0xc0, 0x80};
    command(0x0c, softstart, sizeof(softstart));
    const uint8_t output[] = {0xdf, 0x01, 0x02};
    command(0x01, output, sizeof(output));
    reg(0x11, 0x03);
  }
  reg(0x3c, jobFull_ ? 0x01 : 0x80);
  // This controller compares full old/new planes during a partial waveform.
  // Seeed's demo_epaper_draw_dot also supplies a complete comparison frame.
  // Always restore both planes, even when only the selection cursor changed.
  const sticky::Area fullFrame{0, 0, 800, 480};
  writePlane(0x26, jobFull_ ? snapshot_ : previous_, fullFrame);
  writePlane(0x24, snapshot_, fullFrame);
  reg(0x22, jobFull_ ? 0xf7 : 0xff);
  command(0x20);
  if (!waitReady()) return false;
  // Both planes are explicitly written before the next activation, so an
  // extra post-refresh old-plane transfer is unnecessary.
  return true;
}

bool StickyDisplay::touchRead(uint16_t address, uint8_t* data, size_t len) {
  touchBus_.beginTransmission(touchAddress_);
  touchBus_.write(uint8_t(address >> 8)); touchBus_.write(uint8_t(address));
  if (touchBus_.endTransmission(false) != 0) return false;
  if (touchBus_.requestFrom(touchAddress_, len) != len) return false;
  for (size_t i = 0; i < len; ++i) data[i] = touchBus_.read();
  return true;
}

bool StickyDisplay::touchWrite(uint16_t address, uint8_t value) {
  touchBus_.beginTransmission(touchAddress_);
  touchBus_.write(uint8_t(address >> 8)); touchBus_.write(uint8_t(address)); touchBus_.write(value);
  return touchBus_.endTransmission() == 0;
}

void StickyDisplay::initTouch() {
  touch_ = {};
  gpio_hold_dis(gpio_num_t(kTouchReset));
  gpio_hold_dis(gpio_num_t(kTouchInt));
  gpio_hold_dis(GPIO_NUM_42);
  pinMode(kTouchReset, OUTPUT); digitalWrite(kTouchReset, LOW);
  pinMode(kTouchInt, OUTPUT); digitalWrite(kTouchInt, LOW);
  pinMode(42, OUTPUT); digitalWrite(42, LOW); delay(20);
  digitalWrite(42, HIGH); delay(250);
  touchBus_.begin(3, 2, 100000);
  touchBus_.setTimeOut(20);
  uint8_t id[4]{};
  for (uint8_t address : {uint8_t(0x14), uint8_t(0x5d)}) {
    pinMode(kTouchInt, OUTPUT);
    digitalWrite(kTouchReset, LOW);
    digitalWrite(kTouchInt, address == 0x14 ? HIGH : LOW); delay(20);
    digitalWrite(kTouchReset, HIGH); delay(6);
    // Goodix INT synchronization after reset/address selection. Releasing
    // INT high can leave an accessible ID without a running touch engine.
    digitalWrite(kTouchInt, LOW); delay(50);
    pinMode(kTouchInt, INPUT); delay(100);
    touchAddress_ = address;
    if (touchRead(0x8140, id, 4) && id[0] == '9' && id[1] == '1' && id[2] == '1') break;
    touchAddress_ = 0;
  }
  if (!touchAddress_) { Serial.println("Sticky: GT911 not found; physical buttons remain available"); return; }
  uint8_t range[4]{};
  if (touchRead(0x8146, range, 4)) {
    const uint16_t x = range[0] | uint16_t(range[1]) << 8;
    const uint16_t y = range[2] | uint16_t(range[3]) << 8;
    if (x && y) { touchMaxX_ = x; touchMaxY_ = y; }
  }
  touchWrite(0x814e, 0);
  Serial.printf("Sticky: GT911 address=0x%02x reported=%ux%u mapping=%ux%u\n",
                touchAddress_, range[0] | uint16_t(range[1]) << 8,
                range[2] | uint16_t(range[3]) << 8, touchMaxX_, touchMaxY_);
}

bool StickyDisplay::getTouch(uint16_t* x, uint16_t* y, uint16_t) {
  sticky::servicePowerButton();
  display(); // Also service blocking upstream keyboard/graph input loops.
  if (!touchAddress_) return false;
  const uint32_t now = millis();
  if (uint32_t(now - lastTouchPoll_) >= 8) {
    lastTouchPoll_ = now;
    ++touchPolls_;
    uint8_t status = 0, data[7]{};
    bool ok = touchRead(0x814e, &status, 1), valid = false;
    sticky::Point next{};
    const uint8_t count = status & 0x0f;
    lastTouchStatus_ = status;
    if (ok && (status & 0x80)) {
      ++touchReady_;
      if (count > 0 && count <= 5) {
        // 0x814f is tracking ID; X starts at 0x8150, little-endian.
        ok = touchRead(0x814f, data, sizeof(data));
        if (ok) {
          rawTouchX_ = data[1] | uint16_t(data[2]) << 8;
          rawTouchY_ = data[3] | uint16_t(data[4]) << 8;
          valid = sticky::touchPoint(rawTouchX_, rawTouchY_, touchMaxX_, touchMaxY_, rotation, next);
          if (!valid) ++touchRejected_;
        }
      }
      // Acknowledge every ready report, including zero contacts and malformed
      // counts. Otherwise the controller can stop reporting new touches.
      ok = touchWrite(0x814e, 0) && ok;
    }
    if (!ok) ++touchErrors_;
    if (touch_.apply(ok, status, valid, next, now))
      Serial.printf("Sticky touch: %s x=%u y=%u\n", touch_.down ? "down" : "up", touch_.point.x, touch_.point.y);
  }
  if (touch_.down) { *x = touch_.point.x; *y = touch_.point.y; }
  return touch_.down;
}

void StickyDisplay::diagnostics() {
  const bool busy = busy_.load(std::memory_order_acquire);
  // Worker counters are read only while its completion is published.
  if (busy) { Serial.println("Sticky: display busy; input remains active"); return; }
  Serial.printf("Sticky: refreshes=%lu partials=%lu errors=%lu last_ms=%lu touch=0x%02x heap=%u\n",
    (unsigned long)refreshes_, (unsigned long)partials_, (unsigned long)errors_,
    (unsigned long)lastRefreshMs_, touchAddress_, unsigned(ESP.getFreeHeap()));
}

void StickyDisplay::touchDiagnostics() {
  Serial.printf("Sticky input: polls=%lu ready=%lu errors=%lu rejected=%lu status=0x%02x raw=%u,%u down=%u INT=%d RST=%d EN=%d\n",
    (unsigned long)touchPolls_, (unsigned long)touchReady_, (unsigned long)touchErrors_,
    (unsigned long)touchRejected_, lastTouchStatus_, rawTouchX_, rawTouchY_, touch_.down,
    digitalRead(kTouchInt), digitalRead(kTouchReset), digitalRead(42));
  for (uint16_t address : {uint16_t(0x8040), uint16_t(0x8047), uint16_t(0x8140)}) {
    uint8_t data[16]{};
    Serial.printf("GT911 %04x:", address);
    if (touchAddress_ && touchRead(address, data, sizeof(data)))
      for (uint8_t byte : data) Serial.printf(" %02x", byte);
    else Serial.print(" READ FAILED");
    Serial.println();
  }
}

void StickyDisplay::dumpFrame() {
  if (!canvas_ || busy_.load(std::memory_order_acquire)) {
    Serial.println("Sticky: retry dump after refresh completes"); return;
  }
  Serial.println("STICKY_FRAME_BEGIN 800 480 48000");
  Serial.write(canvas_, sticky::kFrameBytes);
  Serial.println("\nSTICKY_FRAME_END");
}

void StickyDisplay::startDiagnostic() {
  diagnostic_ = true;
  diagnosticDown_ = false;
  setRotation(0); fillScreen(TFT_BLACK);
  setTextSize(2); setTextColor(TFT_WHITE, TFT_BLACK); setTextWrap(false);
  drawCentreString("Sticky touch / refresh test", 240, 16);
  drawCentreString("Tap the five targets", 240, 44);
  const sticky::Point targets[] = {{32, 100}, {447, 100}, {240, 400}, {32, 700}, {447, 700}};
  for (const auto& p : targets) {
    drawCircle(p.x, p.y, 16, TFT_WHITE);
    drawLine(p.x - 22, p.y, p.x + 22, p.y, TFT_WHITE);
    drawLine(p.x, p.y - 22, p.x, p.y + 22, TFT_WHITE);
  }
  drawCentreString("Center button: return to menu", 240, 760);
  display(true);
}

void StickyDisplay::diagnosticTick() {
  if (!diagnostic_) return;
  diagnosticInk_ = !diagnosticInk_;
  fillRect(180, 180, 80, 40, diagnosticInk_ ? TFT_WHITE : TFT_BLACK);
  display(true);
}

void StickyDisplay::serviceDiagnostic() {
  uint16_t x = 0, y = 0;
  const bool down = getTouch(&x, &y);
  if (down && !diagnosticDown_) {
    fillCircle(x, y, 5, TFT_WHITE);
    fillRect(70, 730, 340, 20, TFT_BLACK);
    setTextSize(2); setTextColor(TFT_WHITE, TFT_BLACK);
    drawCentreString("Touch " + String(x) + ", " + String(y), 240, 730);
  }
  diagnosticDown_ = down;
  display();
}

int16_t StickyDisplay::textWidth(const String& s, uint8_t) {
  int16_t x, y; uint16_t w, h;
  getTextBounds(s, 0, 0, &x, &y, &w, &h);
  return w;
}
int16_t StickyDisplay::drawString(const String& s, int32_t x, int32_t y, uint8_t) {
  setCursor(x, y); print(s); return textWidth(s);
}
int16_t StickyDisplay::drawCentreString(const String& s, int32_t x, int32_t y, uint8_t f) {
  return drawString(s, x - textWidth(s, f) / 2, y, f);
}
int16_t StickyDisplay::drawRightString(const String& s, int32_t x, int32_t y, uint8_t f) {
  return drawString(s, x - textWidth(s, f), y, f);
}
void StickyDisplay::drawXBitmap(int16_t x, int16_t y, const uint8_t* b, int16_t w, int16_t h, uint16_t fg) {
  Adafruit_GFX::drawXBitmap(x, y, b, w, h, fg);
}
void StickyDisplay::drawXBitmap(int16_t x, int16_t y, const uint8_t* b, int16_t w, int16_t h, uint16_t fg, uint16_t bg) {
  fillRect(x, y, w, h, bg); drawXBitmap(x, y, b, w, h, fg);
}

void StickyButton::initButton(StickyDisplay* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                             uint16_t outline, uint16_t fill, uint16_t text, char* label, uint8_t size) {
  gfx_ = gfx; x_ = x; y_ = y; w_ = w; h_ = h; outline_ = outline; fill_ = fill;
  text_ = text; text_size_ = size; label_ = label ? label : "";
}
void StickyButton::setLabelDatum(int16_t x, int16_t y, uint8_t datum) {
  label_dx_ = x; label_dy_ = y; datum_ = datum;
}
void StickyButton::drawButton(bool inverted, String name) {
  if (!gfx_) return;
  // Binary contrast: colored settings fills must not collapse into their
  // colored text. Selection is expressed with ink/white inversion.
  const uint16_t fill = inverted ? TFT_WHITE : TFT_BLACK;
  const uint16_t text = inverted ? TFT_BLACK : TFT_WHITE;
  gfx_->fillRect(x_ - w_ / 2, y_ - h_ / 2, w_, h_, fill);
  gfx_->setTextColor(text, fill); gfx_->setTextSize(text_size_);
  const String label = name.length() ? name : label_;
  if (datum_ == ML_DATUM)
    gfx_->drawString(label, x_ + label_dx_, y_ - 4 * text_size_ + label_dy_);
  else
    gfx_->drawCentreString(label, x_ + label_dx_, y_ - 4 * text_size_ + label_dy_);
  if (fill_ == TFT_LIGHTGREY && (text_ == TFT_GREEN || text_ == TFT_RED)) {
    gfx_->drawRect(x_ + w_ / 2 - 25, y_ - 9, 18, 18, text);
    if (text_ == TFT_GREEN) gfx_->fillRect(x_ + w_ / 2 - 21, y_ - 5, 10, 10, text);
  }
}
bool StickyButton::contains(int16_t x, int16_t y) const {
  return x >= x_ - w_ / 2 && x < x_ + w_ / 2 && y >= y_ - h_ / 2 && y < y_ + h_ / 2;
}
void StickyButton::press(bool pressed) { laststate_ = currstate_; currstate_ = pressed; }
#endif
