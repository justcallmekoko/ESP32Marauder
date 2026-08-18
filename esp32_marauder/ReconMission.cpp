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
  pending_flush = 0;

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
  #ifdef HAS_SD
    if (log_file) {
      log_file.flush();
      log_file.close();
    }
  #endif
  running = false;
}

void ReconMission::observe(char type, const uint8_t mac[6], int rssi, uint8_t channel) {
  if (!running) return;
  if ((type == 'b') != (active_mode == ReconMode::BLE_RECON)) return;
  writeObservation(type, mac, rssi, channel);
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

void ReconMission::main(uint32_t current_time) {
  (void)current_time;
  if (!running) return;
  if (!wifi_scan_obj.scanning()) stop();
}
