#include "StickyModel.h"

namespace sticky {
int8_t menuZone(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  if (!width || !height || x >= width || y >= height) return -1;
  return uint32_t(y) * 3 / height;
}

bool touchPoint(uint16_t x, uint16_t y, uint16_t maxX, uint16_t maxY,
                uint8_t rotation, Point& p) {
  if (!maxX || !maxY || x > maxX || y > maxY) return false;
  const uint16_t px = (uint32_t(x) * 479 + maxX / 2) / maxX;
  const uint16_t py = (uint32_t(maxY - y) * 799 + maxY / 2) / maxY;
  switch (rotation & 3) {
    case 0: p = {px, py}; break;
    case 1: p = {py, uint16_t(479 - px)}; break;
    case 2: p = {uint16_t(479 - px), uint16_t(799 - py)}; break;
    default: p = {uint16_t(799 - py), px}; break;
  }
  return true;
}

bool panelPoint(int16_t x, int16_t y, uint8_t rotation, Point& p) {
  const bool landscape = rotation & 1;
  if (x < 0 || y < 0 || x >= (landscape ? 800 : 480) ||
      y >= (landscape ? 480 : 800)) return false;
  int16_t px = x, py = y;
  switch (rotation & 3) {
    case 1: px = 479 - y; py = x; break;
    case 2: px = 479 - x; py = 799 - y; break;
    case 3: px = y; py = 799 - x; break;
  }
  // Seeed demo_touch.cpp maps portrait pixels to (799 - py, px).
  // demo_epaper.cpp additionally enables mirror_x; epaper_panel.c mirrors
  // both the window and its bits before sending them to the controller.
  // Compose both transforms here, so full frames and dirty windows agree.
  p = {uint16_t(py), uint16_t(px)};
  return true;
}

Area difference(const uint8_t* a, const uint8_t* b) {
  uint16_t x0 = kStride, y0 = kPanelHeight, x1 = 0, y1 = 0;
  bool changed = false;
  for (uint16_t y = 0; y < kPanelHeight; ++y) {
    for (uint16_t x = 0; x < kStride; ++x) {
      const size_t i = size_t(y) * kStride + x;
      if (a[i] == b[i]) continue;
      changed = true;
      if (x < x0) x0 = x;
      if (x > x1) x1 = x;
      if (y < y0) y0 = y;
      if (y > y1) y1 = y;
    }
  }
  return changed ? Area{uint16_t(x0 * 8), y0,
                        uint16_t((x1 - x0 + 1) * 8), uint16_t(y1 - y0 + 1)} : Area{};
}

bool DebouncedButton::update(bool pressed, uint32_t now) {
  if (!initialized || raw != pressed) {
    initialized = true;
    raw = pressed;
    changedAt = now;
  }
  if (raw == down || uint32_t(now - changedAt) < 25) return false;
  down = raw;
  return true;
}

PowerEvent PowerButton::update(bool pressed, uint32_t now) {
  const bool wasDown = button.down;
  const bool changed = button.update(pressed, now);
  // Ignore the press used to power on, even if it is still held at boot.
  if (!armed) {
    if (!button.raw && !button.down && uint32_t(now - button.changedAt) >= 25) armed = true;
    return PowerEvent::None;
  }
  if (changed && button.down) { pressedAt = now; held = false; }
  const uint32_t heldUntil = button.raw ? now : button.changedAt;
  if (wasDown && !held && uint32_t(heldUntil - pressedAt) >= 3000) {
    held = true;
    return PowerEvent::Shutdown;
  }
  if (changed && !button.down && !held) return PowerEvent::Select;
  return PowerEvent::None;
}

bool TouchState::apply(bool ok, uint8_t status, bool valid, Point next, uint32_t now) {
  const bool before = down;
  if (!ok) {
    if (uint32_t(now - lastGood) >= 250) down = false;
  } else {
    lastGood = now;
    if (status & 0x80) {
      const uint8_t count = status & 0x0f;
      down = count > 0 && count <= 5 && valid;
      if (down) point = next;
    }
  }
  return before != down;
}
}
