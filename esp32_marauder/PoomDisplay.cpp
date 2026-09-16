#include "PoomDisplay.h"

#ifdef MARAUDER_POOM

#include "libraries/TFT_eSPI/Fonts/glcdfont.c"

namespace {
constexpr uint8_t kAddress = 0x3C;
constexpr uint8_t kColumnOffset = 2;
}

PoomDisplay::PoomDisplay() { memset(framebuffer_, 0, sizeof(framebuffer_)); }

void PoomDisplay::command(uint8_t value) {
  Wire.beginTransmission(kAddress);
  Wire.write(0x00);
  Wire.write(value);
  Wire.endTransmission();
}

void PoomDisplay::init() {
  Wire.begin(0, 1, 100000);
  delay(20);
  const uint8_t init_sequence[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0xAD, 0x8B,
      0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0x22, 0xDB, 0x35,
      0xA4, 0xA6, 0xAF};
  for (uint8_t value : init_sequence) command(value);
  fillScreen(TFT_BLACK);
  display(true);
}

void PoomDisplay::display(bool force) {
  if (!dirty_ && !force) return;
  const uint32_t now = millis();
  if (!force && now - last_flush_ms_ < 33) return;
  for (uint8_t page = 0; page < 8; ++page) {
    command(0xB0 | page);
    command(0x00 | (kColumnOffset & 0x0F));
    command(0x10 | (kColumnOffset >> 4));
    for (uint8_t chunk = 0; chunk < 8; ++chunk) {
      Wire.beginTransmission(kAddress);
      Wire.write(0x40);
      Wire.write(framebuffer_ + page * 128 + chunk * 16, 16);
      Wire.endTransmission();
    }
  }
  dirty_ = false;
  last_flush_ms_ = now;
}

void PoomDisplay::fillScreen(uint16_t color) {
  memset(framebuffer_, white(color) ? 0xFF : 0x00, sizeof(framebuffer_));
  dirty_ = true;
}

void PoomDisplay::drawPixel(int32_t x, int32_t y, uint16_t color) {
  if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
  uint8_t &cell = framebuffer_[x + (y >> 3) * 128];
  const uint8_t mask = 1U << (y & 7);
  if (white(color)) cell |= mask; else cell &= ~mask;
  dirty_ = true;
}

void PoomDisplay::drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color) {
  for (int32_t i = 0; i < w; ++i) drawPixel(x + i, y, color);
}
void PoomDisplay::drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color) {
  for (int32_t i = 0; i < h; ++i) drawPixel(x, y + i, color);
}
void PoomDisplay::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  int32_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;
  while (true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    const int32_t e2 = err * 2;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
void PoomDisplay::drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  drawFastHLine(x, y, w, color); drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color); drawFastVLine(x + w - 1, y, h, color);
}
void PoomDisplay::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
  for (int32_t row = 0; row < h; ++row) drawFastHLine(x, y + row, w, color);
}
void PoomDisplay::drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color) { drawRect(x, y, w, h, color); }
void PoomDisplay::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t, uint16_t color) { fillRect(x, y, w, h, color); }
void PoomDisplay::fillCircle(int32_t x0, int32_t y0, int32_t r, uint16_t color) {
  for (int32_t y = -r; y <= r; ++y) for (int32_t x = -r; x <= r; ++x)
    if (x * x + y * y <= r * r) drawPixel(x0 + x, y0 + y, color);
}

void PoomDisplay::drawXBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w, int32_t h, uint16_t color) {
  for (int32_t row = 0; row < h; ++row) for (int32_t col = 0; col < w; ++col)
    if (pgm_read_byte(bitmap + row * ((w + 7) / 8) + col / 8) & (1U << (col & 7))) drawPixel(x + col, y + row, color);
}
void PoomDisplay::drawXBitmap(int32_t x, int32_t y, const uint8_t *bitmap, int32_t w, int32_t h, uint16_t fg, uint16_t bg) {
  fillRect(x, y, w, h, bg); drawXBitmap(x, y, bitmap, w, h, fg);
}

void PoomDisplay::setCursor(int32_t x, int32_t y) { cursor_x_ = x; cursor_y_ = y; }
void PoomDisplay::setTextColor(uint16_t fg, uint16_t bg) { text_fg_ = fg; text_bg_ = bg; }
void PoomDisplay::setTextSize(uint8_t size) { text_size_ = size ? size : 1; }
void PoomDisplay::setTextWrap(bool x, bool y) { wrap_x_ = x; wrap_y_ = y; }
int16_t PoomDisplay::textWidth(const String &text, uint8_t) const { return text.length() * 6 * text_size_; }
int16_t PoomDisplay::textWidth(const char *text, uint8_t) const { return strlen(text) * 6 * text_size_; }

void PoomDisplay::drawChar(int16_t x, int16_t y, uint8_t c, uint16_t fg, uint16_t bg, uint8_t size) {
  for (uint8_t col = 0; col < 6; ++col) {
    uint8_t line = col == 5 ? 0 : pgm_read_byte(font + c * 5 + col);
    for (uint8_t row = 0; row < 8; ++row) {
      const uint16_t color = (line & 1) ? fg : bg;
      if (size == 1) drawPixel(x + col, y + row, color);
      else fillRect(x + col * size, y + row * size, size, size, color);
      line >>= 1;
    }
  }
}

size_t PoomDisplay::write(uint8_t value) {
  if (value == '\n') { cursor_x_ = 0; cursor_y_ += 8 * text_size_; return 1; }
  if (value == '\r') return 1;
  if (wrap_x_ && cursor_x_ + 6 * text_size_ > 128) { cursor_x_ = 0; cursor_y_ += 8 * text_size_; }
  if (wrap_y_ && cursor_y_ + 8 * text_size_ > 64) cursor_y_ = 0;
  drawChar(cursor_x_, cursor_y_, value, text_fg_, text_bg_, text_size_);
  cursor_x_ += 6 * text_size_;
  return 1;
}

int16_t PoomDisplay::drawString(const String &text, int32_t x, int32_t y, uint8_t) { setCursor(x, y); print(text); return textWidth(text); }
int16_t PoomDisplay::drawString(const char *text, int32_t x, int32_t y, uint8_t) { return drawString(String(text), x, y); }
int16_t PoomDisplay::drawCentreString(const String &text, int32_t x, int32_t y, uint8_t font) { return drawString(text, x - textWidth(text, font) / 2, y, font); }
int16_t PoomDisplay::drawCentreString(const char *text, int32_t x, int32_t y, uint8_t font) { return drawCentreString(String(text), x, y, font); }
int16_t PoomDisplay::drawRightString(const String &text, int32_t x, int32_t y, uint8_t font) { return drawString(text, x - textWidth(text, font), y, font); }
int16_t PoomDisplay::drawRightString(const char *text, int32_t x, int32_t y, uint8_t font) { return drawRightString(String(text), x, y, font); }

void PoomButton::initButton(PoomDisplay *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                            uint16_t outline, uint16_t fill, uint16_t textcolor,
                            char *label, uint8_t textsize) {
  gfx_ = gfx; x_ = x; y_ = y; w_ = w; h_ = h; outline_ = outline; fill_ = fill;
  text_ = textcolor; text_size_ = textsize; label_ = label ? label : "";
}
void PoomButton::setLabelDatum(int16_t x, int16_t y, uint8_t) { label_dx_ = x; label_dy_ = y; }
void PoomButton::drawButton(bool inverted, String long_name) {
  if (!gfx_) return;
  const uint16_t fill = inverted ? text_ : fill_;
  const uint16_t text = inverted ? fill_ : text_;
  gfx_->fillRect(x_ - w_ / 2, y_ - h_ / 2, w_, h_, fill);
  gfx_->drawRect(x_ - w_ / 2, y_ - h_ / 2, w_, h_, outline_);
  gfx_->setTextColor(text, fill); gfx_->setTextSize(text_size_);
  gfx_->drawCentreString(long_name.length() ? long_name : label_, x_ + label_dx_, y_ - 4 + label_dy_);
}
bool PoomButton::contains(int16_t x, int16_t y) const { return x >= x_ - w_ / 2 && x < x_ + w_ / 2 && y >= y_ - h_ / 2 && y < y_ + h_ / 2; }
void PoomButton::press(bool pressed) { laststate_ = currstate_; currstate_ = pressed; }

#endif  // MARAUDER_POOM
