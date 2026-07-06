#include "CSI.h"
#include "WiFiScan.h"

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif

#ifdef HAS_BUTTONS
  #include "Switches.h"
  #if (C_BTN >= 0)
    extern Switches c_btn;
  #endif
  #if (D_BTN >= 0)
    extern Switches d_btn;
  #endif
#endif

extern WiFiScan wifi_scan_obj;

// Screen layout constants (portrait 135x240)
#define CSI_HDR_H   14
#define CSI_GRAPH_H 140
#define CSI_STAT_H  50
#define CSI_HINT_H  14
#define CSI_GRAPH_Y CSI_HDR_H
#define CSI_STAT_Y  (CSI_HDR_H + CSI_GRAPH_H)
#define CSI_HINT_Y  (CSI_HDR_H + CSI_GRAPH_H + CSI_STAT_H)

CsiModule::CsiModule() {
  streamState = CSI_STREAM_IDLE;
  packetCount = 0;
  lastPacketTime = 0;
  lastDisplayUpdate = 0;
  lastGraphUpdate = 0;
  lastMotionUpdate = 0;
  wifiOwned = false;
  operatingChannel = 1;
  peakRssi = -128;
  returnToMenu = false;
  viewMode = 0;
  personDetected = false;
  motionScore = 0;
  totalEnergy = 0;
  numSub = 0;
}

void CsiModule::setup() {
  streamState = CSI_STREAM_IDLE;
  packetCount = 0;
}

void CsiModule::takeWiFi() {
  if (wifiOwned) return;

  if (wifi_scan_obj.wifi_initialized) {
    if (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) {
      wifi_scan_obj.StopScan(wifi_scan_obj.currentScanMode);
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    wifi_scan_obj.wifi_initialized = false;
    delay(100);
  }

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.csi_enable = true;

  esp_err_t ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK) {
    Serial.printf("[CSI] wifi_init failed: %d\n", ret);
    return;
  }

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  esp_wifi_set_channel(operatingChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(true);

  wifi_csi_config_t csi_cfg = {
    .lltf_en = true,
    .htltf_en = true,
    .stbc_htltf2_en = false,
    .ltf_merge_en = true,
    .channel_filter_en = false,
    .manu_scale = false,
    .shift = 0,
  };

  esp_wifi_set_csi_rx_cb(&csiDataCallback, this);
  esp_wifi_set_csi_config(&csi_cfg);
  esp_wifi_set_csi(true);

  wifiOwned = true;
  Serial.printf("[CSI] WiFi initialized on ch %u\n", operatingChannel);
}

void CsiModule::releaseWiFi() {
  if (!wifiOwned) return;

  esp_wifi_set_csi(false);
  esp_wifi_set_csi_rx_cb(NULL, NULL);
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_deinit();

  wifiOwned = false;
  wifi_scan_obj.wifi_initialized = false;

  Serial.println("[CSI] WiFi released");
}

void CsiModule::start(uint8_t channel) {
  if (streamState == CSI_STREAM_ACTIVE) return;

  operatingChannel = channel;
  packetCount = 0;
  peakRssi = -128;
  motionScore = 0;
  totalEnergy = 0;
  personDetected = false;
  returnToMenu = false;
  viewMode = 0;
  numSub = 0;

  for (int i = 0; i < CSI_MAX_SC; i++) {
    subMag[i] = 0;
    subEma[i] = 0;
    subDev[i] = 0;
  }

  takeWiFi();
  streamState = CSI_STREAM_ACTIVE;

#ifdef HAS_SCREEN
  display_obj.tft.fillScreen(TFT_BLACK);
  drawHeader();
  drawHints();
#endif

  Serial.printf("[CSI] Started on ch %u\n", channel);
}

void CsiModule::stop() {
  if (streamState == CSI_STREAM_IDLE) return;

  streamState = CSI_STREAM_IDLE;
  releaseWiFi();

  Serial.printf("[CSI] Stopped. Pkts: %u, Peak RSSI: %d\n", packetCount, peakRssi);

#ifdef HAS_SCREEN
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setCursor(2, TFT_HEIGHT / 2 - 4);
  display_obj.tft.print("CSI Stopped");
#endif

  returnToMenu = true;
}

// ---- Callback (ISR context - keep fast) ----
void CsiModule::csiDataCallback(void *ctx, wifi_csi_info_t *data) {
  if (!ctx || !data) return;
  static_cast<CsiModule*>(ctx)->processCsiData(data);
}

void CsiModule::processCsiData(wifi_csi_info_t *data) {
  packetCount++;
  lastPacketTime = millis();

  if (data->rx_ctrl.rssi > peakRssi)
    peakRssi = data->rx_ctrl.rssi;

  sendCsiBinary(data);
  computeMag(data);
}

void CsiModule::computeMag(wifi_csi_info_t *data) {
  int8_t *buf = data->buf;
  uint16_t len = data->len;
  uint8_t n = len / 2;
  if (n > CSI_MAX_SC) n = CSI_MAX_SC;
  numSub = n;

  uint32_t energy = 0;
  for (int i = 0; i < n; i++) {
    int16_t r = buf[i * 2];
    int16_t q = buf[i * 2 + 1];
    uint16_t mag = (r < 0 ? -r : r) + (q < 0 ? -q : q);
    subMag[i] = mag;
    energy += mag;

    int16_t d = (int16_t)mag - subEma[i];
    subEma[i] += (d >> CSI_EMA_SHIFT);
    subDev[i] += ((int16_t)((d < 0 ? -d : d) - subDev[i]) >> CSI_VAR_SHIFT);
  }
  totalEnergy = energy;
}

void CsiModule::updateMotion() {
  uint32_t sum = 0;
  for (int i = 0; i < numSub && i < CSI_MAX_SC; i++)
    sum += subDev[i];
  motionScore = sum;
  personDetected = (sum > CSI_MOTION_THRESH);
}

// ---- Display drawing ----
void CsiModule::drawHeader() {
  display_obj.tft.fillRect(0, 0, TFT_WIDTH, CSI_HDR_H, TFT_NAVY);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_NAVY);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setCursor(2, 2);
  display_obj.tft.printf("CSI CH%u", operatingChannel);
  display_obj.tft.setCursor(TFT_WIDTH - 66, 2);
  display_obj.tft.printf("Pkts:%05u", packetCount);
}

void CsiModule::drawHints() {
  display_obj.tft.fillRect(0, CSI_HINT_Y, TFT_WIDTH, CSI_HINT_H, TFT_BLACK);
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setCursor(2, CSI_HINT_Y + 2);
  display_obj.tft.print("C=Stop");
  display_obj.tft.setCursor(TFT_WIDTH / 2, CSI_HINT_Y + 2);
  display_obj.tft.print("D=View");
}

void CsiModule::drawGraph() {
  int n = numSub;
  if (n < 2) n = CSI_MAX_SC;
  if (n == 0) return;

  display_obj.tft.fillRect(0, CSI_GRAPH_Y, TFT_WIDTH, CSI_GRAPH_H, TFT_BLACK);

  int barW = TFT_WIDTH / n;
  if (barW < 1) barW = 1;
  if (barW > 3) barW = 3;

  int gap = barW > 1 ? 1 : 0;
  int maxH = CSI_GRAPH_H - 4;
  int maxMag = 80;

  for (int i = 0; i < n && i < CSI_MAX_SC; i++) {
    uint16_t mag = subMag[i];
    int h = (mag * maxH) / maxMag;
    if (h > maxH) h = maxH;
    if (h < 1) h = 1;

    uint16_t c;
    float r = (float)mag / maxMag;
    if (r < 0.33f)      c = TFT_GREEN;
    else if (r < 0.66f) c = TFT_YELLOW;
    else                c = TFT_RED;

    int x = i * (barW + gap);
    int y0 = CSI_GRAPH_Y + CSI_GRAPH_H - h - 2;
    display_obj.tft.fillRect(x, y0, barW, h, c);
  }
}

void CsiModule::drawStats() {
  display_obj.tft.fillRect(0, CSI_STAT_Y, TFT_WIDTH, CSI_STAT_H, TFT_BLACK);

  display_obj.tft.setTextSize(1);

  // Line 1: RSSI + Packets
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.setCursor(2, CSI_STAT_Y + 1);
  display_obj.tft.printf("RSSI:%3d  Pkts:%u", peakRssi, packetCount);

  // Line 2: Energy + Subcarriers
  display_obj.tft.setCursor(2, CSI_STAT_Y + 11);
  display_obj.tft.printf("E:%05u  SC:%u", totalEnergy, numSub);

  // Line 3: Person status (colored background)
  int py = CSI_STAT_Y + 24;
  if (personDetected) {
    display_obj.tft.fillRect(0, py, TFT_WIDTH, 16, TFT_RED);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_RED);
    display_obj.tft.setCursor(2, py + 3);
    display_obj.tft.setTextSize(1);
    display_obj.tft.print("! PERSON DETECTED");
    display_obj.tft.setCursor(TFT_WIDTH - 50, py + 3);
    display_obj.tft.printf("M:%u", motionScore);
  } else {
    display_obj.tft.fillRect(0, py, TFT_WIDTH, 16, TFT_DARKGREEN);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    display_obj.tft.setCursor(2, py + 3);
    display_obj.tft.print("O CLEAR");
    display_obj.tft.setCursor(TFT_WIDTH - 50, py + 3);
    display_obj.tft.printf("M:%u", motionScore);
  }
}

void CsiModule::sendCsiBinary(wifi_csi_info_t *data) {
  uint16_t csiLen = data->len;
  uint32_t now = lastPacketTime;

  Serial.write(CSI_SYNC0);
  Serial.write(CSI_SYNC1);

  Serial.write((uint8_t)(now & 0xFF));
  Serial.write((uint8_t)((now >> 8) & 0xFF));
  Serial.write((uint8_t)((now >> 16) & 0xFF));
  Serial.write((uint8_t)((now >> 24) & 0xFF));

  Serial.write((uint8_t)(packetCount & 0xFF));
  Serial.write((uint8_t)((packetCount >> 8) & 0xFF));
  Serial.write((uint8_t)((packetCount >> 16) & 0xFF));
  Serial.write((uint8_t)((packetCount >> 24) & 0xFF));

  Serial.write((uint8_t)data->rx_ctrl.channel);
  Serial.write((uint8_t)data->rx_ctrl.rssi);
  Serial.write((uint8_t)data->rx_ctrl.rate);
  Serial.write((uint8_t)data->rx_ctrl.sig_mode);

  Serial.write((uint8_t)(csiLen & 0xFF));
  Serial.write((uint8_t)((csiLen >> 8) & 0xFF));

  Serial.write((uint8_t*)data->buf, csiLen);
}

// ---- Main loop (called every iteration) ----
void CsiModule::main(uint32_t currentTime) {
  if (streamState != CSI_STREAM_ACTIVE) return;

  // Button handling
#ifdef HAS_BUTTONS
  #if (D_BTN >= 0)
  if (d_btn.justPressed()) {
    viewMode = !viewMode;
    display_obj.tft.fillRect(0, CSI_GRAPH_Y, TFT_WIDTH, CSI_GRAPH_H, TFT_BLACK);
  }
  #endif
  #if (C_BTN >= 0)
  if (c_btn.justPressed()) {
    stop();
    return;
  }
  #endif
#endif

  // Motion detection (every 200ms)
  if (currentTime - lastMotionUpdate > 200) {
    lastMotionUpdate = currentTime;
    updateMotion();
  }

#ifdef HAS_SCREEN
  // Header update (every 500ms, only update packet count number)
  if (currentTime - lastDisplayUpdate > 500) {
    lastDisplayUpdate = currentTime;
    display_obj.tft.setTextColor(TFT_WHITE, TFT_NAVY);
    display_obj.tft.setCursor(TFT_WIDTH - 66, 2);
    display_obj.tft.printf("Pkts:%05u", packetCount);
  }

  // Graph/stats update (every 400ms)
  if (currentTime - lastGraphUpdate > 400) {
    lastGraphUpdate = currentTime;
    if (viewMode == 0)
      drawGraph();
    drawStats();
  }
#endif
}
