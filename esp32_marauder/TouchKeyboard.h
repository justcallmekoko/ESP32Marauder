#pragma once

#include "configs.h"

#ifdef HAS_TOUCH

#include "Display.h"
#include <stddef.h>
#include <stdint.h>

enum KeyboardResult {
    KB_NONE = 0,
    KB_CHANGED,
    KB_DONE
};

/**
 * Blocking keyboard input.
 *
 * @param buffer   Caller-provided char buffer to hold the text. Will be null-terminated.
 * @param bufLen   Total size of the buffer (including space for '\0').
 * @param title    Optional title displayed above the text box (can be nullptr).
 *
 * @return true if user pressed OK, false if aborted (currently: you’d have to
 *         add your own external abort condition – e.g. button press).
 */
bool keyboardInput(char *buffer, size_t bufLen, const char *title = nullptr);

#endif