#include "CsiMotionMonitor.h"

#if defined(HAS_CSI_MOTION)

#include <string.h>
#include <new>
#include "WiFiScan.h"
#if defined(HAS_SCREEN)
  #include "Display.h"
#endif
#include "settings.h"

extern WiFiScan wifi_scan_obj;
#if defined(HAS_SCREEN)
  extern Display display_obj;
#endif
extern Settings settings_obj;

namespace {
String csiScreenName(String value) {
  if (!settings_obj.loadSetting<bool>("Censor")) return value;
  for (size_t i = 3; i < value.length(); ++i) value.setCharAt(i, 'X');
  return value;
}
}

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
  #if !CONFIG_SOC_WIFI_HE_SUPPORT
    config.lltf_en = true;
    // Keep one stable 64-tone layout across legacy and HT packets. Mixing
    // HT-LTF/STBC blocks changes vector length and breaks window continuity.
    config.htltf_en = false;
    config.stbc_htltf2_en = false;
    config.ltf_merge_en = false;
    config.channel_filter_en = false;
    config.manu_scale = false;
    config.shift = 0;
    #if ESP_IDF_VERSION_MAJOR >= 5
      config.dump_ack_en = false;
    #endif
  #else
    config.enable = 1;
    config.acquire_csi_legacy = 1;
    #if CONFIG_SOC_WIFI_MAC_VERSION_NUM == 3
      // C5 can force the same L-LTF shape for legacy, HT, VHT and HE frames.
      config.acquire_csi_force_lltf = 1;
      config.acquire_csi_ht20 = 1;
      config.acquire_csi_ht40 = 1;
      config.acquire_csi_vht = 1;
      config.acquire_csi_su = 1;
      config.acquire_csi_mu = 1;
      config.acquire_csi_dcm = 1;
      config.acquire_csi_beamformed = 1;
      config.acquire_csi_he_stbc_mode = ESP_CSI_ACQUIRE_STBC_HELTF1;
    #else
      // C6 cannot force L-LTF for every modulation. Restrict capture to
      // legacy frames so every one-second motion window has one tone layout.
      config.acquire_csi_ht20 = 0;
      config.acquire_csi_ht40 = 0;
      config.acquire_csi_su = 0;
      config.acquire_csi_mu = 0;
      config.acquire_csi_dcm = 0;
      config.acquire_csi_beamformed = 0;
      config.acquire_csi_he_stbc = ESP_CSI_ACQUIRE_STBC_HELTF1;
    #endif
    config.val_scale_cfg = 0;
    config.dump_ack_en = 0;
  #endif
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
  target_count_ = 1;
  targets_ = static_cast<Target*>(ps_malloc(sizeof(Target) * target_count_));
  if (!targets_)
    targets_ = static_cast<Target*>(malloc(sizeof(Target) * target_count_));
  if (!targets_) {
    target_count_ = 0;
    return false;
  }
  Target* target_state = new (&targets_[0]) Target();
  memcpy(target_state->bssid, target_, sizeof(target_state->bssid));
  target_state->accepted = target_state->dropped = 0;
  target_state->model.reset(millis(), 30000);
  write_index_ = read_index_ = 0;
  accepted_ = dropped_ = 0;
  started_ms_ = millis();
  last_fusion_ms_ = started_ms_;
  motion_score_ = amplitude_score_ = phase_score_ = correlation_score_ = 0;
  reliability_ = 0;
  samples_per_second_ = 0;
  motion_votes_ = still_votes_ = 0;
  state_ = CsiMotionModel::State::CALIBRATING;
  last_serial_ms_ = 0;
  memset(history_, 0, sizeof(history_));
  history_head_ = 0;
  if (!takeWiFi()) {
    releaseWiFi();
    releaseTargets();
    return false;
  }
  active_ = true;
  wifi_scan_obj.currentScanMode = WIFI_SCAN_CSI_MOTION;
  #if defined(HAS_SCREEN)
    display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                             SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
    draw(millis());
  #endif
  Serial.printf("CSI motion started: AP=\"%s\" BSSID=%02X:%02X:%02X:%02X:%02X:%02X channel=%u; keep the device still for 30 seconds. Stop with stopscan.\n",
      target_name_.c_str(), target_[0], target_[1], target_[2], target_[3],
      target_[4], target_[5], channel_);
  return true;
}

void CsiMotionMonitor::stop() {
  active_ = false;
  releaseWiFi();
  releaseTargets();
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CSI_MOTION)
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
}

void CsiMotionMonitor::recalibrate() {
  write_index_ = read_index_ = 0;
  accepted_ = dropped_ = 0;
  started_ms_ = millis();
  for (uint16_t i = 0; i < target_count_; ++i) {
    targets_[i].accepted = targets_[i].dropped = 0;
    targets_[i].model.reset(started_ms_, 30000);
  }
  motion_score_ = amplitude_score_ = phase_score_ = correlation_score_ = 0;
  reliability_ = 0;
  samples_per_second_ = 0;
  state_ = CsiMotionModel::State::CALIBRATING;
  last_serial_ms_ = 0;
  memset(history_, 0, sizeof(history_));
  history_head_ = 0;
}

void CsiMotionMonitor::callback(void* context, wifi_csi_info_t* info) {
  if (context && info)
    static_cast<CsiMotionMonitor*>(context)->enqueue(info);
}

void CsiMotionMonitor::enqueue(wifi_csi_info_t* info) {
  if (!active_ || !info->buf)
    return;
  const int target_index = findTarget(info->mac);
  if (target_index < 0)
    return;
  const uint8_t next = static_cast<uint8_t>((write_index_ + 1) % kQueueSize);
  if (next == read_index_) {
    ++dropped_;
    ++targets_[target_index].dropped;
    return;
  }
  Sample& sample = queue_[write_index_];
  sample.target_index = static_cast<uint16_t>(target_index);
  sample.length = info->len > kMaxCsiBytes ? kMaxCsiBytes : info->len;
  sample.rssi = info->rx_ctrl.rssi;
  sample.first_word_invalid = info->first_word_invalid;
  sample.timestamp = millis();
  memcpy(sample.iq, info->buf, sample.length);
  write_index_ = next;
  ++accepted_;
  ++targets_[target_index].accepted;
}

int CsiMotionMonitor::findTarget(const uint8_t mac[6]) const {
  for (uint16_t i = 0; i < target_count_; ++i) {
    if (memcmp(mac, targets_[i].bssid, 6) == 0)
      return i;
  }
  return -1;
}

void CsiMotionMonitor::releaseTargets() {
  if (!targets_) {
    target_count_ = 0;
    return;
  }
  for (uint16_t i = 0; i < target_count_; ++i)
    targets_[i].~Target();
  free(targets_);
  targets_ = nullptr;
  target_count_ = 0;
  active_target_count_ = 0;
}

const char* CsiMotionMonitor::stateLabel() const {
  switch (state_) {
    case CsiMotionModel::State::CALIBRATING: return "CALIBRATING";
    case CsiMotionModel::State::INSUFFICIENT_SAMPLES: return "LOW DATA";
    case CsiMotionModel::State::POSSIBLE_MOTION: return "POSSIBLE";
    case CsiMotionModel::State::MOTION: return "MOTION";
    case CsiMotionModel::State::STILL: return "STILL";
  }
  return "UNKNOWN";
}

void CsiMotionMonitor::drawGraph() {
  #if !defined(HAS_SCREEN)
    return;
  #else
  #if SCREEN_HEIGHT <= 160
    const int graph_y = STATUS_BAR_WIDTH + 62;
    const int graph_h = SCREEN_HEIGHT - graph_y;
  #else
    const int content_height = SCREEN_HEIGHT - STATUS_BAR_WIDTH;
    const int graph_y = STATUS_BAR_WIDTH + content_height / 2;
    const int graph_h = SCREEN_HEIGHT - graph_y - 18;
  #endif
  display_obj.tft.fillRect(0, graph_y, SCREEN_WIDTH, graph_h, TFT_BLACK);
  display_obj.tft.drawRect(0, graph_y, SCREEN_WIDTH, graph_h, TFT_DARKGREY);
  const uint8_t* history = history_;
  const size_t head = history_head_;
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
  #endif
}

void CsiMotionMonitor::draw(uint32_t now_ms) {
  #if !defined(HAS_SCREEN)
    (void)now_ms;
    return;
  #elif SCREEN_HEIGHT <= 160
  const int graph_y = STATUS_BAR_WIDTH + 62;
  const int title_y = STATUS_BAR_WIDTH + 1;
  display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                           graph_y - STATUS_BAR_WIDTH, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.setCursor(2, title_y);
  display_obj.tft.print("CSI Motion");
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.setCursor(2, title_y + 10);
  const String display_target = csiScreenName(target_name_);
  display_obj.tft.printf("AP:%.12s C:%u", display_target.c_str(), channel_);
  display_obj.tft.setCursor(2, title_y + 20);
  if (now_ms - started_ms_ < 30000) {
    display_obj.tft.printf("M:%u R:%u CAL:%u%%", motion_score_, reliability_,
        static_cast<unsigned>((now_ms - started_ms_) * 100U / 30000U));
  } else {
    display_obj.tft.printf("M:%u R:%u %s", motion_score_, reliability_, stateLabel());
  }
  display_obj.tft.setCursor(2, title_y + 30);
  display_obj.tft.printf("A:%u P:%u C:%u", amplitude_score_, phase_score_,
      correlation_score_);
  display_obj.tft.setCursor(2, title_y + 40);
  display_obj.tft.printf("Rate:%u D:%lu", samples_per_second_,
      static_cast<unsigned long>(dropped_));
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.setCursor(2, title_y + 50);
  display_obj.tft.print("Up=Cal C=Stop");
  #else
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
  const String display_target = csiScreenName(target_name_);
  display_obj.tft.printf("AP: %.16s CH:%u", display_target.c_str(), channel_);
  display_obj.tft.setCursor(2, title_y + 26);
  display_obj.tft.printf("Rate:%u/s Drop:%lu", samples_per_second_,
      static_cast<unsigned long>(dropped_));
  display_obj.tft.setCursor(2, title_y + 38);
  display_obj.tft.printf("Motion:%u Reliability:%u", motion_score_, reliability_);
  display_obj.tft.setCursor(2, title_y + 50);
  display_obj.tft.printf("Amp:%u Phase:%u Corr:%u", amplitude_score_,
      phase_score_, correlation_score_);
  display_obj.tft.setCursor(2, title_y + 62);
  display_obj.tft.printf("State: %s", stateLabel());
  if (now_ms - started_ms_ < 30000) {
    display_obj.tft.setCursor(2, title_y + 74);
    display_obj.tft.printf("Keep still: %u%%",
        static_cast<unsigned>((now_ms - started_ms_) * 100U / 30000U));
  }
  display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  display_obj.tft.setCursor(2, graph_y - 12);
  #if defined(HAS_CSI_BUTTON_CONTROLS)
    display_obj.tft.print("Up=Recalibrate Center=Stop");
  #else
    display_obj.tft.print("Top=Recalibrate Bottom=Stop");
  #endif
  #endif
  drawGraph();
}

void CsiMotionMonitor::printStatus(bool force) {
  const uint32_t now_ms = millis();
  if (!force && now_ms - last_serial_ms_ < 1000) return;
  last_serial_ms_ = now_ms;
  const uint8_t calibration = now_ms - started_ms_ >= 30000 ? 100 :
      static_cast<uint8_t>((now_ms - started_ms_) * 100U / 30000U);
  Serial.printf(
      "CSI motion | state=%s | motion=%u | reliability=%u | amp=%u | phase=%u | corr=%u | rate=%u/s | dropped=%lu | calibration=%u%% | AP=\"%s\" | channel=%u\n",
      stateLabel(), motion_score_, reliability_, amplitude_score_, phase_score_,
      correlation_score_, samples_per_second_,
      static_cast<unsigned long>(dropped_), calibration, target_name_.c_str(),
      channel_);
}

void CsiMotionMonitor::updateFusion(uint32_t now_ms) {
  if (now_ms - last_fusion_ms_ < 1000)
    return;
  last_fusion_ms_ = now_ms;
  struct Candidate { uint8_t score, amp, phase, corr, reliability; };
  Candidate best[3] = {};
  uint32_t reliability_sum = 0;
  uint32_t total_rate = 0;
  active_target_count_ = 0;
  for (uint16_t i = 0; i < target_count_; ++i) {
    CsiMotionModel& model = targets_[i].model;
    model.tick(now_ms, targets_[i].accepted, targets_[i].dropped);
    total_rate += model.samplesPerSecond();
    if (!model.calibrated() || model.reliability() < 25)
      continue;
    ++active_target_count_;
    reliability_sum += model.reliability();
    Candidate candidate = {model.motionScore(), model.amplitudeScore(),
        model.phaseScore(), model.correlationScore(), model.reliability()};
    for (size_t slot = 0; slot < 3; ++slot) {
      if (candidate.score > best[slot].score) {
        for (size_t move = 2; move > slot; --move) best[move] = best[move - 1];
        best[slot] = candidate;
        break;
      }
    }
  }
  samples_per_second_ = static_cast<uint16_t>(total_rate > 65535 ? 65535 : total_rate);
  reliability_ = active_target_count_ ? static_cast<uint8_t>(reliability_sum / active_target_count_) : 0;
  uint32_t weight_sum = 0, motion_sum = 0, amp_sum = 0, phase_sum = 0, corr_sum = 0;
  for (size_t i = 0; i < 3 && best[i].reliability; ++i) {
    const uint32_t weight = best[i].reliability;
    weight_sum += weight;
    motion_sum += best[i].score * weight;
    amp_sum += best[i].amp * weight;
    phase_sum += best[i].phase * weight;
    corr_sum += best[i].corr * weight;
  }
  motion_score_ = weight_sum ? motion_sum / weight_sum : 0;
  amplitude_score_ = weight_sum ? amp_sum / weight_sum : 0;
  phase_score_ = weight_sum ? phase_sum / weight_sum : 0;
  correlation_score_ = weight_sum ? corr_sum / weight_sum : 0;
  if (now_ms - started_ms_ < 30000) {
    state_ = CsiMotionModel::State::CALIBRATING;
  } else if (!active_target_count_) {
    state_ = CsiMotionModel::State::INSUFFICIENT_SAMPLES;
  } else if (motion_score_ >= CsiMotionModel::kMotionThreshold) {
    motion_votes_ = motion_votes_ < 5 ? motion_votes_ + 1 : 5;
    still_votes_ = 0;
    state_ = motion_votes_ >= 2 ? CsiMotionModel::State::MOTION :
                                  CsiMotionModel::State::POSSIBLE_MOTION;
  } else if (motion_score_ >= CsiMotionModel::kPossibleMotionThreshold) {
    motion_votes_ = still_votes_ = 0;
    state_ = CsiMotionModel::State::POSSIBLE_MOTION;
  } else {
    motion_votes_ = 0;
    still_votes_ = still_votes_ < 3 ? still_votes_ + 1 : 3;
    if (still_votes_ >= 2) state_ = CsiMotionModel::State::STILL;
  }
  history_[history_head_] = reliability_ < 15 ? 255 : motion_score_;
  history_head_ = (history_head_ + 1) % CsiMotionModel::kHistorySize;
}

void CsiMotionMonitor::main(uint32_t now_ms) {
  if (!active_)
    return;
  while (read_index_ != write_index_) {
    Sample& sample = queue_[read_index_];
    if (sample.target_index < target_count_) {
      targets_[sample.target_index].model.addSample(
          sample.iq, sample.length, sample.rssi, sample.timestamp,
          sample.first_word_invalid,
          #if defined(CONFIG_IDF_TARGET_ESP32C5)
            CsiMotionModel::SampleEncoding::SIGNED_12_LE
          #else
            CsiMotionModel::SampleEncoding::SIGNED_8_IMAG_REAL
          #endif
          );
    }
    read_index_ = static_cast<uint8_t>((read_index_ + 1) % kQueueSize);
  }
  updateFusion(now_ms);
  printStatus(false);
  #if defined(HAS_SCREEN)
  if (now_ms - last_draw_ms_ >= 500) {
    last_draw_ms_ = now_ms;
    draw(now_ms);
  }
  #endif
}

#endif
