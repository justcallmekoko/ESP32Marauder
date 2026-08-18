#include "ReconMission.h"

#include "WiFiScan.h"
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
}  // namespace

bool ReconMission::start(ReconMode mode) {
  if (running || wifi_scan_obj.scanning()) return false;
  active_mode = mode;
  wifi_scan_obj.StartScan(active_mode == ReconMode::WIFI_RECON ? WIFI_SCAN_AP_STA : BT_SCAN_ALL,
                          active_mode == ReconMode::WIFI_RECON ? TFT_MAGENTA : TFT_CYAN);
  started_at = millis();
  last_sample = 0;
  pending_flush = 0;
  state.reset();
  if (active_mode == ReconMode::WIFI_RECON) {
    state.consume(ReconSource::AP_LIST, access_points ? access_points->size() : 0);
    state.consume(ReconSource::STATION_LIST, stations ? stations->size() : 0);
  } else {
    state.consume(ReconSource::BLE_LIST, ble_devices ? ble_devices->size() : 0);
  }

  #ifdef HAS_SD
    if (sd_obj.supported) {
      char file_name[32];
      snprintf(file_name, sizeof(file_name), "/r_%lu.rlog",
               static_cast<unsigned long>(started_at));
      log_file = SD.open(file_name, FILE_WRITE);
      if (log_file) {
        const ReconLogHeader header;
        log_file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header));
      }
    }
  #endif

  running = true;
  return true;
}

void ReconMission::stop() {
  if (!running) return;
  observeLists();
  #ifdef HAS_SD
    if (log_file) {
      log_file.flush();
      log_file.close();
    }
  #endif
  running = false;
}

void ReconMission::writeObservation(char type, const uint8_t mac[6], int rssi,
                                    uint8_t channel) {
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
}
