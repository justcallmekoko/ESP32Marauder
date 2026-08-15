#include "DisplayLine.h"

#include <string.h>

void fitDisplayLine(char* output, size_t output_size, const char* input) {
  if ((output == nullptr) || (output_size == 0))
    return;

  const size_t width = output_size - 1;
  const size_t input_length = input == nullptr ? 0 : strlen(input);
  const size_t copy_length = input_length < width ? input_length : width;

  if (copy_length > 0)
    memcpy(output, input, copy_length);
  if (copy_length < width)
    memset(output + copy_length, ' ', width - copy_length);

  output[width] = '\0';
}

uint8_t resolveDisplayTextSize(bool small_print, uint8_t requested_size) {
  if (small_print)
    return 1;

  return requested_size > 0 ? requested_size : 1;
}
