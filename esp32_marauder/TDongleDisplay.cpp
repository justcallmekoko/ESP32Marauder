#include "TDongleDisplay.h"

#ifdef HAS_T_DONGLE_DISPLAY

#include "WiFiScan.h"

namespace {
constexpr uint32_t kRefreshMs = 500;
constexpr uint8_t kRowHeight = 13;
}

void TDongleDisplay::begin() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("MARAUDER CLI", 2, 1);
  tft.drawFastHLine(0, 11, tft.width(), TFT_DARKGREY);
}

void TDongleDisplay::drawValue(uint8_t row, const char* label, int value, uint16_t color) {
  const int y = 14 + (row * kRowHeight);
  tft.fillRect(0, y, tft.width(), kRowHeight, TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(label, 2, y + 2);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawRightString(String(value), tft.width() - 2, y + 2);
}

const char* TDongleDisplay::modeLabel(uint8_t mode) const {
  switch (mode) {
    case WIFI_SCAN_OFF: return "IDLE";
    case WIFI_SCAN_AP: return "WIFI AP";
    case WIFI_SCAN_STATION: return "WIFI STA";
    case WIFI_SCAN_AP_STA: return "AP+STA";
    case WIFI_SCAN_ALL: return "WIFI ALL";
    case WIFI_SCAN_WAR_DRIVE: return "WARDRIVE";
    case BT_SCAN_ALL: return "BLE ALL";
    case BT_SCAN_WAR_DRIVE:
    case BT_SCAN_WAR_DRIVE_CONT: return "BLE DRIVE";
    default: return "ACTIVE";
  }
}

void TDongleDisplay::update(uint32_t now, const WiFiScan& scan) {
  if (now - last_update < kRefreshMs) return;
  last_update = now;

  const int ap_count = static_cast<int>(scan.retainedAccessPointCount());
  const int station_count = static_cast<int>(scan.retainedStationCount());
  const int ble_count = static_cast<int>(scan.retainedBleDeviceCount());

  if (ap_count != last_ap_count) drawValue(0, "WiFi AP", ap_count, TFT_GREEN);
  if (station_count != last_station_count) drawValue(1, "Stations", station_count, TFT_CYAN);
  if (ble_count != last_ble_count) drawValue(2, "BLE", ble_count, TFT_MAGENTA);
  if (scan.set_channel != last_channel) drawValue(3, "Channel", scan.set_channel, TFT_YELLOW);

  if (scan.currentScanMode != last_mode) {
    const int y = 14 + (4 * kRowHeight);
    tft.fillRect(0, y, tft.width(), kRowHeight, TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Mode", 2, y + 2);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawRightString(modeLabel(scan.currentScanMode), tft.width() - 2, y + 2);
  }

  last_ap_count = ap_count;
  last_station_count = station_count;
  last_ble_count = ble_count;
  last_channel = scan.set_channel;
  last_mode = scan.currentScanMode;
}

#endif
