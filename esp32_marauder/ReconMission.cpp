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
  #endif
  writeManifest(true);
  buffer_obj.setDirectory(NULL);
  running = false;
}

void ReconMission::writeObservation(char type, const uint8_t mac[6], int rssi,
                                    uint8_t channel) {
  if (type == 'a') ap_count++;
  else if (type == 's') station_count++;
  else if (type == 'b') ble_count++;
  else repeat_count++;

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

void ReconMission::queueRepeat(char type, const uint8_t mac[6], int8_t rssi,
                               uint8_t channel) {
  if (!running || active_mode != ReconMode::WIFI_RECON) return;
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
      "\"observations\":\"obs.rlog\",\"probes\":\"probes.rlog\",\"capture\":\"%s\"}\n",
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
      snprintf(status, sizeof(status), "REC B %lu:%02lu D%lu G%c",
               static_cast<unsigned long>(seconds / 60),
               static_cast<unsigned long>(seconds % 60),
               static_cast<unsigned long>(ble_count), gps_fix ? '+' : '-');
    }
    const uint16_t color = active_mode == ReconMode::WIFI_RECON ? TFT_MAGENTA : TFT_CYAN;
    display_obj.tft.fillRect(0, 16, TFT_WIDTH, 16, color);
    display_obj.tft.fillCircle(7, 24, 3, TFT_RED);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    display_obj.tft.drawCentreString(status, (TFT_WIDTH / 2) + 4, 20, 1);
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
          channel = access_points->get(station.ap).channel;
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
