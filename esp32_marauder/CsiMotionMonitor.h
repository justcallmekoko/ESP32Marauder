#pragma once

#include "configs.h"

#if defined(MARAUDER_V8)

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

 private:
  static constexpr uint8_t kQueueSize = 64;
  static constexpr uint16_t kMaxCsiBytes = 256;
  struct Sample {
    uint16_t length;
    int8_t rssi;
    uint32_t timestamp;
    int8_t iq[kMaxCsiBytes];
  };

  static void callback(void* context, wifi_csi_info_t* info);
  void enqueue(wifi_csi_info_t* info);
  bool takeWiFi();
  void releaseWiFi();
  void draw(uint32_t now_ms);
  void drawGraph();
  const char* stateLabel() const;

  Sample queue_[kQueueSize] = {};
  volatile uint8_t write_index_ = 0;
  volatile uint8_t read_index_ = 0;
  volatile uint32_t accepted_ = 0;
  volatile uint32_t dropped_ = 0;
  uint8_t target_[6] = {};
  String target_name_;
  uint8_t channel_ = 1;
  bool active_ = false;
  bool wifi_owned_ = false;
  uint32_t last_draw_ms_ = 0;
  CsiMotionModel model_;
};

extern CsiMotionMonitor csi_motion_obj;

#endif
