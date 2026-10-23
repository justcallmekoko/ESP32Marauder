#include "CensorText.h"

#include <ctype.h>
#include <string.h>

namespace CensorText {
bool looksLikeMac(const char* value) {
  if (!value || strlen(value) != 17) return false;
  for (size_t i = 0; i < 17; ++i) {
    if ((i + 1) % 3 == 0) {
      if (value[i] != ':' && value[i] != '-') return false;
    } else if (!isxdigit(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  return true;
}

void name(char* value) {
  if (!value) return;
  const size_t length = strlen(value);
  for (size_t i = 3; i < length; ++i) value[i] = 'X';
}

void mac(char* value) {
  if (!value) return;
  const size_t length = strlen(value);
  size_t hex_digits = 0;
  for (size_t i = 0; i < length; ++i) {
    if (!isxdigit(static_cast<unsigned char>(value[i]))) continue;
    if (hex_digits >= 4) value[i] = 'X';
    ++hex_digits;
  }
}

void identifier(char* value) {
  if (looksLikeMac(value)) mac(value);
  else name(value);
}
}
