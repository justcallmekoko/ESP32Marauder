#include "ZBScan.h"

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif

// Global node list, defined here (declared extern in ZBScan.h), mirroring the
// way WiFiScan.cpp owns the global `access_points` list.
LinkedList<ZigbeeNode>* zigbee_nodes = new LinkedList<ZigbeeNode>();

ZBScan::ZBScan() {}

// ---------------------------------------------------------------------------
// Broadcast / reserved short address helper (mirrors C6_zigbee's is_bcast()).
// ---------------------------------------------------------------------------
static inline bool zb_is_bcast(uint16_t addr) {
  return (addr == 0xFFFF || addr == 0xFFFE || addr == 0xFFFB ||
          addr == 0xFFFC || addr == 0xFFFD);
}

// ---------------------------------------------------------------------------
// Node list helpers - available on every target, radio or not.
// ---------------------------------------------------------------------------
int ZBScan::zbFindNode(uint16_t short_addr) {
  if (zigbee_nodes == nullptr)
    return -1;
  for (int i = 0; i < zigbee_nodes->size(); i++) {
    if (zigbee_nodes->get(i).short_addr == short_addr)
      return i;
  }
  return -1;
}

size_t ZBScan::retainedZigbeeNodeCount() const {
  return zigbee_nodes == nullptr ? 0 : zigbee_nodes->size();
}

int ZBScan::clearZigbeeNodes() {
  if (zigbee_nodes == nullptr) {
    zigbee_nodes = new LinkedList<ZigbeeNode>();
    return 0;
  }
  int num = zigbee_nodes->size();
  for (int i = 0; i < num; i++) {
    ZigbeeNode node = zigbee_nodes->get(i);
    if (node.connections != nullptr) {
      delete node.connections;
      node.connections = nullptr;
    }
  }
  zigbee_nodes->clear();
  return num;
}

// Add a node if new, otherwise refresh it. For source frames, record the link
// to the destination short address (the "connection"). Ported from
// IEEE802154Sniffer::_updateHost(), reduced to the fields Marauder tracks.
ZigbeeNode* ZBScan::zbUpdateNode(const ZbFrameInfo &info, uint16_t addr, bool is_src) {
  if (zb_is_bcast(addr))
    return nullptr;
  if (zigbee_nodes == nullptr)
    zigbee_nodes = new LinkedList<ZigbeeNode>();

  uint32_t now = millis();
  int idx = zbFindNode(addr);

  ZigbeeNode node;
  if (idx < 0) {
    node.short_addr    = addr;
    node.pan_id        = info.panId;
    node.ext_addr      = 0;
    node.rssi          = info.rssi;
    node.lqi           = info.lqi;
    node.channel       = info.channel;
    node.device_type   = (addr == 0x0000) ? ZB_DEV_COORDINATOR : ZB_DEV_UNKNOWN;
    node.selected      = false;
    node.beacon_seen   = false;
    node.assoc_permit  = false;
    node.packets       = 0;
    node.first_seen_ms = now;
    node.last_seen_ms  = now;
    node.connections   = new LinkedList<uint16_t>();
  } else {
    node = zigbee_nodes->get(idx);
  }

  // Learn the extended (MAC) address when it shows up.
  if (addr == info.macSrc && info.srcExtended != 0 && node.ext_addr == 0)
    node.ext_addr = info.srcExtended;
  if (addr == info.macDst && info.dstExtended != 0 && node.ext_addr == 0)
    node.ext_addr = info.dstExtended;

  node.rssi         = info.rssi;
  node.lqi          = info.lqi;
  node.channel      = info.channel;
  node.last_seen_ms = now;
  if (node.packets < 0xFFFF)
    node.packets++;

  // Record a connection from this source node to the destination it talked to.
  if (is_src && !zb_is_bcast(info.macDst) && info.macDst != 0) {
    bool present = false;
    for (int j = 0; j < node.connections->size(); j++) {
      if (node.connections->get(j) == info.macDst) {
        present = true;
        break;
      }
    }
    if (!present)
      node.connections->add(info.macDst);
  }

  // Beacon senders are coordinators or routers.
  if (info.beaconSeen && is_src) {
    node.beacon_seen  = true;
    node.assoc_permit = info.beaconAssocPermit;
    if (node.device_type == ZB_DEV_UNKNOWN)
      node.device_type = ZB_DEV_ROUTER;
  }

  if (idx < 0)
    zigbee_nodes->add(node);
  else
    zigbee_nodes->set(idx, node);

  return nullptr;
}

// Fold one decoded frame into the node list + connections.
void ZBScan::zbProcessFrame(const ZbFrameInfo &info) {
  this->zbUpdateNode(info, info.macSrc, true);
  this->zbUpdateNode(info, info.macDst, false);

  if (info.hasNwk) {
    this->zbUpdateNode(info, info.nwkSrc, true);
    this->zbUpdateNode(info, info.nwkDst, false);
  }
}

#ifndef HAS_IEEE802154
// ---------------------------------------------------------------------------
// Boards without an 802.15.4 radio: scan entry points are harmless no-ops.
// ---------------------------------------------------------------------------
void ZBScan::RunZigbeeScan(uint8_t scan_mode, uint16_t color) {
  (void)scan_mode;
  (void)color;
  Serial.println(F("[ZBScan] Zigbee sniffing requires an ESP32-C5 / ESP32-C6 / ESP32-H2"));
}
void ZBScan::StopZigbeeScan() {}
void ZBScan::zigbeeLoop(uint32_t currentTime) { (void)currentTime; }

#else // HAS_IEEE802154

// ---------------------------------------------------------------------------
// 802.15.4 radio plumbing (ESP32-C5 / ESP32-C6 / ESP32-H2 only).
// Ported from C6_zigbee IEEE802154Sniffer (init/start/stop/rxCallback).
// ---------------------------------------------------------------------------
QueueHandle_t ZBScan::zb_rx_queue = nullptr;

// The ESP-IDF 802.15.4 driver calls these C hooks. Route the receive-done hook
// into ZBScan::rxCallback, then hand the buffer back to the driver.
extern "C" void esp_ieee802154_receive_done(uint8_t *frame,
                                             esp_ieee802154_frame_info_t *fi) {
  ZBScan::rxCallback(frame, fi);
  esp_ieee802154_receive_handle_done(frame);
}
extern "C" void esp_ieee802154_receive_failed(uint16_t error) { (void)error; }
extern "C" void esp_ieee802154_receive_sfd_done(void) {}
extern "C" void esp_ieee802154_transmit_done(const uint8_t *frame,
                                             const uint8_t *ack,
                                             esp_ieee802154_frame_info_t *fi) {
  (void)frame; (void)ack; (void)fi;
}
extern "C" void esp_ieee802154_transmit_failed(const uint8_t *frame,
                                               esp_ieee802154_tx_error_t err) {
  (void)frame; (void)err;
}

// RX ISR context: copy the frame into the queue for the main loop to decode.
void ZBScan::rxCallback(uint8_t *frame, esp_ieee802154_frame_info_t *fi) {
  if (!zb_rx_queue || !frame)
    return;
  uint8_t len = frame[0];               // PHR length byte
  if (len < 3 || len > ZIGBEE_MAX_FRAME_LEN)
    return;

  ZbSnifferFrame sf;
  sf.len     = len - 2;                  // strip the 2-byte FCS
  sf.rssi    = fi ? fi->rssi : 0;
  sf.lqi     = fi ? fi->lqi  : 0;
  sf.channel = esp_ieee802154_get_channel();
  memcpy(sf.data, &frame[1], sf.len);

  BaseType_t woken = pdFALSE;
  xQueueSendFromISR(zb_rx_queue, &sf, &woken);
  if (woken)
    portYIELD_FROM_ISR();
}

bool ZBScan::zbRadioInit(uint8_t channel) {
  this->zb_channel = constrain(channel, ZIGBEE_MIN_CHANNEL, ZIGBEE_MAX_CHANNEL);

  if (!zb_rx_queue) {
    zb_rx_queue = xQueueCreate(ZIGBEE_QUEUE_DEPTH, sizeof(ZbSnifferFrame));
    if (!zb_rx_queue)
      return false;
  }

  if (esp_ieee802154_enable() != ESP_OK)
    return false;

  esp_ieee802154_set_promiscuous(true);
  esp_ieee802154_set_coordinator(false);
  esp_ieee802154_set_rx_when_idle(true);
  esp_ieee802154_set_panid(0xFFFF);
  esp_ieee802154_set_short_address(0xFFFF);

  uint8_t eui64[8] = {};
  esp_read_mac(eui64, ESP_MAC_IEEE802154);
  esp_ieee802154_set_extended_address(eui64);
  esp_ieee802154_set_channel(this->zb_channel);

  this->zb_initialized = true;
  Serial.printf("[ZBScan] 802.15.4 radio initialised on channel %u\n", this->zb_channel);
  return true;
}

bool ZBScan::zbRadioStart() {
  if (!this->zb_initialized && !this->zbRadioInit(this->zb_channel))
    return false;
  if (this->zb_running)
    return true;
  if (esp_ieee802154_receive() != ESP_OK)
    return false;
  this->zb_running = true;
  return true;
}

void ZBScan::zbRadioStop() {
  if (this->zb_running) {
    esp_ieee802154_sleep();
    this->zb_running = false;
  }
  if (this->zb_initialized) {
    esp_ieee802154_disable();
    this->zb_initialized = false;
  }
}

void ZBScan::zbSetChannel(uint8_t channel) {
  this->zb_channel = constrain(channel, ZIGBEE_MIN_CHANNEL, ZIGBEE_MAX_CHANNEL);
  esp_ieee802154_set_channel(this->zb_channel);
  esp_ieee802154_receive();
}

void ZBScan::zbChannelHop() {
  uint8_t next = this->zb_channel + 1;
  if (next > ZIGBEE_MAX_CHANNEL)
    next = ZIGBEE_MIN_CHANNEL;
  this->zbSetChannel(next);
}

// MAC-header decoder. Ported from IEEE802154Sniffer::_decodeMac(), reduced to
// the addressing/beacon fields Marauder records.
bool ZBScan::zbDecodeMac(const ZbSnifferFrame &raw, ZbFrameInfo &info) {
  const uint8_t *p   = raw.data;
  const uint8_t  len = raw.len;
  if (len < 3)
    return false;

  info.rssi    = raw.rssi;
  info.lqi     = raw.lqi;
  info.channel = raw.channel;
  info.isZigbee = false;
  info.hasNwk   = false;
  info.beaconSeen = false;
  info.beaconAssocPermit = false;
  info.beaconRouterCapacity = false;
  info.beaconEndDevCapacity = false;
  info.srcExtended = 0;
  info.dstExtended = 0;
  info.nwkSrc = 0xFFFF;
  info.nwkDst = 0xFFFF;

  uint16_t fc      = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
  info.frameType   = fc & ZB_FC_FRAME_TYPE_MASK;
  info.srcAddrMode = (fc >> 14) & 0x03;
  info.dstAddrMode = (fc >> 10) & 0x03;
  bool panComp     = (fc >> 6) & 0x01;
  info.seqNum      = p[2];
  info.panId       = 0xFFFF;
  info.macSrc      = 0xFFFF;
  info.macDst      = 0xFFFF;
  info.macSecurityEnabled = (fc >> 3) & 0x01;

  uint8_t off = 3;

  // Destination PAN + address
  if (info.dstAddrMode == ZB_ADDR_MODE_SHORT && off + 4 <= len) {
    info.panId  = (uint16_t)p[off] | ((uint16_t)p[off+1] << 8);
    info.macDst = (uint16_t)p[off+2] | ((uint16_t)p[off+3] << 8);
    off += 4;
  } else if (info.dstAddrMode == ZB_ADDR_MODE_EXTENDED && off + 10 <= len) {
    info.panId = (uint16_t)p[off] | ((uint16_t)p[off+1] << 8);
    off += 2;
    info.dstExtended = 0;
    for (int i = 0; i < 8; i++)
      info.dstExtended |= ((uint64_t)p[off+i] << (8*i));
    info.macDst = 0xFFFE;
    off += 8;
  }

  // Source address
  if (info.srcAddrMode == ZB_ADDR_MODE_SHORT) {
    if (!panComp && off + 2 <= len)
      off += 2;                          // skip src PAN
    if (off + 2 <= len) {
      info.macSrc = (uint16_t)p[off] | ((uint16_t)p[off+1] << 8);
      off += 2;
    }
  } else if (info.srcAddrMode == ZB_ADDR_MODE_EXTENDED) {
    if (!panComp && off + 2 <= len)
      off += 2;
    if (off + 8 <= len) {
      info.srcExtended = 0;
      for (int i = 0; i < 8; i++)
        info.srcExtended |= ((uint64_t)p[off+i] << (8*i));
      info.macSrc = 0xFFFE;
      off += 8;
    }
  }

  // Payload decode
  if (off < len && info.frameType == ZB_FC_FRAME_TYPE_DATA) {
    const uint8_t *payload = &p[off];
    uint8_t payLen = len - off;
    if (payLen >= 8) {
      uint8_t nwkType = payload[0] & 0x03;
      if (nwkType == ZB_NWK_TYPE_DATA || nwkType == ZB_NWK_TYPE_CMD) {
        info.isZigbee = true;
        this->zbDecodeNwk(payload, payLen, info);
      }
    }
  } else if (info.frameType == ZB_FC_FRAME_TYPE_BEACON && off + 4 <= len) {
    uint16_t superframe = (uint16_t)p[off] | ((uint16_t)p[off+1] << 8);
    info.beaconAssocPermit = (superframe >> 15) & 0x01;
    info.beaconSeen = true;
    off += 2;
    off += 2;                            // GTS spec + pending address spec
    // Zigbee beacon payload: protocol ID 0x00, then stack profile / caps
    if (off + 3 <= len && p[off] == 0x00) {
      off++;
      off++;                             // stack profile / protocol version
      uint8_t cap = p[off++];
      info.beaconRouterCapacity = (cap >> 2) & 0x01;
      info.beaconEndDevCapacity = (cap >> 7) & 0x01;
    }
  }

  return true;
}

// Zigbee NWK decoder. Ported from IEEE802154Sniffer::_decodeZigbeeNwk(),
// reduced to the source/destination network addresses Marauder tracks.
bool ZBScan::zbDecodeNwk(const uint8_t *p, uint8_t len, ZbFrameInfo &info) {
  if (len < 8)
    return false;

  uint16_t nwkFc = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
  info.zbNwkType = nwkFc & 0x03;
  info.nwkDst    = (uint16_t)p[2] | ((uint16_t)p[3] << 8);
  info.nwkSrc    = (uint16_t)p[4] | ((uint16_t)p[5] << 8);
  info.hasNwk    = true;

  uint8_t off = 8;

  if ((nwkFc & ZB_NWK_FC_EXT_DST) && off + 8 <= len) {
    uint64_t extDst = 0;
    memcpy(&extDst, &p[off], 8);
    if (info.dstExtended == 0)
      info.dstExtended = extDst;
    off += 8;
  }
  if ((nwkFc & ZB_NWK_FC_EXT_SRC) && off + 8 <= len) {
    uint64_t extSrc = 0;
    memcpy(&extSrc, &p[off], 8);
    if (info.srcExtended == 0)
      info.srcExtended = extSrc;
    off += 8;
  }

  return true;
}

// ---------------------------------------------------------------------------
// Scan entry points, driven by WiFiScan's StartScan / StopScan / main.
// ---------------------------------------------------------------------------
void ZBScan::RunZigbeeScan(uint8_t scan_mode, uint16_t color) {
  (void)scan_mode;
  (void)color;

  // Fresh node list for this run, same as RunAPScan recreates access_points.
  this->clearZigbeeNodes();

  this->zb_frame_count  = 0;
  this->zb_zigbee_count = 0;

  this->zb_channel = ZIGBEE_MIN_CHANNEL;
  if (!this->zbRadioInit(this->zb_channel) || !this->zbRadioStart()) {
    Serial.println(F("[ZBScan] Failed to start 802.15.4 radio"));
    return;
  }

  this->zb_last_hop       = millis();
  this->zb_last_ui_update = millis();
  Serial.println(F("[ZBScan] Zigbee sniffing started"));
}

void ZBScan::StopZigbeeScan() {
  this->zbRadioStop();
  Serial.println(F("[ZBScan] Zigbee sniffing stopped"));
}

void ZBScan::zigbeeLoop(uint32_t currentTime) {
  if (!this->zb_running)
    return;

  // Drain the RX queue and fold each frame into the node/connection list.
  ZbSnifferFrame raw;
  while (xQueueReceive(zb_rx_queue, &raw, 0) == pdTRUE) {
    this->zb_frame_count++;

    ZbFrameInfo info = {};
    if (this->zbDecodeMac(raw, info)) {
      if (info.isZigbee)
        this->zb_zigbee_count++;

      int before = (zigbee_nodes != nullptr) ? zigbee_nodes->size() : 0;
      this->zbProcessFrame(info);
      int after = (zigbee_nodes != nullptr) ? zigbee_nodes->size() : 0;

      if (after > before && !zb_is_bcast(info.macSrc)) {
        ZigbeeNode n = zigbee_nodes->get(after - 1);
        Serial.printf("[ZBScan] Node 0x%04X PAN 0x%04X ch %u rssi %d\n",
                      n.short_addr, n.pan_id, n.channel, n.rssi);
        #ifdef HAS_SCREEN
          String line = "#" + String(after - 1) + " 0x" + String(n.short_addr, HEX) +
                        " PAN 0x" + String(n.pan_id, HEX) +
                        " ch" + String(n.channel) + " " + String(n.rssi);
          display_obj.display_buffer->add(line);
        #endif
      }
    }
  }

  // Hop one channel each HOP_DELAY, like the WiFi sniffers.
  if (currentTime - this->zb_last_hop >= (HOP_DELAY * 2)) {
    this->zb_last_hop = millis();
    this->zbChannelHop();
  }
}

#endif // HAS_IEEE802154
