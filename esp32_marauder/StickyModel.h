#pragma once
#include <stddef.h>
#include <stdint.h>

namespace sticky {
constexpr uint16_t kPanelWidth = 800, kPanelHeight = 480;
constexpr size_t kStride = kPanelWidth / 8;
constexpr size_t kFrameBytes = kStride * kPanelHeight;
struct Point { uint16_t x, y; };
struct Area {
  uint16_t x = 0, y = 0, w = 0, h = 0;
  bool empty() const { return !w || !h; }
};
// Raw GT911 coordinates are stretched across the physical portrait axes.
// See Seeed's Sticky_peripheral_demo/main/demo_touch.cpp, not a generic
// GT911 rotation example. Accept the controller's inclusive upper endpoint.
bool touchPoint(uint16_t rawX, uint16_t rawY, uint16_t maxX,
                uint16_t maxY, uint8_t rotation, Point& logical);
bool panelPoint(int16_t x, int16_t y, uint8_t rotation, Point& native);
Area difference(const uint8_t* a, const uint8_t* b);
int8_t menuZone(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

struct DebouncedButton {
  bool down = false, raw = false, initialized = false;
  uint32_t changedAt = 0;
  bool update(bool pressed, uint32_t now);
};
enum class PowerEvent : uint8_t { None, Select, Shutdown };
struct PowerButton {
  DebouncedButton button;
  bool armed = false, held = false;
  uint32_t pressedAt = 0;
  PowerEvent update(bool pressed, uint32_t now);
};

// A not-ready status is NOT a finger release. Only a ready zero-contact
// report releases normally; persistent transport failure is fail-safe.
struct TouchState {
  bool down = false;
  Point point{};
  uint32_t lastGood = 0;
  bool apply(bool transportOk, uint8_t status, bool validPoint,
             Point next, uint32_t now);
};
}
