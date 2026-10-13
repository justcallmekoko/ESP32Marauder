#pragma once

#include "configs.h"

#if defined(HAS_CSI_MOTION)

#include <Arduino.h>
#include "esp_wifi.h"
#include "CsiMotionModel.h"

class CsiMotionMonitor {
 public:
  bool start(const uint8_t target[6], const String& name, uint8_t channel);
  void stop();
  void main(uint32_t now_ms);
  bool active() const { return active_; }
  void recalibrate();
  void printStatus(bool force = true);

 private:
  static constexpr uint8_t kQueueSize = 64;
  static constexpr uint16_t kMaxCsiBytes = 256;
  struct Sample {
    uint16_t length;
    uint16_t target_index;
    int8_t rssi;
    bool first_word_invalid;
    uint32_t timestamp;
    int8_t iq[kMaxCsiBytes];
  };

  struct Target {
    uint8_t bssid[6];
    volatile uint32_t accepted;
    volatile uint32_t dropped;
    CsiMotionModel model;
  };

  static void callback(void* context, wifi_csi_info_t* info);
  void enqueue(wifi_csi_info_t* info);
  bool takeWiFi();
  void releaseWiFi();
  void draw(uint32_t now_ms);
  void drawGraph();
  const char* stateLabel() const;
  void updateFusion(uint32_t now_ms);
  int findTarget(const uint8_t mac[6]) const;
  void releaseTargets();

  Sample queue_[kQueueSize] = {};
  volatile uint8_t write_index_ = 0;
  volatile uint8_t read_index_ = 0;
  volatile uint32_t accepted_ = 0;
  volatile uint32_t dropped_ = 0;
  Target* targets_ = nullptr;
  uint16_t target_count_ = 0;
  uint16_t active_target_count_ = 0;
  uint8_t target_[6] = {};
  String target_name_;
  uint8_t channel_ = 1;
  bool active_ = false;
  bool wifi_owned_ = false;
  uint32_t last_draw_ms_ = 0;
  uint32_t started_ms_ = 0;
  uint32_t last_fusion_ms_ = 0;
  uint32_t last_serial_ms_ = 0;
  uint8_t motion_score_ = 0;
  uint8_t amplitude_score_ = 0;
  uint8_t phase_score_ = 0;
  uint8_t correlation_score_ = 0;
  uint8_t reliability_ = 0;
  uint16_t samples_per_second_ = 0;
  uint8_t motion_votes_ = 0;
  uint8_t still_votes_ = 0;
  CsiMotionModel::State state_ = CsiMotionModel::State::CALIBRATING;
  uint8_t history_[CsiMotionModel::kHistorySize] = {};
  size_t history_head_ = 0;
};

extern CsiMotionMonitor csi_motion_obj;

#endif
