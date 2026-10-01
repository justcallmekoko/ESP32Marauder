#pragma once

#ifndef ZBScan_h
#define ZBScan_h

// ZBScan - IEEE 802.15.4 / Zigbee promiscuous sniffer for the ESP32 Marauder.
//
// The frame parsing and node-tracking logic here is ported from the standalone
// ESP32-C6 sniffer in https://github.com/evilpete/C6_zigbee
// (src/IEEE802154Sniffer.cpp - IEEE802154Sniffer class), trimmed down to the
// pieces the Marauder needs and reshaped to match the Marauder's conventions:
// discovered Zigbee nodes and the links between them are kept in a LinkedList
// of ZigbeeNode records, the same way WiFiScan keeps APs in a LinkedList of
// AccessPoint records.
//
// Only the ESP32-C5 / ESP32-C6 / ESP32-H2 have an 802.15.4 radio, so the radio-facing code
// is compiled only when HAS_ZIGBEE is defined (see configs.h). On any other
// board the class still exists - so WiFiScan can inherit from it unconditionally
// - but its scan entry points are no-ops.

#include "configs.h"

#ifdef HAS_ZIGBEE

#include <Arduino.h>
#include <LinkedList.h>
#include <stdint.h>

// ifdef HAS_ZIGBEE
  #include "freertos/FreeRTOS.h"
  #include "freertos/queue.h"
  #include "esp_ieee802154.h"
  #include "esp_ieee802154_types.h"
  #include "esp_mac.h"
// endif

// Scan-mode identifiers, consistent with the WIFI_SCAN_* / BT_SCAN_* numbering
// used throughout WiFiScan. 200+ is unused by any existing mode.
#define ZIGBEE_SCAN_ALL 200

// 802.15.4 channels for the 2.4GHz band run 11..26.
#define ZIGBEE_MIN_CHANNEL      11
#define ZIGBEE_MAX_CHANNEL      26
#define ZIGBEE_DEFAULT_CHANNEL  11

#define ZIGBEE_MAX_FRAME_LEN    128
#define ZIGBEE_QUEUE_DEPTH      16

// IEEE 802.15.4 MAC frame-control fields
#define ZB_FC_FRAME_TYPE_MASK   0x07
#define ZB_FC_FRAME_TYPE_BEACON 0x00
#define ZB_FC_FRAME_TYPE_DATA   0x01
#define ZB_FC_FRAME_TYPE_ACK    0x02
#define ZB_FC_FRAME_TYPE_MACCMD 0x03

#define ZB_ADDR_MODE_NONE       0x00
#define ZB_ADDR_MODE_SHORT      0x02
#define ZB_ADDR_MODE_EXTENDED   0x03

// Zigbee NWK layer
#define ZB_NWK_TYPE_DATA        0x00
#define ZB_NWK_TYPE_CMD         0x01
#define ZB_NWK_FC_SECURITY      (1 << 9)
#define ZB_NWK_FC_SOURCE_ROUTE  (1 << 10)
#define ZB_NWK_FC_EXT_DST       (1 << 12)
#define ZB_NWK_FC_EXT_SRC       (1 << 11)

// How a discovered node appears to behave on the mesh.
enum ZbDeviceType : uint8_t {
  ZB_DEV_UNKNOWN     = 0,
  ZB_DEV_COORDINATOR = 1,
  ZB_DEV_ROUTER      = 2,
  ZB_DEV_END_DEVICE  = 3,
};

// A single raw frame handed from the RX ISR to the processing loop.
struct ZbSnifferFrame {
  uint8_t  data[ZIGBEE_MAX_FRAME_LEN];
  uint8_t  len;
  int8_t   rssi;
  uint8_t  lqi;
  uint8_t  channel;
};

// Decoded fields we care about for one frame.
struct ZbFrameInfo {
  uint8_t  frameType;
  uint16_t panId;
  uint16_t macSrc;
  uint16_t macDst;
  uint64_t srcExtended;
  uint64_t dstExtended;
  uint8_t  srcAddrMode;
  uint8_t  dstAddrMode;
  uint8_t  seqNum;
  int8_t   rssi;
  uint8_t  lqi;
  uint8_t  channel;

  bool     macSecurityEnabled;

  bool     macFramePending;
  bool     macAckRequest;
  uint8_t  macCmd;            // MAC command id (frameType == MAC_CMD), else 0

  bool     isZigbee;
  uint8_t  zbNwkType;
  uint8_t  zbNwkCmd;          // Zigbee NWK command id (zbNwkType == CMD), else 0
  bool     zbNwkSecurity;
  bool     beaconAssocPermit;
  bool     beaconRouterCapacity;
  bool     beaconEndDevCapacity;
  bool     beaconSeen;

  uint16_t nwkSrc;
  uint16_t nwkDst;
  bool     hasNwk;
};

// A discovered Zigbee / 802.15.4 node. Modeled on WiFiScan's AccessPoint: a
// plain struct held in a LinkedList, carrying a nested LinkedList of the peers
// it has been seen talking to (by short address) - the "connections".
struct ZigbeeNode {
  uint16_t   short_addr;
  uint16_t   pan_id;
  uint64_t   ext_addr;        // 0 if the extended/MAC address has not been seen
  int8_t     rssi;
  uint8_t    lqi;
  uint8_t    channel;
  ZbDeviceType device_type;
  bool       selected;
  bool       beacon_seen;
  bool       assoc_permit;
  uint16_t   packets;
  uint32_t   first_seen_ms;
  uint32_t   last_seen_ms;
  LinkedList<uint16_t>* connections;   // short addrs this node has been seen with
};

// Global node list, mirroring WiFiScan's global `access_points`.
extern LinkedList<ZigbeeNode>* zigbee_nodes;

class ZBScan {
  public:
    ZBScan();

    // Lifecycle, wired into WiFiScan's StartScan / StopScan / main dispatch.
    void RunZigbeeScan(uint8_t scan_mode, uint16_t color);
    void StopZigbeeScan();
    void zigbeeLoop(uint32_t currentTime);

    // Node-list maintenance (callable regardless of radio availability).
    int  clearZigbeeNodes();
    size_t retainedZigbeeNodeCount() const;

    uint8_t getZigbeeChannel() const { return this->zb_channel; }
    bool    zigbeeInitialized() const { return this->zb_initialized; }

    // Lock the sniffer to one channel (11..26) instead of hopping. Call BEFORE
    // StartScan(ZIGBEE_SCAN_ALL). 0 restores full-band channel hopping.
    void    setZigbeeStartChannel(uint8_t ch) { this->zb_start_channel = ch; }

    // Per-frame serial logging (every beacon / command / data / broadcast).
    // On by default - this is what makes generated traffic visible.
    bool    zb_verbose = true;
    bool    zb_show_bcast = true;   // also log broadcast-addressed frames

    uint32_t zb_frame_count = 0;
    uint32_t zb_zigbee_count = 0;

  protected:
    uint8_t zb_channel = ZIGBEE_DEFAULT_CHANNEL;
    uint8_t zb_start_channel = 0;  // 0 = hop; 11..26 = lock to that channel
    bool    zb_channel_hop = true;
    bool    zb_initialized = false;
    bool    zb_running = false;

    uint32_t zb_last_hop = 0;
    uint32_t zb_last_ui_update = 0;

    void zbPrintFrame(const ZbFrameInfo &info);
    static const char *zbFrameTypeName(uint8_t frameType);
    static const char *zbMacCmdName(uint8_t cmd);
    static const char *zbNwkCmdName(uint8_t cmd);

    // Add/refresh a node and (for source frames) record a connection to macDst.
    ZigbeeNode* zbUpdateNode(const ZbFrameInfo &info, uint16_t addr, bool is_src);
    int  zbFindNode(uint16_t short_addr);
    void zbProcessFrame(const ZbFrameInfo &info);

 //ifdef HAS_ZIGBEE
    bool zbRadioInit(uint8_t channel);
    bool zbRadioStart();
    void zbRadioStop();
    void zbSetChannel(uint8_t channel);
    void zbChannelHop();
    bool zbDecodeMac(const ZbSnifferFrame &raw, ZbFrameInfo &info);
    bool zbDecodeNwk(const uint8_t *p, uint8_t len, ZbFrameInfo &info);

  public:
    // Called from the extern "C" 802.15.4 receive-done hook (ZBScan.cpp).
    static void rxCallback(uint8_t *frame, esp_ieee802154_frame_info_t *fi);

  protected:
    static QueueHandle_t zb_rx_queue;
 // endif
};

#endif // HAS_ZIGBEE
#endif // ZBScan_h
