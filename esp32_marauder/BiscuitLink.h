// BiscuitLink -- clean-room codec for the Biscuit Manager BLE line protocol.
//
// Transport-agnostic: pure C, no Arduino / NimBLE / Marauder dependencies, so
// it drops into any project that needs to speak the Biscuit "wardriving node"
// surface. It only parses inbound command lines and formats outbound lines;
// the BLE plumbing (GATT service, characteristics, advertising) lives in
// whatever firmware links this in.
//
// Implemented from the public Biscuit-Wiki GATT Reference (the wire format is
// an interface specification -- UUIDs, command tokens and line layouts -- not
// copied source). Only the wardriving commands are recognized; anything else
// is reported so the caller can answer "unsupported".
//
// SPDX-License-Identifier: MIT
// Copyright (c) 2026 slickrick773
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BiscuitLink_h
#define BiscuitLink_h

#include <stddef.h>
#include <stdint.h>

// --- GATT identity (Biscuit-Wiki GATT Reference) ---------------------------
// Biscuit Manager discovers the node by this service UUID + the local name
// "Biscuit", reads firmware/model from the standard Device Information Service
// (0x180A), writes commands to CMD and subscribes to RSP for the replies.
#define BISCUIT_SVC_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BISCUIT_CMD_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // write  (phone->node)
#define BISCUIT_RSP_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"  // notify (node->phone)
#define BISCUIT_STA_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"  // read   (status JSON)
#define BISCUIT_SET_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26ab"  // read/write (settings)
#define BISCUIT_ADV_NAME  "Biscuit"

typedef enum {
  BISCUIT_CMD_NONE = 0,     // not a line we handle -> drop silently
  BISCUIT_CMD_TIME_SYNC,    // "TIME:SYNC:<iso8601>" -> ack, value ignored
  BISCUIT_CMD_STOPSCAN,     // "CMD:stopscan:"
  BISCUIT_CMD_WARDRIVE,     // "CMD:wardrive{,all,5g}:" / "CMD:scanap:" (see band)
  BISCUIT_CMD_UNSUPPORTED   // "CMD:<other>:" -> answer RSP:<name>:ERROR:unsupported
} BiscuitCmdType;

typedef struct {
  BiscuitCmdType type;
  char           name[24];  // the raw command token, for the "RSP:<name>:..." echo
  uint8_t        band;      // wardrive only: 0 = all, 1 = 2.4 GHz, 2 = 5 GHz
} BiscuitCmd;

#ifdef __cplusplus
extern "C" {
#endif

// Parse one inbound Biscuit line. A trailing CRLF/space is tolerated; the
// caller is expected to have already split the write on '\n'.
BiscuitCmd biscuitParseLine(const char* line, size_t len);

// Format "DATA:AP:SSID,BSSID,Channel,RSSI,Security" (no trailing newline).
// The SSID is flattened (',' CR LF -> '_') so it can never break the
// comma/line-delimited protocol. 'security' is the node's auth token, e.g.
// "[WPA2_PSK]". Returns the written length (excluding the NUL), clamped so the
// buffer is never overrun.
size_t biscuitFormatAp(char* out, size_t outsz,
                       const char* ssid, const char* bssid_upper,
                       int channel, int rssi, const char* security);

// Format the Status characteristic snapshot: {"status":S,"battery":B}.
// status: 0 booting, 1 ready, 2 busy, 6 done. battery: 0..100, or -1 unknown.
size_t biscuitFormatStatus(char* out, size_t outsz, int status, int battery);

// Band filter for a wardrive session: does an AP on 'channel' pass 'band'?
// band 0 = all, 1 = 2.4 GHz (ch <= 14), 2 = 5 GHz (ch >= 36).
int biscuitBandAllows(uint8_t band, int channel);

#ifdef __cplusplus
}
#endif

#endif // BiscuitLink_h
