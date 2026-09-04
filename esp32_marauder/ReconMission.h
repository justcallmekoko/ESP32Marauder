#pragma once

#ifndef ReconMission_h
#define ReconMission_h

#include "configs.h"
#include "ReconProbeQueue.h"
#include "ReconMissionState.h"
#include "ReconUi.h"

#include <Arduino.h>
#ifdef HAS_SD
  #include <FS.h>
#endif

enum class ReconMode : uint8_t { WIFI_RECON, BLE_RECON };

struct Station;
struct AccessPoint;

class ReconMission {
 public:
  bool start(ReconMode mode);
  void stop();
  void main(uint32_t current_time);
  void queueProbe(const uint8_t mac[6], int8_t rssi, uint8_t channel,
                  const uint8_t* name, uint8_t name_length);
  void queueRepeat(char type, const uint8_t mac[6], int8_t rssi, uint8_t channel);
  void queueDeauth(const uint8_t transmitter[6], const uint8_t bssid[6],
                   int8_t rssi, uint8_t channel, uint16_t reason);
  bool active() const { return running; }
  bool suppressScanUi() const { return suppress_scan_ui; }
  ReconMode mode() const { return active_mode; }

 private:
  void observeLists();
  void drainProbeQueue();
  void drainRepeatQueue();
  void drainDeauthQueue();
  void writeObservation(char type, const uint8_t mac[6], int rssi, uint8_t channel,
                        const char* label = nullptr);
  void writeProbe(const ReconProbeEvent& event);
  void writeRelationship(const uint8_t station[6], const uint8_t access_point[6]);
  void recordRelationship(const Station& station, const AccessPoint& access_point);
  void recordSignal(int8_t rssi, uint8_t channel);
  void pruneStaleDevices(uint32_t current_time);
  void rebuildStationLinks();
  void writeManifest(bool complete);
  void drawDashboard(uint32_t current_time);
  void recordUiEvent(char type, const uint8_t mac[6], int8_t rssi,
                     const char* label = nullptr);

  ReconMissionState state;
  bool running = false;
  bool suppress_scan_ui = false;
  ReconMode active_mode = ReconMode::WIFI_RECON;
  uint32_t started_at = 0;
  uint32_t last_sample = 0;
  uint32_t last_dashboard = 0;
  uint32_t last_prune = 0;
  uint32_t last_deauth = 0;
  uint32_t ap_count = 0;
  uint32_t station_count = 0;
  uint32_t ble_count = 0;
  uint32_t probe_count = 0;
  uint32_t repeat_count = 0;
  uint32_t deauth_count = 0;
  uint8_t pending_flush = 0;
  uint8_t probe_pending_flush = 0;
  ReconProbeQueue probe_queue;
  ReconRepeatQueue repeat_queue;
  ReconDeauthQueue deauth_queue;
  ReconRepeatGate repeat_gate;
  struct UiEvent {
    uint8_t mac[6];
    char label[13];
    char type;
    int8_t rssi;
  } ui_events[4] = {};
  uint8_t ui_event_head = 0;
  struct UiRelationship {
    uint8_t station[6];
    uint8_t access_point[6];
    char ap_name[17];
  } ui_relationships[3] = {};
  uint8_t ui_relationship_head = 0;
  uint8_t churn_in_history[64] = {};
  uint8_t churn_out_history[64] = {};
  uint8_t churn_history_count = 0;
  uint16_t pending_churn_in = 0;
  uint16_t pending_churn_out = 0;
  uint32_t band_24_observations = 0;
  uint32_t band_5_observations = 0;
  uint16_t channel_activity[DUAL_BAND_CHANNELS] = {};
  #ifdef HAS_SD
    File log_file;
    File probe_file;
    File relationship_file;
    char session_dir[20] = {};
  #endif
};

#endif
