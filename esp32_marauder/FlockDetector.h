#pragma once

#include <stddef.h>
#include <stdint.h>

bool flockProbeBodyMatches(const uint8_t* body, size_t body_len);
bool flockProbeRequestMatches(const uint8_t* frame, size_t frame_len);

