// BiscuitLink -- clean-room codec for the Biscuit Manager BLE line protocol.
// See BiscuitLink.h for the contract and licence.
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 slickrick773

#include "BiscuitLink.h"

#include <string.h>
#include <stdio.h>

static void bl_copy_token(char* dst, size_t dstsz, const char* src, size_t n) {
  if (dstsz == 0) return;
  if (n >= dstsz) n = dstsz - 1;
  memcpy(dst, src, n);
  dst[n] = '\0';
}

BiscuitCmd biscuitParseLine(const char* line, size_t len) {
  BiscuitCmd r;
  r.type    = BISCUIT_CMD_NONE;
  r.name[0] = '\0';
  r.band    = 0;
  if (!line) return r;

  // Trim trailing CR/LF/space/tab, then leading space/tab.
  while (len && (line[len - 1] == '\r' || line[len - 1] == '\n' ||
                 line[len - 1] == ' '  || line[len - 1] == '\t')) {
    len--;
  }
  size_t s = 0;
  while (s < len && (line[s] == ' ' || line[s] == '\t')) s++;
  const char* p = line + s;
  size_t      n = len - s;

  // The app answers our TIME:REQUEST: with "TIME:SYNC:<iso8601>". We only need
  // to know it arrived (to stop retrying); the value itself is unused.
  if (n >= 10 && strncmp(p, "TIME:SYNC:", 10) == 0) {
    r.type = BISCUIT_CMD_TIME_SYNC;
    return r;
  }

  // Everything actionable is "CMD:<command>[:params]".
  if (n < 4 || strncmp(p, "CMD:", 4) != 0) return r;   // unknown type: drop
  p += 4;
  n -= 4;

  size_t cl = 0;                       // command token runs up to the next ':'
  while (cl < n && p[cl] != ':') cl++;
  bl_copy_token(r.name, sizeof(r.name), p, cl);

  if (strcmp(r.name, "stopscan") == 0) {
    r.type = BISCUIT_CMD_STOPSCAN;
    return r;
  }
  if (strcmp(r.name, "wardriveall") == 0 || strcmp(r.name, "scanap") == 0) {
    r.type = BISCUIT_CMD_WARDRIVE;
    r.band = 0;
    return r;
  }
  if (strcmp(r.name, "wardrive") == 0) {
    r.type = BISCUIT_CMD_WARDRIVE;
    r.band = 1;                        // 2.4 GHz only
    return r;
  }
  if (strcmp(r.name, "wardrive5g") == 0) {
    r.type = BISCUIT_CMD_WARDRIVE;
    r.band = 2;                        // 5 GHz only
    return r;
  }

  r.type = BISCUIT_CMD_UNSUPPORTED;    // a real command, just not one we do
  return r;
}

size_t biscuitFormatAp(char* out, size_t outsz,
                       const char* ssid, const char* bssid_upper,
                       int channel, int rssi, const char* security) {
  if (!out || outsz == 0) return 0;
  if (rssi < -128) rssi = -128;
  if (rssi >  127) rssi =  127;

  // Flatten the SSID so a comma or newline in it can't break the line. 802.11
  // caps the SSID at 32 octets; 64 leaves margin.
  char sbuf[64];
  size_t j = 0;
  if (ssid) {
    for (size_t i = 0; ssid[i] != '\0' && j < sizeof(sbuf) - 1; i++) {
      char c = ssid[i];
      sbuf[j++] = (c == ',' || c == '\r' || c == '\n') ? '_' : c;
    }
  }
  sbuf[j] = '\0';

  int w = snprintf(out, outsz, "DATA:AP:%s,%s,%d,%d,%s",
                   sbuf, bssid_upper ? bssid_upper : "",
                   channel, rssi, security ? security : "");
  if (w < 0) { out[0] = '\0'; return 0; }
  return ((size_t)w >= outsz) ? outsz - 1 : (size_t)w;
}

size_t biscuitFormatStatus(char* out, size_t outsz, int status, int battery) {
  if (!out || outsz == 0) return 0;
  int w = snprintf(out, outsz, "{\"status\":%d,\"battery\":%d}", status, battery);
  if (w < 0) { out[0] = '\0'; return 0; }
  return ((size_t)w >= outsz) ? outsz - 1 : (size_t)w;
}

int biscuitBandAllows(uint8_t band, int channel) {
  if (band == 1) return channel <= 14;   // 2.4 GHz
  if (band == 2) return channel >= 36;    // 5 GHz
  return 1;                               // all bands
}
