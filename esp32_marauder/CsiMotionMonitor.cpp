#include "CsiMotionMonitor.h"

#if defined(MARAUDER_V8)

#include <string.h>
#include <new>
#include "mbedtls/base64.h"
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
  accepted_ = dropped_ = sequence_ = 0;
  started_ms_ = millis();
  last_fusion_ms_ = started_ms_;
  motion_score_ = amplitude_score_ = phase_score_ = correlation_score_ = 0;
  reliability_ = 0;
  samples_per_second_ = 0;
  motion_votes_ = still_votes_ = 0;
  state_ = CsiMotionModel::State::CALIBRATING;
  memset(history_, 0, sizeof(history_));
  history_head_ = 0;
  if (!takeWiFi()) {
    releaseWiFi();
    releaseTargets();
    return false;
  }
  active_ = true;
  wifi_scan_obj.currentScanMode = WIFI_SCAN_CSI_MOTION;
  writeSessionRecord("START", started_ms_);
  display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                           SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
  draw(millis());
  return true;
}

void CsiMotionMonitor::stop() {
  if (active_)
    writeSessionRecord("STOP", millis());
  active_ = false;
  releaseWiFi();
  releaseTargets();
  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CSI_MOTION)
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
}

void CsiMotionMonitor::recalibrate() {
  write_index_ = read_index_ = 0;
  accepted_ = dropped_ = sequence_ = 0;
  started_ms_ = millis();
  for (uint16_t i = 0; i < target_count_; ++i) {
    targets_[i].accepted = targets_[i].dropped = 0;
    targets_[i].model.reset(started_ms_, 30000);
  }
  motion_score_ = amplitude_score_ = phase_score_ = correlation_score_ = 0;
  reliability_ = 0;
  samples_per_second_ = 0;
  state_ = CsiMotionModel::State::CALIBRATING;
  memset(history_, 0, sizeof(history_));
  history_head_ = 0;
  writeSessionRecord("RECALIBRATE", started_ms_);
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
  const uint32_t sequence = sequence_++;
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
  sample.noise_floor = info->rx_ctrl.noise_floor;
  sample.rate = info->rx_ctrl.rate;
  sample.format = info->rx_ctrl.cur_bb_format;
  sample.channel = info->rx_ctrl.channel;
  sample.secondary_channel = info->rx_ctrl.second;
  sample.rx_state = info->rx_ctrl.rx_state;
  sample.first_word_invalid = info->first_word_invalid;
  sample.signal_length = info->rx_ctrl.sig_len;
  sample.timestamp = millis();
  sample.rx_timestamp = info->rx_ctrl.timestamp;
  sample.sequence = sequence;
  memcpy(sample.iq, info->buf, sample.length);
  write_index_ = next;
  ++accepted_;
  ++targets_[target_index].accepted;
}

void CsiMotionMonitor::writeRawRecord(const Sample& sample) {
  // Base64 keeps a complete signed I/Q byte vector while using substantially
  // less serial bandwidth than hexadecimal or decimal CSV.
  unsigned char encoded[((kMaxCsiBytes + 2) / 3) * 4 + 1];
  size_t encoded_length = 0;
  if (mbedtls_base64_encode(encoded, sizeof(encoded) - 1, &encoded_length,
          reinterpret_cast<const unsigned char*>(sample.iq), sample.length) != 0)
    return;
  encoded[encoded_length] = '\0';
  Serial.printf("CSI_RAW,2,%lu,%lu,%lu,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%lu,%lu,",
      static_cast<unsigned long>(sample.sequence),
      static_cast<unsigned long>(sample.rx_timestamp),
      static_cast<unsigned long>(sample.timestamp), sample.rssi, sample.noise_floor,
      sample.rate, sample.format, sample.channel, sample.secondary_channel,
      sample.signal_length, sample.rx_state, sample.first_word_invalid ? 1 : 0,
      sample.length,
      static_cast<unsigned long>(accepted_),
      static_cast<unsigned long>(dropped_));
  Serial.write(encoded, encoded_length);
  Serial.println();
}

void CsiMotionMonitor::writeMetricRecord(uint32_t now_ms) {
  Serial.printf("CSI_METRIC,1,%lu,%u,%lu,%u,%u,%u,%u,%u,%s\n",
      static_cast<unsigned long>(now_ms), samples_per_second_,
      static_cast<unsigned long>(dropped_), reliability_, amplitude_score_,
      phase_score_, correlation_score_, motion_score_, stateLabel());
}

void CsiMotionMonitor::writeSessionRecord(const char* event, uint32_t now_ms) {
  Serial.printf("CSI_SESSION,1,%s,%lu,%02X:%02X:%02X:%02X:%02X:%02X,%u\n",
      event, static_cast<unsigned long>(now_ms), target_[0], target_[1],
      target_[2], target_[3], target_[4], target_[5], channel_);
  if (strcmp(event, "START") == 0) {
    Serial.println("CSI_SCHEMA,2,RAW,seq,rx_us,capture_ms,rssi,noise,rate,format,channel,secondary,sig_len,rx_state,first_word_invalid,csi_len,accepted,dropped,iq_base64");
    Serial.println("CSI_SCHEMA,1,METRIC,ms,rate_hz,dropped,reliability,amp,phase,corr,motion,state");
  }
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
  const int content_height = SCREEN_HEIGHT - STATUS_BAR_WIDTH;
  const int graph_y = STATUS_BAR_WIDTH + content_height / 2;
  const int graph_h = SCREEN_HEIGHT - graph_y - 18;
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
  display_obj.tft.printf("AP: %.16s CH:%u", target_name_.c_str(), channel_);
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
  display_obj.tft.print("Top=Recalibrate Bottom=Stop");
  drawGraph();
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
  writeMetricRecord(now_ms);
}

void CsiMotionMonitor::main(uint32_t now_ms) {
  if (!active_)
    return;
  while (read_index_ != write_index_) {
    Sample& sample = queue_[read_index_];
    if (sample.target_index < target_count_) {
      writeRawRecord(sample);
      targets_[sample.target_index].model.addSample(
          sample.iq, sample.length, sample.rssi, sample.timestamp,
          sample.first_word_invalid);
    }
    read_index_ = static_cast<uint8_t>((read_index_ + 1) % kQueueSize);
  }
  updateFusion(now_ms);
  if (now_ms - last_draw_ms_ >= 500) {
    last_draw_ms_ = now_ms;
    draw(now_ms);
  }
}

#endif
