#pragma once

#include <stddef.h>
#include <stdint.h>

bool setBeaconFrameChannel(
  uint8_t* frame,
  size_t frame_size,
  size_t ssid_length,
  uint8_t channel
);
