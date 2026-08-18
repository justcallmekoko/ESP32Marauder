#pragma once

#ifndef ReconMission_h
#define ReconMission_h

#include "configs.h"
#include "ReconMissionState.h"

#include <Arduino.h>
#ifdef HAS_SD
  #include <FS.h>
#endif

enum class ReconMode : uint8_t { WIFI_RECON, BLE_RECON };

class ReconMission {
 public:
  bool start(ReconMode mode);
  void stop();
  void main(uint32_t current_time);
  bool active() const { return running; }
  ReconMode mode() const { return active_mode; }

 private:
  void observeLists();
  void writeObservation(char type, const uint8_t mac[6], int rssi, uint8_t channel);
  void writeManifest(bool complete);
  void drawDashboard(uint32_t current_time);

  ReconMissionState state;
  bool running = false;
  ReconMode active_mode = ReconMode::WIFI_RECON;
  uint32_t started_at = 0;
  uint32_t last_sample = 0;
  uint32_t last_dashboard = 0;
  uint32_t ap_count = 0;
  uint32_t station_count = 0;
  uint32_t ble_count = 0;
  uint8_t pending_flush = 0;
  #ifdef HAS_SD
    File log_file;
    char session_dir[20] = {};
  #endif
};

#endif
