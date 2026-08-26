#include "ReconMission.h"

#include "Buffer.h"
#include "WiFiScan.h"
#ifdef HAS_SCREEN
  #include "Display.h"
#endif
#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif
#ifdef HAS_SD
  #include "SDInterface.h"
#endif

extern LinkedList<AccessPoint>* access_points;
extern LinkedList<Station>* stations;
extern LinkedList<BleDevice>* ble_devices;
extern WiFiScan wifi_scan_obj;
extern Buffer buffer_obj;
#ifdef HAS_SCREEN
  extern Display display_obj;
#endif
#ifdef HAS_GPS
  extern GpsInterface gps_obj;
#endif
#ifdef HAS_SD
  extern SDInterface sd_obj;
#endif

namespace {
struct __attribute__((packed)) ReconLogHeader {
  char magic[4] = {'R', 'C', 'N', '1'};
};

struct __attribute__((packed)) ReconLogRecord {
  uint32_t elapsed_ms;
  int32_t latitude;
  int32_t longitude;
  uint8_t mac[6];
  int8_t rssi;
  uint8_t channel;
  char type;
};

struct __attribute__((packed)) ReconProbeHeader {
  char magic[4] = {'P', 'R', 'B', '1'};
};

struct __attribute__((packed)) ReconProbeRecord {
  uint32_t elapsed_ms;
  int32_t latitude;
  int32_t longitude;
  uint8_t mac[6];
  int8_t rssi;
  uint8_t channel;
  uint8_t name_length;
  char name[RECON_PROBE_NAME_MAX];
};

struct __attribute__((packed)) ReconRelationshipHeader {
  char magic[4] = {'R', 'E', 'L', '1'};
};

struct __attribute__((packed)) ReconRelationshipRecord {
  uint8_t station[6];
  uint8_t access_point[6];
};

portMUX_TYPE probe_queue_mux = portMUX_INITIALIZER_UNLOCKED;
}  // namespace

bool ReconMission::start(ReconMode mode) {
  if (running || wifi_scan_obj.scanning()) return false;
  active_mode = mode;
  started_at = millis();
  last_sample = 0;
  last_dashboard = 0;
  last_prune = started_at;
  last_deauth = 0;
  pending_flush = 0;
  probe_pending_flush = 0;
  ap_count = 0;
  station_count = 0;
  ble_count = 0;
  probe_count = 0;
  repeat_count = 0;
  deauth_count = 0;
  probe_queue.reset();
  repeat_queue.reset();
  deauth_queue.reset();
  repeat_gate.reset();
  memset(ui_events, 0, sizeof(ui_events));
  ui_event_head = 0;
  memset(ui_relationships, 0, sizeof(ui_relationships));
  ui_relationship_head = 0;
  memset(signal_history, 0, sizeof(signal_history));
  signal_history_count = 0;
  memset(channel_activity, 0, sizeof(channel_activity));
  suppress_scan_ui = true;

  buffer_obj.setDirectory(NULL);
  #ifdef HAS_SD
    session_dir[0] = '\0';
    if (sd_obj.supported) {
      SD.mkdir("/recon");
      for (uint16_t index = 0; index < 10000; index++) {
        snprintf(session_dir, sizeof(session_dir), "/recon/m%04u", index);
        if (!SD.exists(session_dir) && SD.mkdir(session_dir)) break;
        session_dir[0] = '\0';
      }
      if (session_dir[0]) buffer_obj.setDirectory(session_dir);
    }
  #endif

  wifi_scan_obj.StartScan(active_mode == ReconMode::WIFI_RECON ? WIFI_SCAN_AP_STA : BT_SCAN_ALL,
                          active_mode == ReconMode::WIFI_RECON ? TFT_MAGENTA : TFT_CYAN);
  state.reset();
  if (active_mode == ReconMode::WIFI_RECON) {
    state.consume(ReconSource::AP_LIST, access_points ? access_points->size() : 0);
    state.consume(ReconSource::STATION_LIST, stations ? stations->size() : 0);
  } else {
    state.consume(ReconSource::BLE_LIST, ble_devices ? ble_devices->size() : 0);
  }

  #ifdef HAS_SD
    if (session_dir[0]) {
      char file_name[36];
      snprintf(file_name, sizeof(file_name), "%s/obs.rlog", session_dir);
      log_file = SD.open(file_name, FILE_WRITE);
      if (log_file) {
        const ReconLogHeader header;
        log_file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
      }
      snprintf(file_name, sizeof(file_name), "%s/probes.rlog", session_dir);
      probe_file = SD.open(file_name, FILE_WRITE);
      if (probe_file) {
        const ReconProbeHeader header;
        probe_file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
      }
      snprintf(file_name, sizeof(file_name), "%s/relations.rlog", session_dir);
      relationship_file = SD.open(file_name, FILE_WRITE);
      if (relationship_file) {
        const ReconRelationshipHeader header;
        relationship_file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
      }
    }
  #endif

  running = true;
  writeManifest(false);
  drawDashboard(started_at);
  return true;
}

void ReconMission::stop() {
  if (!running) return;
  observeLists();
  drainProbeQueue();
  drainRepeatQueue();
  drainDeauthQueue();
  #ifdef HAS_SD
    if (log_file) {
      log_file.flush();
      log_file.close();
    }
    if (probe_file) {
      probe_file.flush();
      probe_file.close();
    }
    if (relationship_file) {
      relationship_file.flush();
      relationship_file.close();
    }
  #endif
  writeManifest(true);
  buffer_obj.setDirectory(NULL);
  running = false;
  suppress_scan_ui = false;
}

void ReconMission::writeRelationship(const uint8_t station[6],
                                      const uint8_t access_point[6]) {
  #ifdef HAS_SD
    if (!relationship_file) return;
    ReconRelationshipRecord record;
    memcpy(record.station, station, sizeof(record.station));
    memcpy(record.access_point, access_point, sizeof(record.access_point));
    relationship_file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record));
  #else
    (void)station;
    (void)access_point;
  #endif
}

void ReconMission::recordRelationship(const Station& station,
                                      const AccessPoint& access_point) {
  UiRelationship& relationship = ui_relationships[ui_relationship_head++ % 3];
  memcpy(relationship.station, station.mac, sizeof(relationship.station));
  memcpy(relationship.access_point, access_point.bssid, sizeof(relationship.access_point));
  const String& name = access_point.essid;
  if (name.length()) {
    name.substring(0, sizeof(relationship.ap_name) - 1)
        .toCharArray(relationship.ap_name, sizeof(relationship.ap_name));
  } else {
    snprintf(relationship.ap_name, sizeof(relationship.ap_name), "%02X:%02X:%02X",
             access_point.bssid[3], access_point.bssid[4], access_point.bssid[5]);
  }
}

void ReconMission::rebuildStationLinks() {
  if (!access_points || !stations) return;
  for (int ap_index = 0; ap_index < access_points->size(); ap_index++) {
    LinkedList<uint16_t>* links = access_points->get(ap_index).stations;
    if (links) links->clear();
  }
  for (int station_index = 0; station_index < stations->size(); station_index++) {
    Station station = stations->get(station_index);
    if (station.ap >= access_points->size()) continue;
    LinkedList<uint16_t>* links = access_points->get(station.ap).stations;
    if (links) links->add(static_cast<uint16_t>(station_index));
  }
}

void ReconMission::pruneStaleDevices(uint32_t current_time) {
  if (current_time - last_prune < 15000) return;
  last_prune = current_time;

  if (stations) {
    for (int index = stations->size() - 1; index >= 0; index--) {
      if (reconDeviceExpired(current_time, stations->get(index).last_seen_ms))
        stations->remove(index);
    }
  }

  if (access_points) {
    for (int ap_index = access_points->size() - 1; ap_index >= 0; ap_index--) {
      if (!reconDeviceExpired(current_time, access_points->get(ap_index).last_seen_ms)) continue;
      if (stations) {
        for (int station_index = stations->size() - 1; station_index >= 0; station_index--) {
          Station station = stations->get(station_index);
          if (station.ap == ap_index) stations->remove(station_index);
          else if (station.ap > ap_index) {
            station.ap--;
            stations->set(station_index, station);
          }
        }
      }
      LinkedList<uint16_t>* links = access_points->get(ap_index).stations;
      if (links) delete links;
      access_points->remove(ap_index);
    }
  }

  #ifdef HAS_BT
    if (ble_devices) {
      for (int index = ble_devices->size() - 1; index >= 0; index--) {
        if (reconDeviceExpired(current_time, ble_devices->get(index).last_seen_ms))
          ble_devices->remove(index);
      }
    }
  #endif

  rebuildStationLinks();
}

void ReconMission::writeObservation(char type, const uint8_t mac[6], int rssi,
                                    uint8_t channel) {
  if (type == 'a') ap_count++;
  else if (type == 's') station_count++;
  else if (type == 'b') ble_count++;
  else if (type != 'd') repeat_count++;
  recordUiEvent(type, mac, static_cast<int8_t>(rssi));
  recordSignal(static_cast<int8_t>(rssi), channel);

  ReconLogRecord record = {};
  record.elapsed_ms = millis() - started_at;
  record.rssi = static_cast<int8_t>(rssi);
  record.channel = channel;
  record.type = type;
  memcpy(record.mac, mac, sizeof(record.mac));
  #ifdef HAS_GPS
    if (gps_obj.getFixStatus()) {
      record.latitude = gps_obj.getLatInt();
      record.longitude = gps_obj.getLonInt();
    }
  #endif

  #ifdef HAS_SD
    if (log_file) {
      log_file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record));
      if (++pending_flush >= 16) {
        log_file.flush();
        pending_flush = 0;
      }
    }
  #endif
}

void ReconMission::queueProbe(const uint8_t mac[6], int8_t rssi, uint8_t channel,
                              const uint8_t* name, uint8_t name_length) {
  if (!running || active_mode != ReconMode::WIFI_RECON || !name || !name_length) return;
  ReconProbeEvent event = {};
  event.elapsed_ms = millis() - started_at;
  memcpy(event.mac, mac, sizeof(event.mac));
  event.rssi = rssi;
  event.channel = channel;
  event.name_length = min(name_length, static_cast<uint8_t>(RECON_PROBE_NAME_MAX));
  memcpy(event.name, name, event.name_length);
  portENTER_CRITICAL(&probe_queue_mux);
  probe_queue.push(event);
  portEXIT_CRITICAL(&probe_queue_mux);
}

void ReconMission::writeProbe(const ReconProbeEvent& event) {
  probe_count++;
  char label[13] = {};
  memcpy(label, event.name, min(event.name_length, static_cast<uint8_t>(sizeof(label) - 1)));
  recordUiEvent('p', event.mac, event.rssi, label);
  recordSignal(event.rssi, event.channel);
  #ifdef HAS_SD
    if (!probe_file) return;
    ReconProbeRecord record = {};
    record.elapsed_ms = event.elapsed_ms;
    memcpy(record.mac, event.mac, sizeof(record.mac));
    record.rssi = event.rssi;
    record.channel = event.channel;
    record.name_length = event.name_length;
    memcpy(record.name, event.name, event.name_length);
    #ifdef HAS_GPS
      if (gps_obj.getFixStatus()) {
        record.latitude = gps_obj.getLatInt();
        record.longitude = gps_obj.getLonInt();
      }
    #endif
    probe_file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record));
    if (++probe_pending_flush >= 16) {
      probe_file.flush();
      probe_pending_flush = 0;
    }
  #endif
}

void ReconMission::recordSignal(int8_t rssi, uint8_t channel) {
  if (rssi > -127) {
    if (signal_history_count < sizeof(signal_history)) {
      signal_history[signal_history_count++] = rssi;
    } else {
      memmove(signal_history, signal_history + 1, sizeof(signal_history) - 1);
      signal_history[sizeof(signal_history) - 1] = rssi;
    }
  }
  if (!channel) return;
  uint8_t channel_index = UINT8_MAX;
  #ifdef HAS_DUAL_BAND
    for (uint8_t index = 0; index < DUAL_BAND_CHANNELS; index++) {
      if (wifi_scan_obj.dual_band_channels[index] == channel) {
        channel_index = index;
        break;
      }
    }
  #else
    if (channel <= MAX_CHANNEL) channel_index = channel - 1;
  #endif
  if (channel_index != UINT8_MAX && channel_activity[channel_index] < UINT16_MAX)
    channel_activity[channel_index]++;
}

void ReconMission::queueDeauth(const uint8_t transmitter[6], const uint8_t bssid[6],
                               int8_t rssi, uint8_t channel, uint16_t reason) {
  if (!running || active_mode != ReconMode::WIFI_RECON) return;
  ReconDeauthEvent event = {};
  memcpy(event.transmitter, transmitter, sizeof(event.transmitter));
  memcpy(event.bssid, bssid, sizeof(event.bssid));
  event.rssi = rssi;
  event.channel = channel;
  event.reason = reason;
  portENTER_CRITICAL(&probe_queue_mux);
  deauth_queue.push(event);
  portEXIT_CRITICAL(&probe_queue_mux);
}

void ReconMission::recordUiEvent(char type, const uint8_t mac[6], int8_t rssi,
                                 const char* label) {
  UiEvent& event = ui_events[ui_event_head++ % 4];
  memcpy(event.mac, mac, sizeof(event.mac));
  event.type = type;
  event.rssi = rssi;
  event.label[0] = '\0';
  if (label) {
    strncpy(event.label, label, sizeof(event.label) - 1);
    event.label[sizeof(event.label) - 1] = '\0';
  }
}

void ReconMission::queueRepeat(char type, const uint8_t mac[6], int8_t rssi,
                               uint8_t channel) {
  if (!running || (type == 'B') != (active_mode == ReconMode::BLE_RECON)) return;
  portENTER_CRITICAL(&probe_queue_mux);
  if (repeat_gate.accept(mac, rssi, millis())) {
    ReconRepeatEvent event = {};
    memcpy(event.mac, mac, sizeof(event.mac));
    event.rssi = rssi;
    event.channel = channel;
    event.type = type;
    repeat_queue.push(event);
  }
  portEXIT_CRITICAL(&probe_queue_mux);
}

void ReconMission::drainProbeQueue() {
  ReconProbeEvent event;
  while (true) {
    portENTER_CRITICAL(&probe_queue_mux);
    const bool available = probe_queue.pop(event);
    portEXIT_CRITICAL(&probe_queue_mux);
    if (!available) break;
    writeProbe(event);
  }
}

void ReconMission::drainRepeatQueue() {
  ReconRepeatEvent event;
  while (true) {
    portENTER_CRITICAL(&probe_queue_mux);
    const bool available = repeat_queue.pop(event);
    portEXIT_CRITICAL(&probe_queue_mux);
    if (!available) break;
    writeObservation(event.type, event.mac, event.rssi, event.channel);
  }
}

void ReconMission::drainDeauthQueue() {
  ReconDeauthEvent event;
  while (true) {
    portENTER_CRITICAL(&probe_queue_mux);
    const bool available = deauth_queue.pop(event);
    portEXIT_CRITICAL(&probe_queue_mux);
    if (!available) break;
    deauth_count++;
    last_deauth = millis();
    writeObservation('d', event.transmitter, event.rssi, event.channel);
  }
}

void ReconMission::writeManifest(bool complete) {
  #ifdef HAS_SD
    if (!session_dir[0]) return;
    char path[40];
    snprintf(path, sizeof(path), "%s/session.json", session_dir);
    SD.remove(path);
    File manifest = SD.open(path, FILE_WRITE);
    if (!manifest) return;
    bool gps_fix = false;
    #ifdef HAS_GPS
      gps_fix = gps_obj.getFixStatus();
    #endif
    const String capture = active_mode == ReconMode::WIFI_RECON
                             ? buffer_obj.getFileName() : "";
    manifest.printf(
      "{\"schema\":1,\"state\":\"%s\",\"mode\":\"%s\",\"start_ms\":%lu,"
      "\"duration_ms\":%lu,\"ap\":%lu,\"station\":%lu,\"ble\":%lu,"
      "\"probe\":%lu,\"repeat\":%lu,\"deauth\":%lu,\"dropped\":%u,\"gps_fix\":%s,"
      "\"observations\":\"obs.rlog\",\"probes\":\"probes.rlog\","
      "\"relationships\":\"relations.rlog\",\"capture\":\"%s\"}\n",
      complete ? "complete" : "active",
      active_mode == ReconMode::WIFI_RECON ? "wifi" : "ble",
      static_cast<unsigned long>(started_at),
      static_cast<unsigned long>(complete ? millis() - started_at : 0),
      static_cast<unsigned long>(ap_count),
      static_cast<unsigned long>(station_count),
      static_cast<unsigned long>(ble_count),
      static_cast<unsigned long>(probe_count),
      static_cast<unsigned long>(repeat_count),
      static_cast<unsigned long>(deauth_count),
      probe_queue.dropped() + repeat_queue.dropped() + deauth_queue.dropped(),
      gps_fix ? "true" : "false", capture.c_str());
    manifest.close();
  #else
    (void)complete;
  #endif
}

void ReconMission::drawDashboard(uint32_t current_time) {
  #ifdef HAS_SCREEN
    if (last_dashboard && current_time - last_dashboard < 1000) return;
    last_dashboard = current_time;
    const uint32_t seconds = (current_time - started_at) / 1000;
    bool gps_fix = false;
    #ifdef HAS_GPS
      gps_fix = gps_obj.getFixStatus();
    #endif
    char status[40];
    if (active_mode == ReconMode::WIFI_RECON) {
      #if TFT_WIDTH < 200
        snprintf(status, sizeof(status), "R W %lu:%02lu A%lu S%lu P%lu",
                 static_cast<unsigned long>(seconds / 60),
                 static_cast<unsigned long>(seconds % 60),
                 static_cast<unsigned long>(ap_count),
                 static_cast<unsigned long>(station_count),
                 static_cast<unsigned long>(probe_count));
      #else
        snprintf(status, sizeof(status), "REC W %lu:%02lu A%lu S%lu P%lu G%c",
                 static_cast<unsigned long>(seconds / 60),
                 static_cast<unsigned long>(seconds % 60),
                 static_cast<unsigned long>(ap_count),
                 static_cast<unsigned long>(station_count),
                 static_cast<unsigned long>(probe_count), gps_fix ? '+' : '-');
      #endif
    } else {
      snprintf(status, sizeof(status), "REC B %lu:%02lu D%lu U%lu G%c",
               static_cast<unsigned long>(seconds / 60),
               static_cast<unsigned long>(seconds % 60),
               static_cast<unsigned long>(ble_count),
               static_cast<unsigned long>(repeat_count), gps_fix ? '+' : '-');
    }
    const uint16_t color = active_mode == ReconMode::WIFI_RECON ? TFT_MAGENTA : TFT_CYAN;
    display_obj.tft.fillRect(0, 16, TFT_WIDTH, 16, color);
    display_obj.tft.fillCircle(7, 24, 3, TFT_RED);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    display_obj.tft.drawCentreString(status, (TFT_WIDTH / 2) + 4, 20, 1);
    while (display_obj.display_buffer->size()) display_obj.display_buffer->shift();
    #ifdef SCREEN_BUFFER
      while (display_obj.screen_buffer->size()) display_obj.screen_buffer->shift();
    #endif

    const int16_t body_top = 48;
    display_obj.tft.fillRect(0, body_top, TFT_WIDTH, TFT_HEIGHT - body_top, TFT_BLACK);
    #if TFT_HEIGHT < 160
      const int16_t beacon_x = 7;
      const int16_t beacon_y = body_top + 7;
      const int16_t beacon_width = 50;
      const int16_t beacon_height = 62;
    #elif TFT_WIDTH < 200
      const int16_t beacon_x = (TFT_WIDTH - 84) / 2;
      const int16_t beacon_y = body_top + 7;
      const int16_t beacon_width = 84;
      const int16_t beacon_height = 82;
    #else
      const int16_t beacon_x = 8;
      const int16_t beacon_y = body_top + 7;
      const int16_t beacon_width = 92;
      const int16_t beacon_height = 88;
    #endif
    const int8_t latest_rssi = signal_history_count
                                 ? signal_history[signal_history_count - 1] : -128;
    const uint8_t lit_segments = reconSignalSegments(latest_rssi);
    const ReconSignalTrend trend = reconSignalTrend(signal_history, signal_history_count);
    const uint16_t signal_color = lit_segments >= 4 ? TFT_GREEN :
                                  lit_segments >= 2 ? TFT_YELLOW : TFT_ORANGE;
    display_obj.tft.drawRect(beacon_x, beacon_y, beacon_width, beacon_height, TFT_DARKGREY);
    display_obj.tft.fillCircle(beacon_x + beacon_width / 2, beacon_y + 10, 3,
                              lit_segments ? signal_color : TFT_DARKGREY);
    for (uint8_t segment = 0; segment < 5; segment++) {
      const int16_t width = 12 + segment * 11;
      const int16_t x = beacon_x + (beacon_width - width) / 2;
      const int16_t y = beacon_y + 18 + segment * 8;
      const uint16_t segment_color = segment < lit_segments ? signal_color : TFT_DARKGREY;
      display_obj.tft.fillRect(x, y, width, 4, segment_color);
    }
    display_obj.tft.setTextColor(signal_color, TFT_BLACK);
    display_obj.tft.drawCentreString(reconProximityLabel(latest_rssi),
                                     beacon_x + beacon_width / 2,
                                     beacon_y + beacon_height - 15, 1);
    const char* trend_label = trend == ReconSignalTrend::APPROACHING ? "CLOSING +" :
                              trend == ReconSignalTrend::DEPARTING ? "LEAVING -" : "STEADY";
    display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    display_obj.tft.drawCentreString(trend_label, beacon_x + beacon_width / 2,
                                     beacon_y + beacon_height + 2, 1);

    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char line[34];
    #if TFT_HEIGHT < 160
      const int16_t stats_x = 64;
      if (active_mode == ReconMode::WIFI_RECON)
        snprintf(line, sizeof(line), "AP %-4lu STA %-4lu", (unsigned long)ap_count,
                 (unsigned long)station_count);
      else
        snprintf(line, sizeof(line), "BLE %-5lu", (unsigned long)ble_count);
      display_obj.tft.drawString(line, stats_x, body_top + 8, 1);
      if (active_mode == ReconMode::WIFI_RECON)
        snprintf(line, sizeof(line), "PROBE %-3lu UPD %-3lu", (unsigned long)probe_count,
                 (unsigned long)repeat_count);
      else
        snprintf(line, sizeof(line), "UPDATE %-3lu", (unsigned long)repeat_count);
      display_obj.tft.drawString(line, stats_x, body_top + 22, 1);
      if (active_mode == ReconMode::WIFI_RECON) {
        const bool deauth_active = last_deauth && current_time - last_deauth < RECON_DEAUTH_ALERT_MS;
        snprintf(line, sizeof(line), "D! %-4lu", (unsigned long)deauth_count);
        display_obj.tft.setTextColor(deauth_active ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
        display_obj.tft.drawString(line, stats_x, body_top + 36, 1);
      }
    #elif TFT_WIDTH < 200
      if (active_mode == ReconMode::WIFI_RECON)
        snprintf(line, sizeof(line), "AP %lu  STA %lu", (unsigned long)ap_count,
                 (unsigned long)station_count);
      else
        snprintf(line, sizeof(line), "BLE DEVICES %lu", (unsigned long)ble_count);
      display_obj.tft.drawCentreString(line, TFT_WIDTH / 2, body_top + 96, 1);
      if (active_mode == ReconMode::WIFI_RECON)
        snprintf(line, sizeof(line), "PROBE %lu  UPDATE %lu", (unsigned long)probe_count,
                 (unsigned long)repeat_count);
      else
        snprintf(line, sizeof(line), "UPDATE %lu", (unsigned long)repeat_count);
      display_obj.tft.drawCentreString(line, TFT_WIDTH / 2, body_top + 110, 1);
      if (active_mode == ReconMode::WIFI_RECON) {
        const bool deauth_active = last_deauth && current_time - last_deauth < RECON_DEAUTH_ALERT_MS;
        snprintf(line, sizeof(line), "DEAUTH %lu", (unsigned long)deauth_count);
        display_obj.tft.setTextColor(deauth_active ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
        display_obj.tft.drawCentreString(line, TFT_WIDTH / 2, body_top + 124, 1);
      }
    #else
      const int16_t stats_x = 108;
      display_obj.tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
      display_obj.tft.drawString("LIVE ACTIVITY", stats_x, body_top + 9, 1);
      display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
      if (active_mode == ReconMode::WIFI_RECON) {
        snprintf(line, sizeof(line), "AP       %lu", (unsigned long)ap_count);
        display_obj.tft.drawString(line, stats_x, body_top + 28, 1);
        snprintf(line, sizeof(line), "STATION  %lu", (unsigned long)station_count);
        display_obj.tft.drawString(line, stats_x, body_top + 42, 1);
        snprintf(line, sizeof(line), "PROBE    %lu", (unsigned long)probe_count);
        display_obj.tft.drawString(line, stats_x, body_top + 56, 1);
        snprintf(line, sizeof(line), "UPDATE   %lu", (unsigned long)repeat_count);
        display_obj.tft.drawString(line, stats_x, body_top + 70, 1);
        const bool deauth_active = last_deauth && current_time - last_deauth < RECON_DEAUTH_ALERT_MS;
        snprintf(line, sizeof(line), "DEAUTH   %lu", (unsigned long)deauth_count);
        display_obj.tft.setTextColor(deauth_active ? TFT_RED : TFT_DARKGREY, TFT_BLACK);
        display_obj.tft.drawString(line, stats_x, body_top + 84, 1);
      } else {
        snprintf(line, sizeof(line), "BLE DEVICE  %lu", (unsigned long)ble_count);
        display_obj.tft.drawString(line, stats_x, body_top + 35, 1);
        snprintf(line, sizeof(line), "UPDATE      %lu", (unsigned long)repeat_count);
        display_obj.tft.drawString(line, stats_x, body_top + 55, 1);
      }
    #endif

    #if TFT_HEIGHT >= 160
      #if TFT_WIDTH >= 200
        const int16_t relation_y = body_top + 112;
        display_obj.tft.drawFastHLine(6, relation_y - 6, TFT_WIDTH - 12, TFT_DARKGREY);
        display_obj.tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
        display_obj.tft.drawString("NEW ASSOCIATIONS", 8, relation_y, 1);
        for (uint8_t row = 0; row < 3; row++) {
          const UiRelationship& relationship =
              ui_relationships[(ui_relationship_head + row) % 3];
          if (!relationship.ap_name[0]) continue;
          char ap_name[13];
          reconTruncate(relationship.ap_name, ap_name, sizeof(ap_name));
          snprintf(line, sizeof(line), "%02X:%02X:%02X > %s",
                   relationship.station[3], relationship.station[4], relationship.station[5],
                   ap_name);
          display_obj.tft.setTextColor(row == 2 ? TFT_CYAN : TFT_LIGHTGREY, TFT_BLACK);
          display_obj.tft.drawString(line, 8, relation_y + 15 + row * 14, 1);
        }

        if (active_mode == ReconMode::WIFI_RECON) {
          #ifdef HAS_DUAL_BAND
            const uint8_t channel_count = DUAL_BAND_CHANNELS;
          #else
            const uint8_t channel_count = MAX_CHANNEL;
          #endif
          const uint8_t page = reconChannelPage(current_time - started_at, channel_count);
          const uint8_t first_channel = page * RECON_CHANNELS_PER_PAGE;
          const uint8_t visible_channels = reconChannelsOnPage(page, channel_count);
          uint16_t peak = 1;
          for (uint8_t offset = 0; offset < visible_channels; offset++)
            if (channel_activity[first_channel + offset] > peak)
              peak = channel_activity[first_channel + offset];
          const int16_t strip_y = TFT_HEIGHT - 31;
          display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
          display_obj.tft.drawString("CH", 3, strip_y + 7, 1);
          const int16_t chart_left = 22;
          const int16_t slot = (TFT_WIDTH - chart_left - 2) / visible_channels;
          for (uint8_t offset = 0; offset < visible_channels; offset++) {
            const uint8_t index = first_channel + offset;
            const int16_t height = 2 + (channel_activity[index] * 12UL) / peak;
            const int16_t x = chart_left + offset * slot;
            display_obj.tft.fillRect(x + 1, strip_y + 16 - height,
                                     slot > 3 ? slot - 3 : 1, height,
                                     channel_activity[index] ? TFT_BLUE : TFT_DARKGREY);
            #ifdef HAS_DUAL_BAND
              const uint8_t channel = wifi_scan_obj.dual_band_channels[index];
            #else
              const uint8_t channel = index + 1;
            #endif
            char channel_label[4];
            snprintf(channel_label, sizeof(channel_label), "%u", channel);
            display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            display_obj.tft.drawCentreString(channel_label, x + slot / 2, strip_y + 19, 1);
          }
        }
      #else
        const int16_t event_y = TFT_HEIGHT - 32;
        display_obj.tft.drawFastHLine(5, event_y - 5, TFT_WIDTH - 10, TFT_DARKGREY);
        if (active_mode == ReconMode::WIFI_RECON && ui_relationship_head) {
          const UiRelationship& relationship =
              ui_relationships[(ui_relationship_head + 2) % 3];
          char ap_name[9];
          reconTruncate(relationship.ap_name, ap_name, sizeof(ap_name));
          snprintf(line, sizeof(line), "%02X:%02X > %s", relationship.station[4],
                   relationship.station[5], ap_name);
          display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
          display_obj.tft.drawCentreString(line, TFT_WIDTH / 2, event_y, 1);
        } else {
          const UiEvent& latest = ui_events[(ui_event_head + 3) % 4];
          if (latest.type == 'p') snprintf(line, sizeof(line), "PROBE %.12s", latest.label);
          else if (latest.type) snprintf(line, sizeof(line), "%c %02X:%02X:%02X",
                                         latest.type, latest.mac[3], latest.mac[4], latest.mac[5]);
          else snprintf(line, sizeof(line), "Waiting...");
          display_obj.tft.setTextColor(latest.type == 'd' ? TFT_RED : TFT_LIGHTGREY, TFT_BLACK);
          display_obj.tft.drawCentreString(line, TFT_WIDTH / 2, event_y, 1);
        }
      #endif
    #endif
    display_obj.tft.setTextColor(active_mode == ReconMode::WIFI_RECON ? TFT_GREEN : TFT_CYAN,
                                 TFT_BLACK);
  #else
    (void)current_time;
  #endif
}

void ReconMission::observeLists() {
  if (!running) return;
  if (active_mode == ReconMode::WIFI_RECON) {
    if (access_points) {
      const ReconRange range = state.consume(ReconSource::AP_LIST, access_points->size());
      for (size_t index = range.begin; index < range.end; index++) {
        const AccessPoint& ap = access_points->get(index);
        writeObservation('a', ap.bssid, ap.rssi, ap.channel);
      }
    }
    if (stations) {
      const ReconRange range = state.consume(ReconSource::STATION_LIST, stations->size());
      for (size_t index = range.begin; index < range.end; index++) {
        const Station& station = stations->get(index);
        uint8_t channel = 0;
        if (access_points && station.ap < access_points->size()) {
          const AccessPoint& ap = access_points->get(station.ap);
          channel = ap.channel;
          writeRelationship(station.mac, ap.bssid);
          recordRelationship(station, ap);
        }
        writeObservation('s', station.mac, -128, channel);
      }
    }
  } else {
    #ifdef HAS_BT
      if (ble_devices) {
        const ReconRange range = state.consume(ReconSource::BLE_LIST, ble_devices->size());
        for (size_t index = range.begin; index < range.end; index++) {
          const BleDevice& device = ble_devices->get(index);
          writeObservation('b', device.mac, device.rssi, 0);
        }
      }
    #endif
  }
}

void ReconMission::main(uint32_t current_time) {
  if (!running) return;
  if (!wifi_scan_obj.scanning()) {
    stop();
    return;
  }
  if (current_time - last_sample < 500) return;
  last_sample = current_time;
  observeLists();
  drainProbeQueue();
  drainRepeatQueue();
  drainDeauthQueue();
  pruneStaleDevices(current_time);
  drawDashboard(current_time);
}
