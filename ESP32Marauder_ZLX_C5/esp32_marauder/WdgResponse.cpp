#include "WdgResponse.h"

#include <ctype.h>
#include <string.h>

namespace {
  const char* skipWhitespace(const char* value) {
    while (value && *value && isspace(static_cast<unsigned char>(*value)))
      value++;
    return value;
  }

  bool containsIgnoreCase(const char* text, const char* needle) {
    if (!text || !needle || !*needle)
      return false;

    for (; *text; text++) {
      const char* current = text;
      const char* match = needle;
      while (*current && *match &&
             tolower(static_cast<unsigned char>(*current)) ==
               tolower(static_cast<unsigned char>(*match))) {
        current++;
        match++;
      }
      if (!*match)
        return true;
    }
    return false;
  }

  bool copyReason(const char* start, const char* end, char* output, size_t outputSize) {
    if (!start || !end || !output || outputSize < 2 || end <= start)
      return false;

    size_t written = 0;
    bool pendingSpace = false;
    while (start < end && written < outputSize - 1) {
      unsigned char current = static_cast<unsigned char>(*start++);
      if (current == '\\' && start < end) {
        current = static_cast<unsigned char>(*start++);
        if (current == 'n' || current == 'r' || current == 't')
          current = ' ';
      }

      if (isspace(current)) {
        pendingSpace = written > 0;
        continue;
      }
      if (!isprint(current))
        continue;
      if (pendingSpace && written < outputSize - 1)
        output[written++] = ' ';
      pendingSpace = false;
      output[written++] = static_cast<char>(current);
    }

    while (written > 0 && (output[written - 1] == ' ' || output[written - 1] == '"'))
      written--;
    output[written] = '\0';
    return written > 0;
  }

  bool extractJsonValue(const char* body, const char* key, char* output, size_t outputSize) {
    char quotedKey[16];
    const size_t keyLength = strlen(key);
    if (keyLength + 3 > sizeof(quotedKey))
      return false;
    quotedKey[0] = '"';
    memcpy(quotedKey + 1, key, keyLength);
    quotedKey[keyLength + 1] = '"';
    quotedKey[keyLength + 2] = '\0';

    const char* value = strstr(body, quotedKey);
    if (!value)
      return false;
    value = strchr(value + keyLength + 2, ':');
    if (!value)
      return false;
    value = skipWhitespace(value + 1);

    if (*value == '"') {
      const char* end = value + 1;
      bool escaped = false;
      while (*end) {
        if (*end == '"' && !escaped)
          break;
        escaped = (*end == '\\' && !escaped);
        if (*end != '\\')
          escaped = false;
        end++;
      }
      return copyReason(value + 1, end, output, outputSize);
    }

    const char* end = value;
    while (*end && *end != ',' && *end != '}' && *end != '\r' && *end != '\n')
      end++;
    return copyReason(value, end, output, outputSize);
  }
}

bool extractWdgErrorReason(const char* response, char* output, size_t outputSize) {
  if (!output || outputSize == 0)
    return false;
  output[0] = '\0';
  if (!response || !*response)
    return false;

  const char* body = strstr(response, "\r\n\r\n");
  body = body ? body + 4 : response;
  body = skipWhitespace(body);

  static const char* keys[] = {"detail", "message", "error", "reason"};
  for (const char* key : keys) {
    if (extractJsonValue(body, key, output, outputSize)) {
      if (containsIgnoreCase(output, "duplicate")) {
        copyReason("Duplicate upload", "Duplicate upload" + 16, output, outputSize);
      } else if (containsIgnoreCase(output, "already") &&
                 containsIgnoreCase(output, "upload")) {
        copyReason("Already uploaded", "Already uploaded" + 16, output, outputSize);
      }
      return true;
    }
  }

  if (*body && *body != '{' && *body != '<') {
    const char* end = body;
    while (*end && *end != '\r' && *end != '\n')
      end++;
    if (copyReason(body, end, output, outputSize))
      return true;
  }

  const char* status = strstr(response, "HTTP/");
  if (status) {
    status = strchr(status, ' ');
    if (status) {
      status = skipWhitespace(status);
      const char* end = strstr(status, "\r\n");
      if (!end)
        end = status + strlen(status);
      return copyReason(status, end, output, outputSize);
    }
  }

  return false;
}
