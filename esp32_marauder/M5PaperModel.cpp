#include "M5PaperModel.h"

namespace m5paper {
sticky::Area transferArea(sticky::Area dirty) {
  if (dirty.empty()) return dirty;
  // Portrait Y is native IT8951 X. Avoid its single four-pixel-word window
  // workaround, which can extend a one-pixel update beyond a panel edge.
  const uint16_t end = (dirty.y + dirty.h + 7) & ~7u;
  dirty.y &= ~7u;
  dirty.h = (end < kHeight ? end : kHeight) - dirty.y;
  return dirty;
}

bool panelPoint(int16_t x, int16_t y, uint8_t rotation, sticky::Point& p) {
  const bool landscape = rotation & 1;
  if (x < 0 || y < 0 || x >= (landscape ? kHeight : kWidth) ||
      y >= (landscape ? kWidth : kHeight)) return false;
  switch (rotation & 3) {
    case 0: p = {uint16_t(x), uint16_t(y)}; break;
    case 1: p = {uint16_t(kWidth - 1 - y), uint16_t(x)}; break;
    case 2: p = {uint16_t(kWidth - 1 - x), uint16_t(kHeight - 1 - y)}; break;
    default: p = {uint16_t(y), uint16_t(kHeight - 1 - x)}; break;
  }
  return true;
}

bool touchPoint(uint16_t x, uint16_t y, uint16_t maxX, uint16_t maxY,
                uint8_t rotation, sticky::Point& p) {
  if (!maxX || !maxY || x > maxX || y > maxY) return false;
  // GT911 reports axis lengths. Preserve pixel positions; clamp inclusive ends.
  const uint16_t px = uint32_t(x < maxX ? x : maxX - 1) * kWidth / maxX;
  const uint16_t py = uint32_t(y < maxY ? y : maxY - 1) * kHeight / maxY;
  switch (rotation & 3) {
    case 0: p = {px, py}; break;
    case 1: p = {py, uint16_t(kWidth - 1 - px)}; break;
    case 2: p = {uint16_t(kWidth - 1 - px), uint16_t(kHeight - 1 - py)}; break;
    default: p = {uint16_t(kHeight - 1 - py), px}; break;
  }
  return true;
}

sticky::Area difference(const uint8_t* a, const uint8_t* b) {
  uint16_t x0 = kStride, y0 = kHeight, x1 = 0, y1 = 0;
  bool changed = false;
  for (uint16_t y = 0; y < kHeight; ++y) {
    for (uint16_t x = 0; x < kStride; ++x) {
      const size_t i = size_t(y) * kStride + x;
      // Four padding bits at the end of each row are never display pixels.
      const uint8_t mask = x == kStride - 1 ? 0xf0 : 0xff;
      if (!((a[i] ^ b[i]) & mask)) continue;
      changed = true;
      if (x < x0) x0 = x;
      if (x > x1) x1 = x;
      if (y < y0) y0 = y;
      if (y > y1) y1 = y;
    }
  }
  const uint16_t right = (x1 + 1) * 8 < kWidth ? (x1 + 1) * 8 : kWidth;
  return changed ? sticky::Area{uint16_t(x0 * 8), y0,
                               uint16_t(right - x0 * 8), uint16_t(y1 - y0 + 1)} : sticky::Area{};
}
}
