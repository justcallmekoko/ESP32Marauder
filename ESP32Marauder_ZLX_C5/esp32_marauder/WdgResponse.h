#pragma once

#include <stddef.h>

// Extract a short, display-safe error reason from an HTTP response returned by
// WDG Wars. Returns false only when the response contains no usable reason.
bool extractWdgErrorReason(const char* response, char* output, size_t outputSize);
