#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef ARDUINO
#include "freertos/FreeRTOS.h"
#endif

namespace marauder {

struct MemoryGuardThresholds {
  size_t pauseFreeBytes = 24 * 1024;
  size_t resumeFreeBytes = 40 * 1024;
  size_t pauseLargestBlockBytes = 8 * 1024;
  size_t resumeLargestBlockBytes = 12 * 1024;
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
