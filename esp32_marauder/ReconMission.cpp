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

void ReconMission::formatMac(const uint8_t mac[6], char output[18]) {
  snprintf(output, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool ReconMission::start(ReconMode mode) {
  if (running) return false;
  active_mode = mode;
  started_at = millis();
  last_sample = 0;
  ap_count = station_count = ble_count = 0;
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
      snprintf(file_name, sizeof(file_name), "/recon_%lu.csv",
               static_cast<unsigned long>(started_at));
      log_file = SD.open(file_name, FILE_WRITE);
      if (log_file) log_file.println(F("elapsed_ms,type,mac,rssi,channel,lat_e6,lon_e6"));
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

void ReconMission::writeObservation(const char* type, const uint8_t mac[6], int rssi,
                                    uint8_t channel) {
  char mac_text[18];
  formatMac(mac, mac_text);
  int32_t lat = 0;
  int32_t lon = 0;
  #ifdef HAS_GPS
    if (gps_obj.getFixStatus()) {
      lat = gps_obj.getLatInt();
      lon = gps_obj.getLonInt();
    }
  #endif

  #ifdef HAS_SD
    if (log_file) {
      char line[112];
      snprintf(line, sizeof(line), "%lu,%s,%s,%d,%u,%ld,%ld",
               static_cast<unsigned long>(millis() - started_at), type, mac_text, rssi,
               channel, static_cast<long>(lat), static_cast<long>(lon));
      log_file.println(line);
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
        writeObservation("ap", ap.bssid, ap.rssi, ap.channel);
        ap_count++;
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
        writeObservation("station", station.mac, -128, channel);
        station_count++;
      }
    }
  } else {
    #ifdef HAS_BT
      if (ble_devices) {
        const ReconRange range = state.consume(ReconSource::BLE_LIST, ble_devices->size());
        for (size_t index = range.begin; index < range.end; index++) {
          const BleDevice& device = ble_devices->get(index);
          writeObservation("ble", device.mac, device.rssi, 0);
          ble_count++;
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

void ReconMission::printStatus(Stream& output) const {
  output.print(running ? F("Recon active ") : F("Recon stopped "));
  output.print(active_mode == ReconMode::WIFI_RECON ? F("WiFi ") : F("BLE "));
  output.print(ap_count);
  output.print('/');
  output.print(station_count);
  output.print('/');
  output.print(ble_count);
  output.println();
}
