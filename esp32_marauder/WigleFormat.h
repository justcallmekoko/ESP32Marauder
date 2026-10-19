#pragma once

#include <stdint.h>

static const char WIGLE_CSV_COLUMNS[] =
  "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,"
  "CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,"
  "RCOIs,MfgrId,Type";

constexpr uint16_t wigleFrequencyMHz(uint16_t channel) {
  if (channel >= 1 && channel <= 13)
    return 2407 + (channel * 5);
  if (channel == 14)
    return 2484;
  if (channel >= 32 && channel <= 177)
    return 5000 + (channel * 5);
  return 0;
}
