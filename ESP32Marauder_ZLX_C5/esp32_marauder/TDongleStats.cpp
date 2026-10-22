#include "TDongleStats.h"

const char* TDongleStats::modeLabel(uint8_t mode) {
  switch (mode) {
    case 0: return "IDLE";
    case 2: return "WIFI AP";
    case 26: return "WIFI STA";
    case 49: return "AP+STA";
    case 6: return "WIFI ALL";
    case 32: return "WARDRIVE";
    case 10: return "BLE ALL";
    case 34:
    case 35: return "BLE DRIVE";
    default: return "ACTIVE";
  }
}
