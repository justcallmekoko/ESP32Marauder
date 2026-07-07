#pragma once
#ifndef JsonSerial_h
#define JsonSerial_h

// ----------------------------------------------------------------------------
//  JsonSerial - machine-readable (JSON) command set for the serial CLI
//
//  Adds a small set of structured commands so any program driving the USB
//  serial port can parse device data reliably instead of scraping the
//  human-formatted CLI text. Every line of machine-readable output is prefixed
//  with "@J " so a host can cleanly separate it from the regular human-readable
//  CLI output the firmware already produces. These commands DO NOT touch the
//  scanning internals - they only read the data structures the firmware already
//  maintains (access_points, stations, ssids, ...).
//
//  Commands:
//    jsoninfo               -> one {"t":"info", ...} handshake line
//    jsonstatus             -> one {"t":"status", ...} line (poll friendly)
//    jsonlist <a|s|c|i|p|t> -> NDJSON: one object per line + a closing
//                              {"t":"end", ...} line
//    jsonmode <0|1>         -> toggle continuous streaming
// ----------------------------------------------------------------------------

#include "configs.h"
#include <Arduino.h>
#include <LinkedList.h>

// JSON protocol version reported by jsoninfo (kept in configs.h)
#ifndef MARAUDER_JSON_PROTO
  #define MARAUDER_JSON_PROTO 1
#endif

// Prefix that tags every machine-readable line.
#define JSON_LINE_PREFIX "@J "

// Command tokens
const char PROGMEM JSON_INFO_CMD[]   = "jsoninfo";
const char PROGMEM JSON_LIST_CMD[]   = "jsonlist";
const char PROGMEM JSON_STATUS_CMD[] = "jsonstatus";
const char PROGMEM JSON_MODE_CMD[]   = "jsonmode";

namespace JsonSerial {
  // Returns true if cmd_args[0] was a JSON command (and it was handled).
  // Returns false otherwise so the normal CLI dispatch can continue.
  bool handle(LinkedList<String>* cmd_args);

  // True once a host has requested JSON mode (via `jsoninfo` or `jsonmode 1`).
  // Continuous analyzer streaming is gated on this so a plain serial monitor is
  // never flooded with "@J" lines.
  bool jsonModeEnabled();

  // Analyzer streaming (gated by jsonModeEnabled). The host redraws the charts
  // itself from the streamed values.
  //
  // emitAnalyzerSample: one new rolling-graph sample per refresh (Channel
  //   Analyzer / BT Analyzer). Streaming one point (not the whole pixel array)
  //   keeps well under the serial bandwidth; the host maintains its own rolling
  //   buffer. mode = scan-mode constant; ch = current WiFi channel, or -1 when
  //   it is not channel-meaningful (BT analyzer); value = sample.
  void emitAnalyzerSample(uint8_t mode, int ch, int16_t value);

  // emitChannelActivity: the currently displayed page of the Channel Activity
  //   bar chart, as parallel arrays of real channel numbers (ch[]) and their
  //   counts (v[]), plus the 1-based page index. Emitting actual channel
  //   numbers means the host needs no out-of-band channel mapping.
  void emitChannelActivity(const int* channels, const uint8_t* values, int count, int page);
}

#endif
