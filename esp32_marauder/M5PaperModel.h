#pragma once
#include "StickyModel.h"

namespace m5paper {
constexpr uint16_t kWidth = 540, kHeight = 960;
constexpr size_t kStride = (kWidth + 7) / 8;
constexpr size_t kFrameBytes = kStride * kHeight;
// Canvas coordinates are portrait, matching M5GFX's M5Paper offset_rotation=3.
bool panelPoint(int16_t x, int16_t y, uint8_t rotation, sticky::Point& native);
bool touchPoint(uint16_t x, uint16_t y, uint16_t maxX, uint16_t maxY,
                uint8_t rotation, sticky::Point& logical);
sticky::Area difference(const uint8_t* a, const uint8_t* b);
sticky::Area transferArea(sticky::Area dirty);
}
