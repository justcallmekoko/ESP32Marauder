#include "configs.h"
#ifdef MARAUDER_M5PAPER
#include "M5PaperDisplay.h"
int16_t M5PaperDisplay::textWidth(const String& s, uint8_t) {
  int16_t x, y; uint16_t w, h;
  getTextBounds(s, 0, 0, &x, &y, &w, &h);
  return w;
}
int16_t M5PaperDisplay::drawString(const String& s, int32_t x, int32_t y, uint8_t) {
  setCursor(x, y); print(s); return textWidth(s);
}
int16_t M5PaperDisplay::drawCentreString(const String& s, int32_t x, int32_t y, uint8_t f) {
  return drawString(s, x - textWidth(s, f) / 2, y, f);
}
int16_t M5PaperDisplay::drawRightString(const String& s, int32_t x, int32_t y, uint8_t f) {
  return drawString(s, x - textWidth(s, f), y, f);
}
void M5PaperDisplay::drawXBitmap(int16_t x, int16_t y, const uint8_t* b, int16_t w, int16_t h, uint16_t fg) {
  Adafruit_GFX::drawXBitmap(x, y, b, w, h, fg);
}
void M5PaperDisplay::drawXBitmap(int16_t x, int16_t y, const uint8_t* b, int16_t w, int16_t h, uint16_t fg, uint16_t bg) {
  fillRect(x, y, w, h, bg); drawXBitmap(x, y, b, w, h, fg);
}

void M5PaperButton::initButton(M5PaperDisplay* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                             uint16_t outline, uint16_t fill, uint16_t text, char* label, uint8_t size) {
  gfx_ = gfx; x_ = x; y_ = y; w_ = w; h_ = h; outline_ = outline; fill_ = fill;
  text_ = text; text_size_ = size; label_ = label ? label : "";
}
void M5PaperButton::setLabelDatum(int16_t x, int16_t y, uint8_t datum) {
  label_dx_ = x; label_dy_ = y; datum_ = datum;
}
void M5PaperButton::drawButton(bool inverted, String name) {
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
bool M5PaperButton::contains(int16_t x, int16_t y) const {
  return x >= x_ - w_ / 2 && x < x_ + w_ / 2 && y >= y_ - h_ / 2 && y < y_ + h_ / 2;
}
void M5PaperButton::press(bool pressed) { laststate_ = currstate_; currstate_ = pressed; }
#endif
