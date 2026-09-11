#include "TDongleDisplay.h"

#ifdef HAS_T_DONGLE_DISPLAY

#include "WiFiScan.h"
#include "TDongleStats.h"
#include <SPI.h>

namespace {
constexpr uint32_t kRefreshMs = 500;
constexpr uint8_t kRowHeight = 13;
constexpr uint8_t kBacklightPin = 0;
}

void TDongleDisplay::begin() {
  pinMode(kBacklightPin, OUTPUT);
  digitalWrite(kBacklightPin, HIGH);
  delay(500);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("MARAUDER CLI", 2, 1);
  tft.drawFastHLine(0, 11, tft.width(), TFT_DARKGREY);
  digitalWrite(kBacklightPin, LOW);
}

void TDongleDisplay::drawValue(uint8_t row, const char* label, int value, uint16_t color) {
  const int y = 14 + (row * kRowHeight);
  tft.fillRect(0, y, tft.width(), kRowHeight, TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString(label, 2, y + 2);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawRightString(String(value), tft.width() - 2, y + 2, 1);
}

bool TDongleDisplay::update(uint32_t now, const WiFiScan& scan) {
  if (now - last_update < kRefreshMs) return false;
  last_update = now;
  bool drew = false;

  const int ap_count = static_cast<int>(scan.retainedAccessPointCount());
  const int station_count = static_cast<int>(scan.retainedStationCount());
  const int ble_count = static_cast<int>(scan.retainedBleDeviceCount());
  const bool ap_changed = ap_count != last_ap_count;
  const bool station_changed = station_count != last_station_count;
  const bool ble_changed = ble_count != last_ble_count;
  const bool channel_changed = scan.set_channel != last_channel;
  const bool mode_changed = scan.currentScanMode != last_mode;

  if (!(ap_changed || station_changed || ble_changed || channel_changed || mode_changed)) {
    return false;
  }

  // LED updates bit-bang the TFT's MOSI/MISO pins. Reclaim the shared bus only
  // when a redraw is required, then let the caller write the LED state last.
  SPI.end();
  SPI.begin(T_DONGLE_SPI_SCLK_PIN, T_DONGLE_SPI_MISO_PIN,
            T_DONGLE_SPI_MOSI_PIN, -1);

  if (ap_changed) {
    drawValue(0, "WiFi AP", ap_count, TFT_GREEN);
    drew = true;
  }
  if (station_changed) {
    drawValue(1, "Stations", station_count, TFT_CYAN);
    drew = true;
  }
  if (ble_changed) {
    drawValue(2, "BLE", ble_count, TFT_MAGENTA);
    drew = true;
  }
  if (channel_changed) {
    drawValue(3, "Channel", scan.set_channel, TFT_YELLOW);
    drew = true;
  }

  if (mode_changed) {
    const int y = 14 + (4 * kRowHeight);
    tft.fillRect(0, y, tft.width(), kRowHeight, TFT_BLACK);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.drawString("Mode", 2, y + 2);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawRightString(TDongleStats::modeLabel(scan.currentScanMode), tft.width() - 2, y + 2, 1);
    drew = true;
  }

  last_ap_count = ap_count;
  last_station_count = station_count;
  last_ble_count = ble_count;
  last_channel = scan.set_channel;
  last_mode = scan.currentScanMode;
  return drew;
}

#endif
