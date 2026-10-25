#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef ARDUINO
#include "freertos/FreeRTOS.h"
// Needed so the threshold defaults below can react to per-board capability
// macros (specifically MARAUDER_LOW_INTERNAL_RAM). configs.h is #pragma
// once-guarded and Arduino-only, so this neither recurses nor affects the
// host unit-test build (which never defines ARDUINO and keeps the standard
// thresholds).
#include "configs.h"
#endif

namespace marauder {

// --- Runtime memory-guard thresholds (why these are board-tunable) ---------
//
// The guard pauses heap allocations when free internal SRAM gets low, to keep
// the WiFi/BLE stacks from OOM-crashing, and resumes once memory recovers.
// It is intentionally hysteretic: it stops at the lower "pause" level and does
// not restart until the higher "resume" level, so it cannot thrash on/off.
//
// The default floor (24 KB free / 40 KB resume) assumes a target with ample
// internal SRAM (ESP32 / S3 / WROOM). It does NOT fit every board: an
// ESP32-C5 that also drives a display (MARAUDER_WS_C5_28 / WS_C5_35) runs with
// only ~17 KB internal SRAM free at runtime (small C5 internal SRAM + display
// framebuffers + the full feature profile). On those boards the guard would
// drop below 24 KB immediately and latch paused forever (it can never reach
// the 40 KB resume level), so EVERY WiFi AP add was rejected
// ("[memguard] AP not added ... aps=0") and starting the SoftAP OOM-crashed
// inside the WiFi driver (ieee80211_hostap_attach).
//
// So the floor is made board-tunable: boards that are known to run with little
// internal SRAM define MARAUDER_LOW_INTERNAL_RAM (see configs.h) to select a
// lower, workable floor. Individual values can still be overridden per board
// via the MARAUDER_MEMGUARD_* macros. These low defaults are a starting point
// and should be tuned on hardware against the "[memguard] free=... largest=..."
// log: low enough to let the board operate, high enough to leave the WiFi
// driver headroom. Every other target, and the host test build, keep the
// original 24/40/8/12 KB values unchanged.
#if defined(MARAUDER_LOW_INTERNAL_RAM)
  // Low-internal-SRAM profile (e.g. C5 + display). Tune per the memguard log.
  #ifndef MARAUDER_MEMGUARD_PAUSE_FREE
    #define MARAUDER_MEMGUARD_PAUSE_FREE     (10 * 1024)  // pause below ~10 KB free
  #endif
  #ifndef MARAUDER_MEMGUARD_RESUME_FREE
    #define MARAUDER_MEMGUARD_RESUME_FREE    (14 * 1024)  // resume once >= ~14 KB free
  #endif
  #ifndef MARAUDER_MEMGUARD_PAUSE_LARGEST
    #define MARAUDER_MEMGUARD_PAUSE_LARGEST  (6 * 1024)   // pause if largest block < ~6 KB
  #endif
  #ifndef MARAUDER_MEMGUARD_RESUME_LARGEST
    #define MARAUDER_MEMGUARD_RESUME_LARGEST (8 * 1024)   // resume once largest block >= ~8 KB
  #endif
#else
  // Standard profile: original upstream thresholds, suited to boards with
  // ample internal SRAM (ESP32 / S3 / WROOM) and the host unit-test build.
  #define MARAUDER_MEMGUARD_PAUSE_FREE     (24 * 1024)
  #define MARAUDER_MEMGUARD_RESUME_FREE    (40 * 1024)
  #define MARAUDER_MEMGUARD_PAUSE_LARGEST  (8 * 1024)
  #define MARAUDER_MEMGUARD_RESUME_LARGEST (12 * 1024)
#endif

struct MemoryGuardThresholds {
  size_t pauseFreeBytes = MARAUDER_MEMGUARD_PAUSE_FREE;
  size_t resumeFreeBytes = MARAUDER_MEMGUARD_RESUME_FREE;
  size_t pauseLargestBlockBytes = MARAUDER_MEMGUARD_PAUSE_LARGEST;
  size_t resumeLargestBlockBytes = MARAUDER_MEMGUARD_RESUME_LARGEST;
};

class MemoryGuardState {
 public:
  explicit MemoryGuardState(
      MemoryGuardThresholds thresholds = MemoryGuardThresholds{});

  bool allow(size_t freeBytes, size_t largestBlockBytes,
             size_t requestedBytes = 0);
  bool paused() const;

 private:
  MemoryGuardThresholds thresholds_;
  bool paused_ = false;
};

class QueueBackpressureState {
 public:
  explicit QueueBackpressureState(size_t limit);
  bool allow(size_t currentSize);
  bool paused() const;

 private:
  size_t limit_;
  size_t resume_limit_;
  bool paused_ = false;
};

#ifdef ARDUINO
class RuntimeMemoryGuard {
 public:
  static RuntimeMemoryGuard& instance();

  bool allow(size_t requestedBytes = 0);
  bool paused() const;
  uint32_t rejectedAllocations() const;

 private:
  MemoryGuardState state_;
  uint32_t rejected_allocations_ = 0;
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

template <typename List, typename Value>
bool memoryGuardedAdd(List* list, const Value& value,
                      size_t extraReserveBytes = 32) {
  if (list == nullptr) return false;
  const size_t requested = sizeof(Value) + sizeof(void*) + extraReserveBytes;
  if (!RuntimeMemoryGuard::instance().allow(requested)) return false;
  return list->add(value);
}
#endif

}  // namespace marauder
