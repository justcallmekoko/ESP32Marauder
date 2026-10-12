#include "CsiMotionMonitor.h"

#if defined(MARAUDER_V8)

#include <string.h>
#include "WiFiScan.h"
#include "Display.h"

extern WiFiScan wifi_scan_obj;
extern Display display_obj;

bool CsiMotionMonitor::takeWiFi() {
  if (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF)
    wifi_scan_obj.StopScan(wifi_scan_obj.currentScanMode);
  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  init.csi_enable = true;
  esp_err_t result = esp_wifi_init(&init);
  if (result == ESP_ERR_WIFI_INIT_STATE) {
    esp_wifi_stop();
    esp_wifi_deinit();
    result = esp_wifi_init(&init);
  }
  if (result != ESP_OK)
    return false;
  wifi_owned_ = true;
  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
      esp_wifi_start() != ESP_OK ||
      esp_wifi_set_channel(channel_, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
      esp_wifi_set_promiscuous(true) != ESP_OK)
    return false;

  wifi_csi_config_t config = {};
  config.enable = 1;
  config.acquire_csi_legacy = 1;
  config.acquire_csi_force_lltf = 1;
  config.acquire_csi_ht20 = 1;
  config.acquire_csi_ht40 = 1;
  config.acquire_csi_vht = 1;
  config.acquire_csi_su = 1;
  config.acquire_csi_mu = 1;
  config.acquire_csi_dcm = 1;
  config.acquire_csi_beamformed = 1;
  config.acquire_csi_he_stbc_mode = ESP_CSI_ACQUIRE_STBC_HELTF1;
  config.val_scale_cfg = 0;
  config.dump_ack_en = 0;
  if (esp_wifi_set_csi_rx_cb(&CsiMotionMonitor::callback, this) != ESP_OK ||
      esp_wifi_set_csi_config(&config) != ESP_OK ||
      esp_wifi_set_csi(true) != ESP_OK)
    return false;
  wifi_scan_obj.wifi_initialized = true;
  return true;
}

void CsiMotionMonitor::releaseWiFi() {
  if (!wifi_owned_)
    return;
  esp_wifi_set_csi(false);
  esp_wifi_set_csi_rx_cb(nullptr, nullptr);
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_owned_ = false;
  wifi_scan_obj.wifi_initialized = false;
}

bool CsiMotionMonitor::start(const uint8_t target[6], const String& name,
                             uint8_t channel) {
  stop();
  memcpy(target_, target, sizeof(target_));
  target_name_ = name;
  channel_ = channel;
  write_index_ = read_index_ = 0;
  accepted_ = dropped_ = 0;
  model_.reset(millis(), 30000);
  if (!takeWiFi()) {
    releaseWiFi();
    return false;
  }
  active_ = true;
  wifi_scan_obj.currentScanMode = WIFI_SCAN_CSI_MOTION;
  display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                           SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  draw(millis());
  return true;
}

void CsiMotionMonitor::stop() {
  active_ = false;
  releaseWiFi();
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CSI_MOTION)
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
}

void CsiMotionMonitor::recalibrate() {
  write_index_ = read_index_ = 0;
  accepted_ = dropped_ = 0;
  model_.reset(millis(), 30000);
}

void CsiMotionMonitor::callback(void* context, wifi_csi_info_t* info) {
  if (context && info)
    static_cast<CsiMotionMonitor*>(context)->enqueue(info);
}

void CsiMotionMonitor::enqueue(wifi_csi_info_t* info) {
  if (!active_ || !info->buf || memcmp(info->mac, target_, 6) != 0)
    return;
  const uint8_t next = static_cast<uint8_t>((write_index_ + 1) % kQueueSize);
  if (next == read_index_) {
    ++dropped_;
    return;
  }
  Sample& sample = queue_[write_index_];
  sample.length = info->len > kMaxCsiBytes ? kMaxCsiBytes : info->len;
  sample.rssi = info->rx_ctrl.rssi;
  sample.timestamp = millis();
  memcpy(sample.iq, info->buf, sample.length);
  write_index_ = next;
  ++accepted_;
}

const char* CsiMotionMonitor::stateLabel() const {
  switch (model_.state()) {
    case CsiMotionModel::State::CALIBRATING: return "CALIBRATING";
    case CsiMotionModel::State::INSUFFICIENT_SAMPLES: return "LOW DATA";
    case CsiMotionModel::State::POSSIBLE_MOTION: return "POSSIBLE";
    case CsiMotionModel::State::MOTION: return "MOTION";
    case CsiMotionModel::State::STILL: return "STILL";
  }
  return "UNKNOWN";
}

void CsiMotionMonitor::drawGraph() {
  const int content_height = SCREEN_HEIGHT - STATUS_BAR_WIDTH;
  const int graph_y = STATUS_BAR_WIDTH + content_height / 2;
  const int graph_h = SCREEN_HEIGHT - graph_y - 18;
  display_obj.tft.fillRect(0, graph_y, SCREEN_WIDTH, graph_h, TFT_BLACK);
  display_obj.tft.drawRect(0, graph_y, SCREEN_WIDTH, graph_h, TFT_DARKGREY);
  const uint8_t* history = model_.history();
  const size_t head = model_.historyHead();
  for (int x = 1; x < SCREEN_WIDTH; ++x) {
    const size_t offset =
        (static_cast<size_t>(SCREEN_WIDTH - x) * CsiMotionModel::kHistorySize) /
        SCREEN_WIDTH;
    size_t a = (head + CsiMotionModel::kHistorySize - offset - 1) %
               CsiMotionModel::kHistorySize;
    size_t b = (a + 1) % CsiMotionModel::kHistorySize;
    if (history[a] == 255 || history[b] == 255)
      continue;
    int y1 = graph_y + graph_h - 2 - (history[a] * (graph_h - 4) / 100);
    int y2 = graph_y + graph_h - 2 - (history[b] * (graph_h - 4) / 100);
    display_obj.tft.drawLine(x - 1, y1, x, y2,
                            history[b] >= CsiMotionModel::kMotionThreshold
                                ? TFT_RED : TFT_CYAN);
  }
  int threshold_y = graph_y + graph_h - 2 -
      (CsiMotionModel::kMotionThreshold * (graph_h - 4) / 100);
  display_obj.tft.drawFastHLine(1, threshold_y, SCREEN_WIDTH - 2, TFT_ORANGE);
}

void CsiMotionMonitor::draw(uint32_t now_ms) {
  const int content_height = SCREEN_HEIGHT - STATUS_BAR_WIDTH;
  const int graph_y = STATUS_BAR_WIDTH + content_height / 2;
  const int title_y = STATUS_BAR_WIDTH + 2;
  display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                           graph_y - STATUS_BAR_WIDTH, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.setCursor(2, title_y);
  display_obj.tft.print("CSI Motion Monitor");
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.setCursor(2, title_y + 14);
  display_obj.tft.printf("AP: %.20s", target_name_.c_str());
  display_obj.tft.setCursor(2, title_y + 26);
  display_obj.tft.printf("%02X:%02X:%02X:%02X:%02X:%02X CH:%u",
      target_[0], target_[1], target_[2], target_[3], target_[4], target_[5],
      channel_);
  display_obj.tft.setCursor(2, title_y + 38);
  display_obj.tft.printf("RSSI:%d Rate:%u/s Drop:%lu", model_.rssi(),
      model_.samplesPerSecond(), static_cast<unsigned long>(dropped_));
  display_obj.tft.setCursor(2, title_y + 50);
  display_obj.tft.printf("Motion:%u Reliability:%u", model_.motionScore(),
      model_.reliability());
  display_obj.tft.setCursor(2, title_y + 62);
  display_obj.tft.printf("State: %s", stateLabel());
  if (!model_.calibrated()) {
    display_obj.tft.setCursor(2, title_y + 74);
    display_obj.tft.printf("Keep still: %u%%", model_.calibrationPercent(now_ms));
  }
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.setCursor(2, graph_y - 12);
  display_obj.tft.print("Top=Recalibrate Bottom=Stop");
  drawGraph();
}

void CsiMotionMonitor::main(uint32_t now_ms) {
  if (!active_)
    return;
  while (read_index_ != write_index_) {
    Sample& sample = queue_[read_index_];
    model_.addSample(sample.iq, sample.length, sample.rssi, sample.timestamp);
    read_index_ = static_cast<uint8_t>((read_index_ + 1) % kQueueSize);
  }
  model_.tick(now_ms, accepted_, dropped_);
  if (now_ms - last_draw_ms_ >= 500) {
    last_draw_ms_ = now_ms;
    draw(now_ms);
  }
}

#endif
