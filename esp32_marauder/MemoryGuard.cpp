#include "MemoryGuard.h"

#ifdef ARDUINO
#include "esp_heap_caps.h"
#endif

namespace marauder {

MemoryGuardState::MemoryGuardState(MemoryGuardThresholds thresholds)
    : thresholds_(thresholds) {}

bool MemoryGuardState::allow(size_t freeBytes, size_t largestBlockBytes,
                             size_t requestedBytes) {
  if (paused_) {
    if (freeBytes < thresholds_.resumeFreeBytes ||
        largestBlockBytes < thresholds_.resumeLargestBlockBytes)
      return false;
    paused_ = false;
  }

  const bool freePressure =
      freeBytes <= thresholds_.pauseFreeBytes + requestedBytes;
  const bool fragmentationPressure =
      largestBlockBytes <= thresholds_.pauseLargestBlockBytes + requestedBytes;
  if (freePressure || fragmentationPressure) {
    paused_ = true;
    return false;
  }
  return true;
}

bool MemoryGuardState::paused() const { return paused_; }

QueueBackpressureState::QueueBackpressureState(size_t limit)
    : limit_(limit), resume_limit_(limit / 2) {}

bool QueueBackpressureState::allow(size_t currentSize) {
  if (paused_) {
    if (currentSize > resume_limit_) return false;
    paused_ = false;
  }
  if (currentSize >= limit_) {
    paused_ = true;
    return false;
  }
  return true;
}

bool QueueBackpressureState::paused() const { return paused_; }

#ifdef ARDUINO
RuntimeMemoryGuard& RuntimeMemoryGuard::instance() {
  static RuntimeMemoryGuard guard;
  return guard;
}

bool RuntimeMemoryGuard::allow(size_t requestedBytes) {
  const uint32_t capabilities = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  const size_t freeBytes = heap_caps_get_free_size(capabilities);
  const size_t largestBlockBytes =
      heap_caps_get_largest_free_block(capabilities);
  portENTER_CRITICAL(&mux_);
  const bool allowed = state_.allow(freeBytes, largestBlockBytes, requestedBytes);
  if (!allowed && rejected_allocations_ != UINT32_MAX) ++rejected_allocations_;
  portEXIT_CRITICAL(&mux_);
  return allowed;
}

bool RuntimeMemoryGuard::paused() const {
  portENTER_CRITICAL(&mux_);
  const bool isPaused = state_.paused();
  portEXIT_CRITICAL(&mux_);
  return isPaused;
}

uint32_t RuntimeMemoryGuard::rejectedAllocations() const {
  portENTER_CRITICAL(&mux_);
  const uint32_t rejected = rejected_allocations_;
  portEXIT_CRITICAL(&mux_);
  return rejected;
}
#endif

}  // namespace marauder
