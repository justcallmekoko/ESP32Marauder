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
  pending_flush = 0;
  probe_pending_flush = 0;
  ap_count = 0;
  station_count = 0;
  ble_count = 0;
  probe_count = 0;
  repeat_count = 0;
  probe_queue.reset();
  repeat_queue.reset();
  repeat_gate.reset();
  memset(ui_events, 0, sizeof(ui_events));
  ui_event_head = 0;

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

void ReconMission::writeObservation(char type, const uint8_t mac[6], int rssi,
                                    uint8_t channel) {
  if (type == 'a') ap_count++;
  else if (type == 's') station_count++;
  else if (type == 'b') ble_count++;
  else repeat_count++;
  recordUiEvent(type, mac, static_cast<int8_t>(rssi));

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
      "\"probe\":%lu,\"repeat\":%lu,\"dropped\":%u,\"gps_fix\":%s,"
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
      probe_queue.dropped() + repeat_queue.dropped(),
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
        snprintf(status, sizeof(status), "REC W %lu:%02lu A%lu S%lu P%lu U%lu G%c",
                 static_cast<unsigned long>(seconds / 60),
                 static_cast<unsigned long>(seconds % 60),
                 static_cast<unsigned long>(ap_count),
                 static_cast<unsigned long>(station_count),
                 static_cast<unsigned long>(probe_count),
                 static_cast<unsigned long>(repeat_count), gps_fix ? '+' : '-');
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
      const int16_t meter_x = 7;
      const int16_t meter_y = body_top + 10;
      const int16_t meter_width = 50;
      const int16_t meter_height = 54;
    #elif TFT_WIDTH < 200
      const int16_t meter_x = (TFT_WIDTH - 76) / 2;
      const int16_t meter_y = body_top + 10;
      const int16_t meter_width = 76;
      const int16_t meter_height = 76;
    #else
      const int16_t meter_x = 10;
      const int16_t meter_y = body_top + 10;
      const int16_t meter_width = 84;
      const int16_t meter_height = 84;
    #endif
    display_obj.tft.drawRect(meter_x, meter_y, meter_width, meter_height, TFT_DARKGREY);
    display_obj.tft.drawFastHLine(meter_x + 1, meter_y + meter_height / 3,
                                  meter_width - 2, TFT_DARKGREY);
    display_obj.tft.drawFastHLine(meter_x + 1, meter_y + (meter_height * 2) / 3,
                                  meter_width - 2, TFT_DARKGREY);
    const int16_t slot_width = (meter_width - 6) / 4;
    const int16_t baseline = meter_y + meter_height - 3;
    for (uint8_t index = 0; index < 4; index++) {
      const UiEvent& event = ui_events[(ui_event_head + index) % 4];
      if (!event.type) continue;
      uint16_t bar_color = TFT_CYAN;
      if (event.type == 'a') bar_color = TFT_GREEN;
      else if (event.type == 'p') bar_color = TFT_YELLOW;
      else if (event.type == 'A' || event.type == 'S' || event.type == 'B')
        bar_color = TFT_MAGENTA;
      const uint8_t level = reconRssiLevel(event.rssi);
      const int16_t x = meter_x + 3 + index * slot_width;
      const int16_t bar_width = slot_width - 3 > 3 ? slot_width - 3 : 3;
      if (level) {
        const int16_t scaled_height = (meter_height - 7) * level / 8;
        const int16_t bar_height = scaled_height > 2 ? scaled_height : 2;
        display_obj.tft.fillRect(x, baseline - bar_height, bar_width, bar_height, bar_color);
      } else {
        display_obj.tft.drawRect(x, baseline - 3, bar_width, 3, TFT_DARKGREY);
      }
    }

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
      } else {
        snprintf(line, sizeof(line), "BLE DEVICE  %lu", (unsigned long)ble_count);
        display_obj.tft.drawString(line, stats_x, body_top + 35, 1);
        snprintf(line, sizeof(line), "UPDATE      %lu", (unsigned long)repeat_count);
        display_obj.tft.drawString(line, stats_x, body_top + 55, 1);
      }
    #endif

    #if TFT_HEIGHT >= 160
      const int16_t event_y = TFT_HEIGHT - 34;
      display_obj.tft.drawFastHLine(6, event_y - 5, TFT_WIDTH - 12, TFT_DARKGREY);
      const UiEvent& latest = ui_events[(ui_event_head + 3) % 4];
      if (latest.type) {
        if (latest.type == 'p') snprintf(line, sizeof(line), "PROBE  %s", latest.label);
        else snprintf(line, sizeof(line), "%s %02X:%02X:%02X",
          (latest.type == 'a') ? "NEW AP " : (latest.type == 's') ? "NEW STA" :
          (latest.type == 'b') ? "NEW BLE" : "UPDATE ",
          latest.mac[3], latest.mac[4], latest.mac[5]);
        display_obj.tft.setTextColor(latest.type == 'p' ? TFT_YELLOW : TFT_CYAN, TFT_BLACK);
        display_obj.tft.drawString(line, 8, event_y, 1);
      } else {
        display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        display_obj.tft.drawString("Waiting for activity...", 8, event_y, 1);
      }
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
  drawDashboard(current_time);
}
