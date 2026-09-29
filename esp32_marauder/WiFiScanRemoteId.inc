#include <cstring>

namespace {
void copyIfPresent(char* destination, size_t destinationSize,
                   const char* source, bool present) {
  if (!present) return;
  std::strncpy(destination, source, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}
}

void WiFiScan::mergeRemoteIdRecord(RemoteIdRecord& destination,
                                   const RemoteIdRecord& source) {
  copyIfPresent(destination.uasId, sizeof(destination.uasId), source.uasId,
                source.hasUasId);
  copyIfPresent(destination.operatorId, sizeof(destination.operatorId),
                source.operatorId, source.hasOperatorId);
  copyIfPresent(destination.description, sizeof(destination.description),
                source.description, source.hasDescription);
  if (source.hasUasId) {
    destination.hasUasId = true;
    destination.idType = source.idType;
    destination.uaType = source.uaType;
  }
  if (source.hasOperatorId) destination.hasOperatorId = true;
  if (source.hasDescription) destination.hasDescription = true;
  if (source.hasLocation) {
    destination.hasLocation = true;
    destination.latitudeE7 = source.latitudeE7;
    destination.longitudeE7 = source.longitudeE7;
    destination.altitudeGeoM = source.altitudeGeoM;
    destination.altitudePressureM = source.altitudePressureM;
    destination.heightM = source.heightM;
    destination.horizontalSpeedMps = source.horizontalSpeedMps;
    destination.verticalSpeedMps = source.verticalSpeedMps;
    destination.directionDeg = source.directionDeg;
    destination.operationalStatus = source.operationalStatus;
  }
  if (source.hasOperatorLocation) {
    destination.hasOperatorLocation = true;
    destination.operatorLatitudeE7 = source.operatorLatitudeE7;
    destination.operatorLongitudeE7 = source.operatorLongitudeE7;
    destination.operatorLocationType = source.operatorLocationType;
  }
}

bool WiFiScan::processRemoteIdWifiFrame(const uint8_t* frame, size_t length,
                                        const uint8_t mac[6], int8_t rssi) {
  RemoteIdRecord decoded;
  RemoteIdDecodeResult result = remote_id_decoder.decodeWifiBeacon(frame, length, decoded);
  RemoteIdTransport transport = RemoteIdTransport::WifiBeacon;
  if (result != RemoteIdDecodeResult::Decoded) {
    result = remote_id_decoder.decodeWifiNan(frame, length, decoded);
    transport = RemoteIdTransport::WifiNan;
  }
  if (result != RemoteIdDecodeResult::Decoded)
    return false;

  RemoteIdRecord* record = decoded.hasUasId
      ? remote_id_store.findByUasId(decoded.uasId)
      : nullptr;
  if (record == nullptr)
    record = &remote_id_store.observe(mac, transport,
                                      rssi, millis());
  else {
    record->transportMask |= static_cast<uint8_t>(transport);
    record->rssi = rssi;
    record->lastSeenMs = millis();
    ++record->packetCount;
  }
  mergeRemoteIdRecord(*record, decoded);
  logRemoteIdRecord(*record);
  return true;
}

bool WiFiScan::processRemoteIdBlePayload(const uint8_t* payload, size_t length,
                                        const uint8_t mac[6], int8_t rssi) {
  if (payload == nullptr) return false;
  size_t offset = 0;
  while (offset < length) {
    const size_t adLength = payload[offset];
    if (adLength == 0) break;
    if (offset + adLength + 1 > length) return false;
    const uint8_t adType = payload[offset + 1];
    if (adType == 0x16 && adLength >= 4) {
      RemoteIdRecord decoded;
      if (remote_id_decoder.decodeBleServiceData(payload + offset + 2,
                                                  adLength - 1, decoded) ==
          RemoteIdDecodeResult::Decoded) {
        RemoteIdRecord* record = decoded.hasUasId
            ? remote_id_store.findByUasId(decoded.uasId)
            : nullptr;
        if (record == nullptr)
          record = &remote_id_store.observe(mac, RemoteIdTransport::BleLegacy,
                                            rssi, millis());
        else {
          record->transportMask |= static_cast<uint8_t>(RemoteIdTransport::BleLegacy);
          record->rssi = rssi;
          record->lastSeenMs = millis();
          ++record->packetCount;
        }
        mergeRemoteIdRecord(*record, decoded);
        logRemoteIdRecord(*record);
        return true;
      }
    }
    offset += adLength + 1;
  }
  return false;
}

void WiFiScan::remoteIdWifiCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT || buf == nullptr) return;
  extern WiFiScan wifi_scan_obj;
  wifi_promiscuous_pkt_t* packet = static_cast<wifi_promiscuous_pkt_t*>(buf);
  size_t length = packet->rx_ctrl.sig_len;
  if (length >= 4) length -= 4;
  if (length < 24 || (packet->payload[0] != 0x80 && packet->payload[0] != 0xD0)) return;
  wifi_scan_obj.processRemoteIdWifiFrame(packet->payload, length,
                                        packet->payload + 10,
                                        packet->rx_ctrl.rssi);
}

void WiFiScan::RunRemoteIdScan(uint8_t scan_mode, uint16_t color) {
  if (scan_mode == REMOTE_ID_SCAN_ALL) {
    clearRemoteIds();
  }

  #ifdef HAS_SD
    startLog("remoteid");
    buffer_obj.append("millis,uas_id,operator_id,transport,rssi,lat,lon,alt_geo,height,speed,heading,operator_lat,operator_lon\n");
  #endif

  #ifdef HAS_SCREEN
    setupScanDisplayArea(TFT_BLACK, color);
    display_obj.tft.fillScreen(TFT_BLACK);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    display_obj.tft.drawCentreString(scan_mode == REMOTE_ID_SCAN_TARGET
                                        ? "Remote ID Target"
                                        : "Remote ID Scan",
                                    TFT_WIDTH / 2, 16, 2);
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
  #endif

  esp_wifi_init(&cfg2);
  #ifdef HAS_IDF_3
    esp_wifi_set_country(&country);
    esp_event_loop_create_default();
  #endif
  setWiFiMode(WIFI_MODE_NULL, remoteIdWifiCallback);
  changeChannel(set_channel);
  wifi_initialized = true;

  setupRemoteIdBle();

  setLEDMode(MODE_SNIFF);
  initTime = millis();
  remote_id_last_render_ms = 0;
}

String WiFiScan::remoteIdLabel(size_t index) const {
  const RemoteIdRecord* record = remote_id_store.at(index);
  if (record == nullptr) return "";
  String id = record->hasUasId ? String(record->uasId) : macToString(record->mac);
  return String(record->rssi) + " " + id;
}

bool WiFiScan::selectRemoteIdTarget(size_t index) {
  const RemoteIdRecord* record = remote_id_store.at(index);
  if (record == nullptr) return false;
  memcpy(remote_id_target_mac, record->mac, sizeof(remote_id_target_mac));
  remote_id_target_has_uas = record->hasUasId;
  memset(remote_id_target_uas, 0, sizeof(remote_id_target_uas));
  if (record->hasUasId)
    strncpy(remote_id_target_uas, record->uasId,
            sizeof(remote_id_target_uas) - 1);
  remote_id_target_selected = true;
  return true;
}

void WiFiScan::clearRemoteIds() {
  remote_id_store.clear();
  remote_id_target_selected = false;
  remote_id_target_has_uas = false;
  memset(remote_id_target_uas, 0, sizeof(remote_id_target_uas));
  memset(remote_id_target_mac, 0, sizeof(remote_id_target_mac));
}

void WiFiScan::logRemoteIdRecord(const RemoteIdRecord& record) {
  const uint32_t now = millis();
  if (now - remote_id_last_log_ms < 1000) return;
  remote_id_last_log_ms = now;
  String line = String(now) + "," +
                (record.hasUasId ? String(record.uasId) : macToString(record.mac)) + "," +
                (record.hasOperatorId ? String(record.operatorId) : "") + "," +
                String(record.transportMask) + "," + String(record.rssi) + "," +
                String(record.latitudeE7 / 1e7, 7) + "," +
                String(record.longitudeE7 / 1e7, 7) + "," +
                String(record.altitudeGeoM, 1) + "," + String(record.heightM, 1) + "," +
                String(record.horizontalSpeedMps, 1) + "," + String(record.directionDeg) + "," +
                String(record.operatorLatitudeE7 / 1e7, 7) + "," +
                String(record.operatorLongitudeE7 / 1e7, 7);
  Serial.println("RID," + line);
  #ifdef HAS_SD
    buffer_obj.append(line + "\n");
  #endif
}

void WiFiScan::renderRemoteIdGlobal() {
  #ifdef HAS_SCREEN
    const RemoteIdLayout layout = remoteIdLayoutForDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);
    const int16_t top = 34;
    const int16_t contentHeight = SCREEN_HEIGHT - top;
    const int16_t listHeight = layout == RemoteIdLayout::SplitListGrid
                                   ? contentHeight / 2
                                   : contentHeight;
    display_obj.tft.fillRect(0, top, SCREEN_WIDTH, contentHeight, TFT_BLACK);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    display_obj.tft.setCursor(2, top + 2);
    display_obj.tft.print("RID devices: " + String(remote_id_store.size()));
    const size_t rows = layout == RemoteIdLayout::Compact ? 5 :
                        static_cast<size_t>((listHeight - 14) / 10);
    const size_t first = remote_id_store.size() > rows
                             ? remote_id_store.size() - rows
                             : 0;
    int16_t y = top + 13;
    for (size_t i = first; i < remote_id_store.size(); ++i) {
      const RemoteIdRecord* record = remote_id_store.at(i);
      display_obj.tft.setCursor(2, y);
      display_obj.tft.setTextColor(rssiToColor(record->rssi), TFT_BLACK);
      display_obj.tft.print(remoteIdLabel(i));
      y += 10;
    }

    if (layout == RemoteIdLayout::SplitListGrid) {
      const int16_t gridTop = top + listHeight;
      const int16_t gridHeight = SCREEN_HEIGHT - gridTop;
      display_obj.tft.drawRect(0, gridTop, SCREEN_WIDTH, gridHeight, TFT_DARKGREY);
      display_obj.tft.drawFastVLine(SCREEN_WIDTH / 2, gridTop + 2,
                                   gridHeight - 4, TFT_DARKGREY);
      display_obj.tft.drawFastHLine(2, gridTop + gridHeight / 2,
                                   SCREEN_WIDTH - 4, TFT_DARKGREY);
      display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
      display_obj.tft.setCursor(SCREEN_WIDTH / 2 - 3, gridTop + 2);
      display_obj.tft.print("N");
      #ifdef HAS_GPS
        if (gps_obj.getFixStatus()) {
          const int32_t originLat = gps_obj.getLatInt() * 10;
          const int32_t originLon = gps_obj.getLonInt() * 10;
          const float radius = remoteIdGridScaleMeters(remote_id_records,
              remote_id_store.size(), originLat, originLon, 50.0f);
          display_obj.tft.fillCircle(SCREEN_WIDTH / 2,
              gridTop + gridHeight / 2, 3, TFT_GREEN);
          for (size_t i = 0; i < remote_id_store.size(); ++i) {
            const RemoteIdRecord* record = remote_id_store.at(i);
            if (record->hasLocation) {
              RemoteIdGridPoint p = remoteIdProjectToGrid(record->latitudeE7,
                  record->longitudeE7, originLat, originLon,
                  SCREEN_WIDTH, gridHeight, radius, 8);
              if (p.visible) display_obj.tft.fillCircle(p.x, gridTop + p.y, 3, TFT_CYAN);
            }
            if (record->hasOperatorLocation) {
              RemoteIdGridPoint p = remoteIdProjectToGrid(record->operatorLatitudeE7,
                  record->operatorLongitudeE7, originLat, originLon,
                  SCREEN_WIDTH, gridHeight, radius, 8);
              if (p.visible) display_obj.tft.fillCircle(p.x, gridTop + p.y, 3, TFT_MAGENTA);
            }
          }
          display_obj.tft.setCursor(3, SCREEN_HEIGHT - 10);
          display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
          display_obj.tft.print("R " + String(radius, 0) + "m");
        } else {
          display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
          display_obj.tft.drawCentreString("NO GPS FIX", SCREEN_WIDTH / 2,
                                           gridTop + gridHeight / 2 - 4, 1);
        }
      #else
        display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
        display_obj.tft.drawCentreString("GPS UNAVAILABLE", SCREEN_WIDTH / 2,
                                         gridTop + gridHeight / 2 - 4, 1);
      #endif
    }
  #endif
}

void WiFiScan::renderRemoteIdTarget() {
  const RemoteIdRecord* record = nullptr;
  if (remote_id_target_selected) {
    if (remote_id_target_has_uas)
      record = remote_id_store.findByUasId(remote_id_target_uas);
    if (record == nullptr)
      record = remote_id_store.findByMac(remote_id_target_mac);
  }
  if (record == nullptr) return;
  #ifdef HAS_SCREEN
    display_obj.tft.fillRect(0, 34, SCREEN_WIDTH, SCREEN_HEIGHT - 34, TFT_BLACK);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    int16_t y = 36;
    auto line = [&](const String& value, uint16_t color = TFT_WHITE) {
      display_obj.tft.setCursor(2, y);
      display_obj.tft.setTextColor(color, TFT_BLACK);
      display_obj.tft.print(value);
      y += 11;
    };
    line(record->hasUasId ? "ID " + String(record->uasId)
                          : "MAC " + macToString(record->mac), TFT_CYAN);
    if (record->hasOperatorId) line("OP " + String(record->operatorId), TFT_MAGENTA);
    if (record->hasDescription) line("DESC " + String(record->description));
    line("RSSI " + String(record->rssi) + "  AGE " +
         String((millis() - record->lastSeenMs) / 1000) + "s");
    if (record->hasLocation) {
      line("LAT " + String(record->latitudeE7 / 1e7, 7));
      line("LON " + String(record->longitudeE7 / 1e7, 7));
      line("ALT " + String(record->altitudeGeoM, 1) + "m  H " +
           String(record->heightM, 1) + "m");
      line("SPD " + String(record->horizontalSpeedMps, 1) + "m/s  HDG " +
           String(record->directionDeg));
    }
    if (record->hasOperatorLocation) {
      line("OPLAT " + String(record->operatorLatitudeE7 / 1e7, 7), TFT_MAGENTA);
      line("OPLON " + String(record->operatorLongitudeE7 / 1e7, 7), TFT_MAGENTA);
    }
    #ifdef HAS_GPS
      if (gps_obj.getFixStatus() && record->hasLocation) {
        const float distance = remoteIdDistanceMeters(gps_obj.getLatInt() * 10,
            gps_obj.getLonInt() * 10, record->latitudeE7, record->longitudeE7);
        const float bearing = remoteIdBearingDegrees(gps_obj.getLatInt() * 10,
            gps_obj.getLonInt() * 10, record->latitudeE7, record->longitudeE7);
        line("RANGE " + String(distance, 0) + "m  BRG " + String(bearing, 0), TFT_GREEN);
      }
    #endif
  #endif
}
