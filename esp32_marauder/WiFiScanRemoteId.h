#include <cstring>

namespace {
void copyIfPresent(char* destination, size_t destinationSize,
                   const char* source, bool present) {
  if (!present) return;
  std::strncpy(destination, source, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

#ifdef HAS_SD
String remoteIdXmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  value.replace("'", "&apos;");
  return value;
}
#endif

void updateKnownRemoteIdObservation(RemoteIdRecord& record,
                                    const uint8_t mac[6],
                                    RemoteIdTransport transport,
                                    int8_t rssi, uint32_t now) {
  memcpy(record.mac, mac, sizeof(record.mac));
  record.transportMask |= static_cast<uint8_t>(transport);
  record.rssi = rssi;
  if (record.isLost) {
    record.isLost = false;
    ++record.reacquiredCount;
    record.lastReacquiredMs = now;
  }
  record.lastSeenMs = now;
  ++record.packetCount;
  if (record.rateWindowStartedMs == 0) record.rateWindowStartedMs = now;
  ++record.rateWindowPackets;
  const uint32_t elapsed = now - record.rateWindowStartedMs;
  if (elapsed >= 1000) {
    record.packetRateHz = record.rateWindowPackets * 1000.0f /
                          static_cast<float>(elapsed);
    record.rateWindowStartedMs = now;
    record.rateWindowPackets = 0;
  }
}
}

void WiFiScan::mergeRemoteIdRecord(RemoteIdRecord& destination,
                                   const RemoteIdRecord& source) {
  const bool acceptUasId = source.hasUasId &&
      remoteIdShouldReplaceBasicId(destination.idType, destination.hasUasId,
                                   source.idType);
  copyIfPresent(destination.uasId, sizeof(destination.uasId), source.uasId,
                acceptUasId);
  copyIfPresent(destination.operatorId, sizeof(destination.operatorId),
                source.operatorId, source.hasOperatorId);
  copyIfPresent(destination.description, sizeof(destination.description),
                source.description, source.hasDescription);
  if (acceptUasId) {
    destination.hasUasId = true;
    destination.idType = source.idType;
    destination.uaType = source.uaType;
  }
  if (source.hasOperatorId) destination.hasOperatorId = true;
  if (source.hasDescription) destination.hasDescription = true;
  if (source.hasLocation) {
    if (!destination.hasLocation && source.historyCount != 0) {
      memcpy(destination.history, source.history, sizeof(destination.history));
      destination.historyCount = source.historyCount;
    }
    remoteIdUpdateLocation(destination, source.latitudeE7, source.longitudeE7);
    destination.altitudeGeoM = source.altitudeGeoM;
    destination.altitudePressureM = source.altitudePressureM;
    destination.heightM = source.heightM;
    destination.horizontalSpeedMps = source.horizontalSpeedMps;
    destination.verticalSpeedMps = source.verticalSpeedMps;
    destination.directionDeg = source.directionDeg;
    destination.operationalStatus = source.operationalStatus;
    destination.horizontalAccuracy = source.horizontalAccuracy;
    destination.verticalAccuracy = source.verticalAccuracy;
    destination.speedAccuracy = source.speedAccuracy;
    destination.locationTimestampDeciseconds =
        source.locationTimestampDeciseconds;
    destination.heightIsAboveGround = source.heightIsAboveGround;
    destination.directionValid = source.directionValid;
    destination.horizontalSpeedValid = source.horizontalSpeedValid;
    destination.verticalSpeedValid = source.verticalSpeedValid;
    destination.altitudePressureValid = source.altitudePressureValid;
    destination.altitudeGeoValid = source.altitudeGeoValid;
    destination.heightValid = source.heightValid;
    destination.locationTimestampValid = source.locationTimestampValid;
  }
  if (source.hasAuthentication) {
    destination.hasAuthentication = true;
    destination.authenticationType = source.authenticationType;
    destination.authenticationPage = source.authenticationPage;
    destination.authenticationLastPage = source.authenticationLastPage;
    destination.authenticationLength = source.authenticationLength;
    destination.authenticationTimestamp = source.authenticationTimestamp;
  }
  if (source.hasOperatorLocation) {
    destination.hasOperatorLocation = true;
    destination.operatorLatitudeE7 = source.operatorLatitudeE7;
    destination.operatorLongitudeE7 = source.operatorLongitudeE7;
    destination.operatorLocationType = source.operatorLocationType;
    destination.classificationType = source.classificationType;
    destination.areaCount = source.areaCount;
    destination.areaRadiusM = source.areaRadiusM;
    destination.areaCeilingM = source.areaCeilingM;
    destination.areaFloorM = source.areaFloorM;
    destination.areaCeilingValid = source.areaCeilingValid;
    destination.areaFloorValid = source.areaFloorValid;
    destination.categoryEu = source.categoryEu;
    destination.classEu = source.classEu;
    destination.operatorAltitudeGeoM = source.operatorAltitudeGeoM;
    destination.operatorAltitudeValid = source.operatorAltitudeValid;
    destination.systemTimestamp = source.systemTimestamp;
  }
}

bool WiFiScan::processRemoteIdWifiFrame(const uint8_t* frame, size_t length,
                                        const uint8_t mac[6], int8_t rssi,
                                        uint8_t channel) {
  RemoteIdRecord decoded;
  RemoteIdDecodeResult result = remote_id_decoder.decodeWifiBeacon(frame, length, decoded);
  RemoteIdTransport transport = RemoteIdTransport::WifiBeacon;
  if (result != RemoteIdDecodeResult::Decoded) {
    result = remote_id_decoder.decodeWifiNan(frame, length, decoded);
    transport = RemoteIdTransport::WifiNan;
  }
  if (result != RemoteIdDecodeResult::Decoded)
    return false;

  const uint32_t now = millis();
  RemoteIdRecord snapshot;
  bool shouldLog = false;
  portENTER_CRITICAL(&remote_id_mux);
  RemoteIdRecord* record = decoded.hasUasId
      ? remote_id_store.findByUasId(decoded.uasId)
      : nullptr;
  RemoteIdRecord* provisional = remote_id_store.findByMac(mac);
  if (record == nullptr)
    record = &remote_id_store.observe(mac, transport, rssi, now);
  else {
    if (provisional != nullptr && provisional != record) {
      mergeRemoteIdRecord(*record, *provisional);
      record->transportMask |= provisional->transportMask;
      record->packetCount += provisional->packetCount;
      record->lostCount += provisional->lostCount;
      record->reacquiredCount += provisional->reacquiredCount;
      remote_id_store.eraseByMac(mac, record);
      record = remote_id_store.findByUasId(decoded.uasId);
    }
    updateKnownRemoteIdObservation(*record, mac, transport, rssi, now);
  }
  mergeRemoteIdRecord(*record, decoded);
  record->lastWifiChannel = channel;
  shouldLog = now - record->lastLoggedMs >= 1000;
  if (shouldLog) record->lastLoggedMs = now;
  snapshot = *record;
  portEXIT_CRITICAL(&remote_id_mux);
  if (shouldLog) logRemoteIdRecord(snapshot);
  return true;
}

bool WiFiScan::processRemoteIdBlePayload(const uint8_t* payload, size_t length,
                                        const uint8_t mac[6], int8_t rssi,
                                        RemoteIdTransport transport) {
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
        const uint32_t now = millis();
        RemoteIdRecord snapshot;
        bool shouldLog = false;
        portENTER_CRITICAL(&remote_id_mux);
        RemoteIdRecord* record = decoded.hasUasId
            ? remote_id_store.findByUasId(decoded.uasId)
            : nullptr;
        RemoteIdRecord* provisional = remote_id_store.findByMac(mac);
        if (record == nullptr)
          record = &remote_id_store.observe(mac, transport, rssi, now);
        else {
          if (provisional != nullptr && provisional != record) {
            mergeRemoteIdRecord(*record, *provisional);
            record->transportMask |= provisional->transportMask;
            record->packetCount += provisional->packetCount;
            record->lostCount += provisional->lostCount;
            record->reacquiredCount += provisional->reacquiredCount;
            remote_id_store.eraseByMac(mac, record);
            record = remote_id_store.findByUasId(decoded.uasId);
          }
          updateKnownRemoteIdObservation(*record, mac, transport, rssi, now);
        }
        mergeRemoteIdRecord(*record, decoded);
        shouldLog = now - record->lastLoggedMs >= 1000;
        if (shouldLog) record->lastLoggedMs = now;
        snapshot = *record;
        portEXIT_CRITICAL(&remote_id_mux);
        if (shouldLog) logRemoteIdRecord(snapshot);
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
                                        packet->rx_ctrl.rssi,
                                        packet->rx_ctrl.channel);
}

void WiFiScan::RunRemoteIdScan(uint8_t scan_mode, uint16_t color) {
  if (scan_mode == REMOTE_ID_SCAN_ALL) {
    clearRemoteIds();
  }

  #ifdef HAS_SD
    startLog("remoteid");
    buffer_obj.append("millis,uas_id,operator_id,transport,channel,rssi,packets,packet_rate_hz,state,lost_count,reacquired_count,last_lost_ms,last_reacquired_ms,lat,lon,alt_geo,alt_geo_valid,height,height_valid,speed,speed_valid,heading,heading_valid,operator_lat,operator_lon,auth_type,auth_page,auth_last_page,auth_length,auth_timestamp\n");
    startRemoteIdGpx();
  #endif

  #ifdef HAS_SCREEN
    setupScanDisplayArea(TFT_BLACK, color);
    // Preserve the status bar at the top of the display. Clearing the entire
    // screen erases its background, while periodic status updates repaint only
    // individual fields and leave the bar looking fragmented.
    display_obj.tft.fillRect(0, STATUS_BAR_WIDTH, SCREEN_WIDTH,
                             SCREEN_HEIGHT - STATUS_BAR_WIDTH, TFT_BLACK);
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
  changeChannel(6);
  wifi_initialized = true;
  Serial.println("RID scheduler: NAN dwell CH6/CH149 when supported; beacon sweep constrained by configured radio plan");

  setupRemoteIdBle();

  setLEDMode(MODE_SNIFF);
  initTime = millis();
  remote_id_last_render_ms = 0;
  remote_id_schedule_step = 0;
  remote_id_beacon_channel_index = 0;
}

String WiFiScan::remoteIdLabel(size_t index) const {
  RemoteIdRecord snapshot;
  portENTER_CRITICAL(&remote_id_mux);
  const RemoteIdRecord* record = remote_id_store.at(index);
  if (record != nullptr) snapshot = *record;
  portEXIT_CRITICAL(&remote_id_mux);
  if (record == nullptr) return "";
  String id = snapshot.hasUasId ? String(snapshot.uasId) : macToString(snapshot.mac);
  String label = String(snapshot.rssi) + " " + id;
  #ifdef HAS_GPS
    if (gps_obj.getFixStatus() && snapshot.hasLocation) {
      char distance[16];
      remoteIdFormatDistanceKm(remoteIdDistanceMeters(
          gps_obj.getLatInt() * 10, gps_obj.getLonInt() * 10,
          snapshot.latitudeE7, snapshot.longitudeE7),
          distance, sizeof(distance));
      if (distance[0] != '\0') label += " " + String(distance);
    }
  #endif
  return label;
}

void WiFiScan::hopRemoteIdChannel() {
  // Reserve three out of every four dwell periods for the ASTM preferred NAN
  // channels. The remaining period advances through the legal board channel
  // plan so Wi-Fi Beacon transmitters on other channels remain discoverable.
  const uint8_t phase = remote_id_schedule_step++ & 0x03;
  if (phase == 3 && currentScanMode == REMOTE_ID_SCAN_TARGET &&
      remote_id_target_selected) {
    portENTER_CRITICAL(&remote_id_mux);
    const RemoteIdRecord* target = remote_id_target_has_uas
        ? remote_id_store.findByUasId(remote_id_target_uas)
        : remote_id_store.findByMac(remote_id_target_mac);
    RemoteIdRecord snapshot;
    if (target != nullptr) snapshot = *target;
    portEXIT_CRITICAL(&remote_id_mux);
    if (target != nullptr && snapshot.lastWifiChannel != 0 &&
        !remoteIdIsStale(millis(), snapshot.lastSeenMs, REMOTE_ID_STALE_MS)) {
      changeChannel(snapshot.lastWifiChannel);
      return;
    }
  }
  if (phase == 0 || phase == 2) {
    changeChannel(6);
    return;
  }
  #ifdef HAS_DUAL_BAND
    if (phase == 1) {
      changeChannel(149);
      return;
    }
    remote_id_beacon_channel_index =
        (remote_id_beacon_channel_index + 1) % DUAL_BAND_CHANNELS;
    changeChannel(dual_band_channels[remote_id_beacon_channel_index]);
  #else
    ++remote_id_beacon_channel_index;
    if (remote_id_beacon_channel_index == 0 ||
        remote_id_beacon_channel_index > MAX_CHANNEL)
      remote_id_beacon_channel_index = 1;
    changeChannel(remote_id_beacon_channel_index);
  #endif
}

bool WiFiScan::selectRemoteIdTarget(size_t index) {
  portENTER_CRITICAL(&remote_id_mux);
  const RemoteIdRecord* record = remote_id_store.at(index);
  if (record == nullptr) {
    portEXIT_CRITICAL(&remote_id_mux);
    return false;
  }
  memcpy(remote_id_target_mac, record->mac, sizeof(remote_id_target_mac));
  remote_id_target_has_uas = record->hasUasId;
  memset(remote_id_target_uas, 0, sizeof(remote_id_target_uas));
  if (record->hasUasId)
    strncpy(remote_id_target_uas, record->uasId,
            sizeof(remote_id_target_uas) - 1);
  remote_id_target_selected = true;
  portEXIT_CRITICAL(&remote_id_mux);
  return true;
}

void WiFiScan::clearRemoteIds() {
  portENTER_CRITICAL(&remote_id_mux);
  remote_id_store.clear();
  remote_id_target_selected = false;
  remote_id_target_has_uas = false;
  memset(remote_id_target_uas, 0, sizeof(remote_id_target_uas));
  memset(remote_id_target_mac, 0, sizeof(remote_id_target_mac));
  portEXIT_CRITICAL(&remote_id_mux);
}

void WiFiScan::logRemoteIdRecord(const RemoteIdRecord& record) {
  const uint32_t now = millis();
  String line = String(now) + "," +
                (record.hasUasId ? String(record.uasId) : macToString(record.mac)) + "," +
                (record.hasOperatorId ? String(record.operatorId) : "") + "," +
                String(record.transportMask) + "," +
                String(record.lastWifiChannel) + "," + String(record.rssi) + "," +
                String(record.packetCount) + "," +
                String(record.packetRateHz, 1) + "," +
                String(record.isLost ? "lost" : "active") + "," +
                String(record.lostCount) + "," + String(record.reacquiredCount) + "," +
                String(record.lastLostMs) + "," + String(record.lastReacquiredMs) + "," +
                String(record.latitudeE7 / 1e7, 7) + "," +
                String(record.longitudeE7 / 1e7, 7) + "," +
                String(record.altitudeGeoM, 1) + "," + String(record.altitudeGeoValid ? 1 : 0) + "," +
                String(record.heightM, 1) + "," + String(record.heightValid ? 1 : 0) + "," +
                String(record.horizontalSpeedMps, 1) + "," + String(record.horizontalSpeedValid ? 1 : 0) + "," +
                String(record.directionDeg) + "," + String(record.directionValid ? 1 : 0) + "," +
                String(record.operatorLatitudeE7 / 1e7, 7) + "," +
                String(record.operatorLongitudeE7 / 1e7, 7) + "," +
                String(record.hasAuthentication ? record.authenticationType : 0) + "," +
                String(record.hasAuthentication ? record.authenticationPage : 0) + "," +
                String(record.hasAuthentication ? record.authenticationLastPage : 0) + "," +
                String(record.hasAuthentication ? record.authenticationLength : 0) + "," +
                String(record.hasAuthentication ? record.authenticationTimestamp : 0);
  Serial.println("RID," + line);
  #ifdef HAS_SD
    buffer_obj.append(line + "\n");
  #endif
}

#ifdef HAS_SD
void WiFiScan::startRemoteIdGpx() {
  remote_id_gpx_active = false;
  remote_id_gpx_file_name = "";
  memset(remote_id_gpx_tracks, 0, sizeof(remote_id_gpx_tracks));
  if (!sd_obj.supported || !settings_obj.loadSetting<bool>("SavePCAP")) return;
  for (uint16_t index = 0; index < 10000; ++index) {
    const String candidate = "/remoteid_" + String(index) + ".gpx";
    if (!SD.exists(candidate)) {
      remote_id_gpx_file_name = candidate;
      remote_id_gpx_session_index = index;
      break;
    }
  }
  if (remote_id_gpx_file_name.length() == 0) return;
  File file = SD.open(remote_id_gpx_file_name, FILE_WRITE);
  if (!file) return;
  file.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  file.print("<gpx version=\"1.1\" creator=\"ESP32 Marauder Remote ID\" ");
  file.print("xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
  file.close();
  remote_id_gpx_active = true;
  Serial.println("Remote ID GPX: " + remote_id_gpx_file_name);
}

void WiFiScan::logRemoteIdGpxPositions(uint32_t nowMs) {
  if (!remote_id_gpx_active) return;
  size_t count = 0;
  portENTER_CRITICAL(&remote_id_mux);
  for (size_t i = 0; i < remote_id_store.size() && count < REMOTE_ID_CAPACITY; ++i) {
    RemoteIdRecord* record = remote_id_store.at(i);
    if (record == nullptr || !record->hasLocation ||
        record->lastGpxRevision == record->locationRevision ||
        nowMs - record->lastGpxLoggedMs < 1000)
      continue;
    record->lastGpxLoggedMs = nowMs;
    record->lastGpxRevision = record->locationRevision;
    remote_id_snapshots[count++] = *record;
  }
  portEXIT_CRITICAL(&remote_id_mux);
  if (count == 0) return;

  for (size_t i = 0; i < count; ++i) {
    const RemoteIdRecord& record = remote_id_snapshots[i];
    size_t trackIndex = REMOTE_ID_CAPACITY;
    for (size_t track = 0; track < REMOTE_ID_CAPACITY; ++track) {
      RemoteIdGpxTrack& candidate = remote_id_gpx_tracks[track];
      if (!candidate.used) continue;
      if (remoteIdIdentityMatches(candidate.uasId, candidate.mac, record)) {
        trackIndex = track;
        if (record.hasUasId && candidate.uasId[0] == '\0') {
          strncpy(candidate.uasId, record.uasId, sizeof(candidate.uasId) - 1);
          strncpy(candidate.name, record.uasId, sizeof(candidate.name) - 1);
        }
        break;
      }
    }
    if (trackIndex == REMOTE_ID_CAPACITY) {
      for (size_t track = 0; track < REMOTE_ID_CAPACITY; ++track) {
        if (remote_id_gpx_tracks[track].used) continue;
        trackIndex = track;
        RemoteIdGpxTrack& created = remote_id_gpx_tracks[track];
        created.used = true;
        memcpy(created.mac, record.mac, sizeof(created.mac));
        if (record.hasUasId) {
          strncpy(created.uasId, record.uasId, sizeof(created.uasId) - 1);
          strncpy(created.name, record.uasId, sizeof(created.name) - 1);
        } else {
          macToString(record.mac).toCharArray(created.name, sizeof(created.name));
        }
        break;
      }
    }
    if (trackIndex == REMOTE_ID_CAPACITY) continue;

    char fragmentPath[40];
    snprintf(fragmentPath, sizeof(fragmentPath), "/remoteid_%u_%u.tmp",
             remote_id_gpx_session_index, static_cast<unsigned>(trackIndex));
    File fragment = SD.open(fragmentPath, FILE_APPEND);
    if (!fragment) continue;
    fragment.print("      <trkpt lat=\"");
    fragment.print(record.latitudeE7 / 1e7, 7);
    fragment.print("\" lon=\"");
    fragment.print(record.longitudeE7 / 1e7, 7);
    fragment.print("\">\n");
    if (record.altitudeGeoValid) {
      fragment.print("        <ele>");
      fragment.print(record.altitudeGeoM, 1);
      fragment.print("</ele>\n");
    }
    fragment.print("      </trkpt>\n");
    fragment.close();
    ++remote_id_gpx_tracks[trackIndex].pointCount;
  }
}

void WiFiScan::finishRemoteIdGpx() {
  if (!remote_id_gpx_active) return;
  File file = SD.open(remote_id_gpx_file_name, FILE_APPEND);
  if (file) {
    uint8_t copyBuffer[256];
    for (size_t trackIndex = 0; trackIndex < REMOTE_ID_CAPACITY; ++trackIndex) {
      const RemoteIdGpxTrack& track = remote_id_gpx_tracks[trackIndex];
      if (!track.used || track.pointCount == 0) continue;
      char fragmentPath[40];
      snprintf(fragmentPath, sizeof(fragmentPath), "/remoteid_%u_%u.tmp",
               remote_id_gpx_session_index, static_cast<unsigned>(trackIndex));
      File fragment = SD.open(fragmentPath, FILE_READ);
      if (!fragment) continue;
      file.print("  <trk>\n    <name>");
      file.print(remoteIdXmlEscape(String(track.name)));
      file.print("</name>\n    <desc>Remote ID device route</desc>\n    <trkseg>\n");
      while (fragment.available()) {
        const size_t bytesRead = fragment.read(copyBuffer, sizeof(copyBuffer));
        if (bytesRead == 0) break;
        file.write(copyBuffer, bytesRead);
      }
      fragment.close();
      file.print("    </trkseg>\n  </trk>\n");
      SD.remove(fragmentPath);
    }
    file.print("</gpx>\n");
    file.close();
  }
  memset(remote_id_gpx_tracks, 0, sizeof(remote_id_gpx_tracks));
  remote_id_gpx_active = false;
}
#endif

size_t WiFiScan::snapshotRemoteIds(RemoteIdRecord* records,
                                   size_t capacity) const {
  if (records == nullptr || capacity == 0) return 0;
  portENTER_CRITICAL(&remote_id_mux);
  const size_t count = std::min(capacity, remote_id_store.size());
  for (size_t i = 0; i < count; ++i) records[i] = *remote_id_store.at(i);
  portEXIT_CRITICAL(&remote_id_mux);
  return count;
}

size_t WiFiScan::remoteIdCount() const {
  portENTER_CRITICAL(&remote_id_mux);
  const size_t count = remote_id_store.size();
  portEXIT_CRITICAL(&remote_id_mux);
  return count;
}

bool WiFiScan::snapshotRemoteIdTarget(RemoteIdRecord& record) const {
  portENTER_CRITICAL(&remote_id_mux);
  const RemoteIdRecord* selected = nullptr;
  if (remote_id_target_selected) {
    selected = remote_id_target_has_uas
        ? remote_id_store.findByUasId(remote_id_target_uas)
        : remote_id_store.findByMac(remote_id_target_mac);
    if (selected == nullptr)
      selected = remote_id_store.findByMac(remote_id_target_mac);
  }
  if (selected != nullptr) record = *selected;
  portEXIT_CRITICAL(&remote_id_mux);
  return selected != nullptr;
}

void WiFiScan::renderRemoteIdGlobal() {
  #ifdef HAS_SCREEN
    const size_t recordCount =
        snapshotRemoteIds(remote_id_snapshots, REMOTE_ID_CAPACITY);
    const RemoteIdLayout layout = remoteIdLayoutForDisplay(SCREEN_WIDTH, SCREEN_HEIGHT);
    const int16_t top = 34;
    const int16_t contentHeight = SCREEN_HEIGHT - top;
    const int16_t legendHeight = layout == RemoteIdLayout::SplitListGrid ? 12 : 0;
    const int16_t listHeight = layout == RemoteIdLayout::SplitListGrid
                                   ? (contentHeight - legendHeight) / 2
                                   : contentHeight;
    display_obj.tft.fillRect(0, top, SCREEN_WIDTH, contentHeight, TFT_BLACK);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    display_obj.tft.setCursor(2, top + 2);
    display_obj.tft.print("RID devices: " + String(recordCount) +
                          " CH:" + String(set_channel));
    const size_t rows = layout == RemoteIdLayout::Compact ? 5 :
                        static_cast<size_t>((listHeight - 14) / 10);
    const size_t first = recordCount > rows
                             ? recordCount - rows
                             : 0;
    int16_t y = top + 13;
    for (size_t i = first; i < recordCount; ++i) {
      const RemoteIdRecord* record = &remote_id_snapshots[i];
      char transports[16];
      remoteIdFormatTransports(record->transportMask, transports,
                               sizeof(transports));
      display_obj.tft.setCursor(2, y);
      display_obj.tft.setTextColor(rssiToColor(record->rssi), TFT_BLACK);
      String id = record->hasUasId ? String(record->uasId) : macToString(record->mac);
      display_obj.tft.print(String(i) + ":" + String(record->rssi) +
                            " [" + String(transports) + "] " + id);
      y += 10;
    }
    if (first > 0) {
      display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      display_obj.tft.setCursor(SCREEN_WIDTH - 58, top + 2);
      display_obj.tft.print("+" + String(first) + " older");
    }

    if (layout == RemoteIdLayout::SplitListGrid) {
      const int16_t legendTop = top + listHeight;
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.tft.setCursor(3, legendTop + 1);
      display_obj.tft.print("M Marauder");
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.setCursor(SCREEN_WIDTH / 2 - 30, legendTop + 1);
      display_obj.tft.print("A Aircraft");
      display_obj.tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
      display_obj.tft.setCursor(SCREEN_WIDTH - 62, legendTop + 1);
      display_obj.tft.print("O Operator");
      const int16_t gridTop = legendTop + legendHeight;
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
          const float radius = remoteIdGridScaleMeters(remote_id_snapshots,
              recordCount, originLat, originLon, 50.0f);
          display_obj.tft.fillCircle(SCREEN_WIDTH / 2,
              gridTop + gridHeight / 2, 3, TFT_GREEN);
          for (size_t i = 0; i < recordCount; ++i) {
            const RemoteIdRecord* record = &remote_id_snapshots[i];
            if (record->hasLocation) {
              RemoteIdGridPoint previousTail;
              bool hasPreviousTail = false;
              for (size_t h = 0; h < record->historyCount; ++h) {
                const RemoteIdGridPoint tail = remoteIdProjectHistoryToGrid(
                    *record, record->history[h], originLat, originLon,
                    SCREEN_WIDTH, gridHeight, radius, 8);
                if (tail.visible) {
                  if (hasPreviousTail)
                    display_obj.tft.drawLine(previousTail.x,
                        gridTop + previousTail.y, tail.x,
                        gridTop + tail.y, TFT_RED);
                  previousTail = tail;
                  hasPreviousTail = true;
                }
              }
              RemoteIdGridPoint p = remoteIdProjectToGrid(record->latitudeE7,
                  record->longitudeE7, originLat, originLon,
                  SCREEN_WIDTH, gridHeight, radius, 8);
              if (hasPreviousTail && p.visible)
                display_obj.tft.drawLine(previousTail.x,
                    gridTop + previousTail.y, p.x, gridTop + p.y, TFT_RED);
              if (p.visible)
                display_obj.tft.fillCircle(p.x, gridTop + p.y, 3, TFT_CYAN);
              else if (p.offGrid)
                display_obj.tft.drawCircle(p.x, gridTop + p.y, 4, TFT_CYAN);
            }
            if (record->hasOperatorLocation) {
              RemoteIdGridPoint p = remoteIdProjectToGrid(record->operatorLatitudeE7,
                  record->operatorLongitudeE7, originLat, originLon,
                  SCREEN_WIDTH, gridHeight, radius, 8);
              if (p.visible)
                display_obj.tft.fillCircle(p.x, gridTop + p.y, 3, TFT_MAGENTA);
              else if (p.offGrid)
                display_obj.tft.drawCircle(p.x, gridTop + p.y, 4, TFT_MAGENTA);
            }
          }
          display_obj.tft.setCursor(3, SCREEN_HEIGHT - 10);
          display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
          char radiusLabel[16];
          remoteIdFormatGridRadius(radius, radiusLabel, sizeof(radiusLabel));
          display_obj.tft.print("R " + String(radiusLabel));
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
  RemoteIdRecord snapshot;
  if (!snapshotRemoteIdTarget(snapshot)) return;
  const RemoteIdRecord* record = &snapshot;
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
    char transports[16];
    remoteIdFormatTransports(record->transportMask, transports,
                             sizeof(transports));
    line("TRANSPORT: " + String(transports), TFT_CYAN);
    line("RSSI " + String(record->rssi) + "  AGE " +
         String((millis() - record->lastSeenMs) / 1000) + "s");
    if (record->isLost)
      line("STATUS LOST  RATE " + String(record->packetRateHz, 1) + "/s", TFT_RED);
    else if (record->lastReacquiredMs != 0 &&
             millis() - record->lastReacquiredMs < 10000)
      line("STATUS REACQUIRED  " + String(record->packetRateHz, 1) + "/s", TFT_YELLOW);
    else
      line("PKT " + String(record->packetCount) + " " +
           String(record->packetRateHz, 1) + "/s CH " +
           String(record->lastWifiChannel), TFT_GREEN);
    if (record->hasLocation) {
      line("LAT " + String(record->latitudeE7 / 1e7, 7));
      line("LON " + String(record->longitudeE7 / 1e7, 7));
      String altitude = record->altitudeGeoValid
          ? String(record->altitudeGeoM, 1) + "m" : "N/A";
      String height = record->heightValid
          ? String(record->heightM, 1) + "m" : "N/A";
      line("ALT " + altitude + "  H " + height);
      String speed = record->horizontalSpeedValid
          ? String(record->horizontalSpeedMps, 1) + "m/s" : "N/A";
      String heading = record->directionValid
          ? String(record->directionDeg) : "N/A";
      line("SPD " + speed + "  HDG " + heading);
    }
    if (record->hasAuthentication)
      line("AUTH " + String(record->authenticationType) + " PAGE " +
           String(record->authenticationPage) + "/" +
           String(record->authenticationLastPage), TFT_YELLOW);
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
