#include "configs.h"
#ifdef MARAUDER_EPAPER_UI
#include "Display.h"
#include "ReconMission.h"
#include "WiFiScan.h"
#include <Fonts/FreeSansBold18pt7b.h>

extern Display display_obj;

#ifdef MARAUDER_RETERMINAL_STICKY
void StickyDisplay::showPowerOff() {
  if (!initialized_) return;
  uint32_t started = millis();
  while (busy_.load(std::memory_order_acquire)) {
    if (uint32_t(millis() - started) >= 15000) {
      Serial.println("Sticky: display timeout during shutdown");
      return;
    }
    delay(5);
  }
  diagnostic_ = false;
  setRotation(0);
  fillScreen(TFT_BLACK);
  drawHeading("Powered off");
  setTextColor(TFT_WHITE, TFT_BLACK);
  drawCentreString("Press power to start", 240, 368);
  drawCentreString("Release the button", 240, 420);
  requestClean();
  display(true);
  started = millis();
  while (busy_.load(std::memory_order_acquire)) {
    if (uint32_t(millis() - started) >= 15000) return;
    delay(5);
  }
  // SSD1677 vendor sleep command, after the final refresh has completed.
  reg(0x10, 0x03);
  delay(100);
}

#endif

void TFT_eSPI::drawHeading(String title) {
  title.trim();
  // The menu calls this feature a detector; keep the activity heading concise.
  if (title == "Deauthentication Sniffer") title = "Deauth detector";
  fillRect(0, STATUS_BAR_WIDTH, width(), 104, TFT_BLACK);
  setTextWrap(false);
  setTextColor(TFT_WHITE, TFT_BLACK);
  setTextSize(1);
  setFreeFont(&FreeSansBold18pt7b);
  const int available = width() - 48;
  if (textWidth(title) <= available) {
    drawString(title, 24, 83);
  } else {
    // Wrap long titles without shrinking them back to the LCD's tiny type.
    int split = title.lastIndexOf(' ');
    while (split > 0 && textWidth(title.substring(0, split)) > available)
      split = title.lastIndexOf(' ', split - 1);
    if (split <= 0) {
      split = title.length();
      while (split > 1 && textWidth(title.substring(0, split)) > available) --split;
    }
    String second = title.substring(split);
    second.trim();
    if (textWidth(second) > available) {
      while (second.length() && textWidth(second + "...") > available)
        second.remove(second.length() - 1);
      second += "...";
    }
    drawString(title.substring(0, split), 24, 64);
    drawString(second, 24, 106);
  }
  drawFastHLine(24, 123, width() - 48, TFT_WHITE);
  setFreeFont(nullptr);
  setTextSize(2);
}

void Display::drawStickyScanButton(uint8_t index, int16_t x, const char* label) {
  x += (tft.width() - 480) / 2;
  TOP_FIXED_AREA_2 = TOP_FIXED_AREA;
  tft.setFreeFont(nullptr);
  key[index].initButton(&tft, x, 160, 100, 56,
                       TFT_WHITE, TFT_BLACK, TFT_WHITE,
                       const_cast<char*>(label), 2);
  key[index].setLabelDatum(0, 0, MC_DATUM);
  key[index].drawButton();
  tft.drawRect(x - 50, 132, 100, 56, TFT_WHITE);
}

void ReconMission::previewStickyDashboard() {
  if (!running) drawStickyDashboard(0, 0, 0);
}

void ReconMission::drawStickyDashboard(uint32_t seconds, uint8_t churn_in, uint8_t churn_out) {
  auto& screen = display_obj.tft;
  const bool wifi = active_mode == ReconMode::WIFI_RECON;
  screen.fillRect(0, STATUS_BAR_WIDTH, screen.width(), screen.height() - STATUS_BAR_WIDTH, TFT_BLACK);
  screen.drawHeading(wifi ? "Wi-Fi recon" : "Bluetooth recon");
  char line[64];
  snprintf(line, sizeof(line), "%s  %lu:%02lu", running ? "RECORDING" : "PREVIEW",
           static_cast<unsigned long>(seconds / 60), static_cast<unsigned long>(seconds % 60));
  screen.drawString(line, 24, 136);

  const auto metric = [&](const char* label, uint32_t value, int x, int y) {
    screen.setTextSize(2);
    screen.drawString(label, x, y);
    screen.setTextSize(3);
    screen.drawString(String(value), x, y + 22);
  };
  if (wifi) {
    metric("ACCESS POINTS", ap_count, 24, 174);
    metric("STATIONS", station_count, screen.width() / 2 + 24, 174);
    metric("PROBES", probe_count, 24, 236);
    metric("UPDATES", repeat_count, screen.width() / 2 + 24, 236);
    metric("DEAUTH EVENTS", deauth_count, 24, 298);
    screen.setTextSize(2);
    screen.drawString("BAND SHARE", screen.width() / 2 + 24, 298);
    const uint32_t total = band_24_observations + band_5_observations;
    const uint8_t percent = total ? (uint64_t(band_24_observations) * 100) / total : 0;
    snprintf(line, sizeof(line), "2G %u%% 5G %u%%", percent, total ? 100 - percent : 0);
    screen.drawString(line, screen.width() / 2 + 24, 324);
  } else {
    metric("BLE DEVICES", ble_count, 24, 174);
    metric("UPDATES", repeat_count, screen.width() / 2 + 24, 174);
    metric("IBEACONS", ibeacon_count, 24, 236);
  }

  screen.setTextSize(2);
  screen.drawFastHLine(24, 360, screen.width() - 48, TFT_WHITE);
  snprintf(line, sizeof(line), "CHURN  +%u / -%u", churn_in, churn_out);
  screen.drawString(line, 24, 374);
  // Separate positive/negative traces so monochrome does not merge the series.
  screen.drawString("+", 24, 408);
  screen.drawString("-", 24, 440);
  screen.drawFastHLine(48, 435, screen.width() - 72, TFT_WHITE);
  uint8_t peak = 1;
  for (uint8_t i = 0; i < churn_history_count; ++i) {
    if (churn_in_history[i] > peak) peak = churn_in_history[i];
    if (churn_out_history[i] > peak) peak = churn_out_history[i];
  }
  const int first = screen.width() - 24 - churn_history_count * 6;
  for (uint8_t i = 0; i < churn_history_count; ++i) {
    const int incoming = (uint16_t(churn_in_history[i]) * 30) / peak;
    const int outgoing = (uint16_t(churn_out_history[i]) * 30) / peak;
    if (incoming) screen.fillRect(first + i * 6, 435 - incoming, 3, incoming, TFT_WHITE);
    if (outgoing) screen.fillRect(first + i * 6, 436, 3, outgoing, TFT_WHITE);
  }

  screen.drawFastHLine(24, 480, screen.width() - 48, TFT_WHITE);
  screen.drawString(wifi ? "NEW ASSOCIATIONS" : "DEVICE TYPES", 24, 496);
  bool any = false;
  for (uint8_t row = 0; row < 3; ++row) {
    if (wifi) {
      const auto& relation = ui_relationships[(ui_relationship_head + row) % 3];
      if (!relation.ap_name[0]) continue;
      snprintf(line, sizeof(line), "%02X:%02X:%02X > %.16s",
               relation.station[3], relation.station[4], relation.station[5], relation.ap_name);
    } else {
      const auto& event = ui_events[(ui_event_head + row + 1) % 4];
      if (event.type != 'b') continue;
      snprintf(line, sizeof(line), "%02X:%02X:%02X > %.12s",
               event.mac[3], event.mac[4], event.mac[5], event.label[0] ? event.label : "BLE");
    }
    screen.drawString(line, 24, 526 + row * 26);
    any = true;
  }
  if (!any) screen.drawString("Waiting for devices...", 24, 526);
  screen.drawFastHLine(24, 614, screen.width() - 48, TFT_WHITE);
  const auto& latest = ui_events[(ui_event_head + 3) % 4];
  if (latest.type == 'p') snprintf(line, sizeof(line), "PROBE %.12s", latest.label);
  else if (latest.type) snprintf(line, sizeof(line), "%c  %02X:%02X:%02X  RSSI %d",
                                latest.type, latest.mac[3], latest.mac[4], latest.mac[5], latest.rssi);
  else snprintf(line, sizeof(line), "No events yet");
  screen.drawString(line, 24, 632);

  if (wifi) {
    screen.drawString("CHANNEL ACTIVITY", 24, 670);
    uint16_t channel_peak = 1;
    for (uint8_t i = 0; i < MAX_CHANNEL; ++i)
      if (channel_activity[i] > channel_peak) channel_peak = channel_activity[i];
    const int slot = (screen.width() - 48) / MAX_CHANNEL;
    for (uint8_t i = 0; i < MAX_CHANNEL; ++i) {
      const int h = (uint32_t(channel_activity[i]) * 40) / channel_peak;
      const int x = 24 + i * slot;
      if (h) screen.fillRect(x + 4, 736 - h, slot - 8, h, TFT_WHITE);
      screen.drawCentreString(String(i + 1), x + slot / 2, 740);
    }
  }
  screen.drawFastHLine(24, screen.height() - 32, screen.width() - 48, TFT_WHITE);
  screen.drawCentreString(running ? "Tap screen to stop" : "Display preview - radio off", screen.width() / 2, screen.height() - 20);
}
#endif
