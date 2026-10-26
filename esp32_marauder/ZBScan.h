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
// Max RX frames decoded per zigbeeLoop() call. Bounding the drain keeps the
// main loop cycling (and the -/+/HOP/X touch buttons responsive) under heavy
// 802.15.4 traffic; leftover frames are handled on the next cycle. At typical
// loop rates this still processes far more frames/sec than any real network
// produces, so nothing meaningful is dropped.
#define ZB_MAX_FRAMES_PER_LOOP  6

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

  // Byte offset (into the raw 802.15.4 frame) at which the MAC payload / NWK
  // layer begins. Set by zbDecodeMac; the key-capture path needs it to walk
  // the NWK + APS headers over the real on-air bytes.
  uint8_t  macPayloadOffset;
};

// ---------------------------------------------------------------------------
// Zigbee key material (network key interception). Ported from the C6_zigbee
// IEEE802154Sniffer / ZbKeyCapture classes. A captured or preloaded key is a
// plain ZbKey held in a global LinkedList, mirroring zigbee_nodes.
// ---------------------------------------------------------------------------
#define ZIGBEE_KEY_LEN        16
#define ZB_MAX_PENDING_JOINS  4

// --- Active TX / simulation (ZbJoiner / ZbAttack / ZbPing) ------------------
// 802.15.4 MAC command ids and association-capability bits for the transmit
// path. The radio shares the 2.4GHz front end, so each TX stops RX, transmits,
// then resumes RX.
#define ZB_MAC_CMD_ASSOC_REQUEST    0x01
#define ZB_MAC_CMD_DATA_REQUEST     0x04
#define ZB_ASSOC_CAP_ALLOC_ADDR     0x80   // request a short address
#define ZB_ASSOC_CAP_SIMPLE_DEVICE  ZB_ASSOC_CAP_ALLOC_ADDR  // RFD, battery, sleepy

enum ZbKeyType : uint8_t {
  ZB_KEY_TCLK    = 0,   // Trust Center Link Key (used to decrypt Transport Key)
  ZB_KEY_NETWORK = 1,   // captured Zigbee network key
  ZB_KEY_APP     = 2,
};

struct ZbKey {
  ZbKeyType type;
  uint8_t   key[ZIGBEE_KEY_LEN];
  uint8_t   seqNum;
  uint32_t  capturedAt_ms;
  char      label[20];
};

// Tracks a device mid-join (association response seen) so a following APS
// Transport Key frame can be matched to it.
struct ZbJoinState {
  uint16_t shortAddr;
  uint64_t extAddr;
  uint32_t assocTime_ms;
  bool     active;
};

// Global captured/known-key list, mirroring the global `zigbee_nodes`.
extern LinkedList<ZbKey>* zigbee_keys;

// --- ZDO ping / find_lock (ZbPing) -----------------------------------------
// Application-layer reachability (ZDO Node_Desc_req/rsp) and door-lock
// discovery (Active_EP + Simple_Desc enumeration). Requires an active join
// (own short address) and a captured network key. Door-lock ACTUATION
// (lock/unlock) is deliberately NOT ported. Ported from C6_zigbee ZbPing.
#define ZB_ZDO_PROFILE_ID       0x0000
#define ZB_ZDO_EP               0x00
#define ZB_ZDO_NODE_DESC_REQ    0x0002
#define ZB_ZDO_NODE_DESC_RSP    0x8002
#define ZB_ZDO_SIMPLE_DESC_REQ  0x0004
#define ZB_ZDO_SIMPLE_DESC_RSP  0x8004
#define ZB_ZDO_ACTIVE_EP_REQ    0x0005
#define ZB_ZDO_ACTIVE_EP_RSP    0x8005
#define ZB_CLUSTER_DOOR_LOCK    0x0101

#define ZB_PING_MAX_PENDING       6
#define ZB_NODE_DESC_CACHE        16
#define ZB_LOCK_MAX_ENDPOINTS     16
#define ZB_LOCK_MAX_SWEEP         32
#define ZB_LOCK_MAX_INFO          8
#define ZB_PING_TIMEOUT_MS        3000
#define ZB_LOCK_STEP_TIMEOUT_MS   2500

struct ZbPendingPing {
  bool     active;
  uint16_t target;
  uint16_t pan;
  uint8_t  zdoSeq;
  uint32_t sentAt_ms;
};

struct ZbNodeDescCache {
  bool     valid;
  uint16_t addr;
  uint8_t  logicalType;
  uint8_t  macCapability;
  uint16_t manufacturerCode;
  uint8_t  maxBufferSize;
  uint16_t maxInTransferSize;
  uint16_t serverMask;
  uint16_t maxOutTransferSize;
  uint8_t  descCapability;
};

struct ZbLockInfo {
  bool     valid;
  uint16_t addr;
  uint16_t pan;
  uint8_t  endpoint;
  uint16_t profile;
};

enum ZbLockScanPhase : uint8_t {
  ZB_LOCK_IDLE = 0, ZB_LOCK_WAIT_ACTIVE_EP = 1, ZB_LOCK_WAIT_SIMPLE_DESC = 2
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

    // --- Network-key capture ------------------------------------------------
    // Watch the APS Transport Key sent to a joining device and recover the
    // network key by trying each known Trust Center Link Key (TCLK). Enabled
    // by default; the default TCLKs (ZigBeeAlliance09, eISY) are preloaded at
    // scan start. Captured keys land in the global `zigbee_keys` list.
    bool     zb_key_capture = true;

    // Set true whenever a new key is captured/added, so WiFiScan can persist
    // the key list to SD (polled + cleared from the ZIGBEE main() dispatch).
    bool     zb_keys_dirty = false;

    void   zbLoadDefaultKeys();            // preload known TCLKs (idempotent)
    int    clearZigbeeKeys();              // free + empty the key list
    bool   zbAddKey(ZbKeyType type, const uint8_t *key16,
                    uint8_t seqNum = 0, const char *label = nullptr);
    ZbKey *zbFindNetworkKey(uint8_t seqNum);
    size_t zigbeeKeyCount() const;

    // --- Transmit primitives (shared 2.4GHz front end: stop-RX / TX / RX) ---
    bool zbSendRawFrame(const uint8_t *frame, uint8_t frameLen);
    bool zbSendBeaconRequest();
    bool zbSendAssociationRequest(uint16_t dstPan, uint16_t dstShortAddr,
                                  uint8_t capabilityInfo = ZB_ASSOC_CAP_SIMPLE_DEVICE);
    bool zbSendDataRequest(uint16_t dstPan, uint16_t dstShortAddr,
                           bool useOwnShortAddr, uint16_t ownShortAddr = 0xFFFE);
    bool zbSendAck(uint8_t seqNum);

    // --- Own extended address (EUI64) + spoofing ----------------------------
    uint64_t zbGetOwnEUI64() const { return this->zb_ownEUI64; }
    uint64_t zbGetHwEUI64()  const { return this->zb_hwEUI64; }
    void     zbSetOwnEUI64(uint64_t eui64);   // program a spoofed source EUI64
    void     zbRestoreOwnEUI64();             // revert to the hardware MAC
    bool     zbIsEUI64Spoofed() const { return this->zb_ownEUI64 != this->zb_hwEUI64; }

    // --- ACK-attack (spoofed MAC ACK injected from the RX ISR) --------------
    void     zbStartAckAttack(uint16_t target);
    void     zbStopAckAttack();
    bool     zbAckAttackActive() const { return zb_ackAttackActive; }
    uint16_t zbAckAttackTarget() const { return zb_ackAttackTarget; }
    uint32_t zbAckAttackCount()  const { return zb_ackAttackCount; }

    // --- Join a network as an end device (ZbJoiner) -------------------------
    // Sends a MAC Association Request to a permit-join parent, then polls for
    // the Association Response; on success we get a short address and the
    // coordinator's following Transport Key is captured by the key pipeline.
    enum ZbJoinPhase : uint8_t {
      ZB_JOIN_IDLE = 0, ZB_JOIN_WAIT = 1, ZB_JOIN_ASSOCIATED = 2, ZB_JOIN_FAILED = 3
    };
    bool        zbJoinStart(uint16_t parentShort, uint16_t parentPan,
                            uint8_t cap = ZB_ASSOC_CAP_SIMPLE_DEVICE);
    void        zbJoinAbort();
    ZbJoinPhase zbJoinGetPhase()  const { return this->zb_joinPhase; }
    uint16_t    zbJoinShortAddr() const { return this->zb_joinAssignedShort; }
    bool        zbJoinAssociated() const { return this->zb_joinPhase == ZB_JOIN_ASSOCIATED; }
    void        zbJoinPrintStatus() const;

    // --- Offensive probes (ZbAttack) - authorised testing of YOUR net only --
    // insecure_rejoin: impersonate a known device (spoof its EUI64) and send an
    // unsecured NWK Rejoin Request; a permissive Trust Center answers and
    // transports the network key under the default TCLK, which the key pipeline
    // then recovers. ack_attack: see zbStartAckAttack/zbStopAckAttack.
    bool zbInsecureRejoin(uint16_t targetShortAddr);

    // --- ZDO ping / door-lock discovery (ZbPing) ----------------------------
    // Require zbJoinAssociated() + a captured network key. No lock actuation.
    bool   zbPing(uint16_t targetShortAddr, uint16_t targetPan);
    bool   zbFindLock(uint16_t targetShortAddr, uint16_t targetPan);
    void   zbFindLockSweepAll();
    void   zbPrintLockStatus() const;
    ZbKey *zbFindLatestNetworkKey();   // most recent captured NETWORK key

  protected:
    uint8_t zb_channel = ZIGBEE_DEFAULT_CHANNEL;
    uint8_t zb_start_channel = 0;  // 0 = hop; 11..26 = lock to that channel
    bool    zb_channel_hop = true;
    bool    zb_initialized = false;
    bool    zb_running = false;

    uint32_t zb_last_hop = 0;
    uint32_t zb_last_ui_update = 0;

    // Fill colour of the one-line on-screen status strip (the scan-tile colour,
    // stashed by WiFiScan::StartScan so zbDrawStatusLabel can redraw the strip
    // live as the channel hops). 0 == RGB565 black, a safe default pre-scan.
    uint16_t zb_color = 0;

    // Redraw the status strip ("Zigbee HOP/LOCK CH n") in the same rect the
    // scan title uses. No-op on screenless builds.
    void zbDrawStatusLabel();

    void zbPrintFrame(const ZbFrameInfo &info);
    static const char *zbFrameTypeName(uint8_t frameType);
    static const char *zbMacCmdName(uint8_t cmd);
    static const char *zbNwkCmdName(uint8_t cmd);

    // Add/refresh a node and (for source frames) record a connection to macDst.
    ZigbeeNode* zbUpdateNode(const ZbFrameInfo &info, uint16_t addr, bool is_src);
    int  zbFindNode(uint16_t short_addr);
    void zbProcessFrame(const ZbFrameInfo &info);

    // --- Key-capture internals (ported from C6_zigbee ZbKeyCapture) ---------
    ZbJoinState zb_joins[ZB_MAX_PENDING_JOINS];
    uint64_t    zb_coordEUI64 = 0;   // coordinator EUI64, learned from traffic
    bool        zb_keys_loaded = false;

    // Own extended address: zb_hwEUI64 is the real hardware MAC; zb_ownEUI64 is
    // what outgoing frames use (== hardware unless spoofed). Integer convention
    // matches srcExtended/dstExtended (big-integer canonical form).
    uint64_t    zb_ownEUI64 = 0;
    uint64_t    zb_hwEUI64  = 0;
    uint8_t     zb_hwEUI64Bytes[8] = {};

    // ACK-attack state is touched from the RX ISR, hence static + volatile.
    static volatile bool     zb_ackAttackActive;
    static volatile uint16_t zb_ackAttackTarget;
    static volatile uint32_t zb_ackAttackCount;

    // Join state machine (ZbJoiner) + attack NWK sequence counter.
    ZbJoinPhase zb_joinPhase        = ZB_JOIN_IDLE;
    uint16_t    zb_joinParentShort  = 0xFFFF;
    uint16_t    zb_joinParentPan    = 0xFFFF;
    uint16_t    zb_joinAssignedShort = 0xFFFE;
    uint32_t    zb_joinStartMs      = 0;
    uint32_t    zb_joinLastPollMs   = 0;
    uint8_t     zb_joinPollCount    = 0;
    uint8_t     zb_attackNwkSeq     = 0;

    void zbJoinUpdate();   // drive polling / timeout each loop
    void zbJoinProcessFrame(const ZbFrameInfo &info, const uint8_t *raw,
                            uint8_t rawLen, uint8_t macOff);
    bool zbSendRejoinRequest(uint16_t coordShort, uint16_t pan, uint64_t spoofEui64);
    static const char *zbAssocStatusName(uint8_t status);

    // --- ZbPing state + internals -------------------------------------------
    ZbPendingPing   zb_pending[ZB_PING_MAX_PENDING];
    ZbNodeDescCache zb_descCache[ZB_NODE_DESC_CACHE];
    uint32_t        zb_nwkFrameCounter = 0;
    uint8_t         zb_pingMacSeq = 0, zb_pingNwkSeq = 0, zb_apsCounter = 0, zb_zdoSeq = 0;
    bool            zb_pingSeeded = false;

    ZbLockScanPhase zb_lockPhase = ZB_LOCK_IDLE;
    uint16_t        zb_lockTarget = 0xFFFF, zb_lockPan = 0xFFFF;
    uint8_t         zb_lockEndpoints[ZB_LOCK_MAX_ENDPOINTS];
    uint8_t         zb_lockEpCount = 0, zb_lockEpIndex = 0;
    uint32_t        zb_lockSentMs = 0;
    bool            zb_lockFound = false, zb_lockSweeping = false;
    uint16_t        zb_sweepQueue[ZB_LOCK_MAX_SWEEP];
    uint8_t         zb_sweepLen = 0, zb_sweepIdx = 0;
    uint16_t        zb_sweepPan = 0xFFFF;
    ZbLockInfo      zb_locks[ZB_LOCK_MAX_INFO];

    void zbPingSeed();                 // seed counters from HWRNG (once)
    void zbPingUpdate();               // pending/lock-scan timeouts each loop
    void zbPingProcessFrame(const ZbFrameInfo &info, const uint8_t *raw,
                            uint8_t rawLen, uint8_t macOff);
    bool zbSendApsData(uint8_t dstEp, uint8_t srcEp, uint16_t cluster,
                       uint16_t profile, const uint8_t *payload, uint8_t payloadLen,
                       uint16_t target, uint16_t pan);
    int  zbSendZdoReq(uint16_t cluster, uint16_t target, uint16_t pan,
                      const uint8_t *extra, uint8_t extraLen);
    bool zbNwkEncryptAndSend(uint16_t dstPan, uint16_t dstShort,
                             const uint8_t *apsFrame, uint8_t apsLen);
    bool zbNwkDecrypt(const uint8_t *nwk, uint8_t nwkLen, uint16_t nwkSrc,
                      uint8_t *plainOut, uint8_t &plainLen);
    bool zbFindAuxHeader(const uint8_t *nwk, uint8_t len, uint16_t nwkFc,
                         uint8_t &auxOffset);
    void zbReportNodeDesc(uint16_t addr, const uint8_t *nodeDesc, uint32_t rtt_ms);
    static const char *zbLogicalTypeName(uint8_t t);
    bool zbStartLockScan(uint16_t target, uint16_t pan);
    void zbSendSimpleDescReq(uint8_t endpoint);
    void zbHandleActiveEpRsp(uint16_t src, const uint8_t *zdo, uint8_t zdoLen);
    void zbHandleSimpleDescRsp(uint16_t src, const uint8_t *zdo, uint8_t zdoLen);
    void zbFinishLockTarget();
    void zbNextSweepOrIdle();
    ZbPendingPing   *zbFindPending(uint16_t target);
    ZbPendingPing   *zbAllocPending(uint16_t target, uint16_t pan, uint8_t zdoSeq);
    ZbNodeDescCache *zbFindCache(uint16_t addr);
    ZbNodeDescCache *zbAllocCache(uint16_t addr);
    void             zbRecordLockInfo(uint16_t addr, uint16_t pan, uint8_t ep, uint16_t profile);

    // Feed one fully-decoded frame (plus its raw bytes) through the key-capture
    // pipeline. rawLen/macOff describe the on-air frame in `raw`.
    void zbKeyCaptureFrame(const ZbFrameInfo &info, const uint8_t *raw,
                           uint8_t rawLen, uint8_t macOff);
    void zbExpireJoins(uint32_t timeout_ms = 30000);
    void zbPrintKey(const ZbKey &key);   // inline (screen + serial) report

    ZbJoinState *zbFindJoin(uint16_t addr);
    ZbJoinState *zbAllocJoin(uint16_t addr);
    void         zbFreeJoin(ZbJoinState *j);

    void zbHandleAssocResponse(const ZbFrameInfo &info);
    bool zbHandleTransportKey(const ZbFrameInfo &info,
                              const uint8_t *nwkPayload, uint8_t nwkLen);
    bool zbSkipNwkHeader(const uint8_t *p, uint8_t len, uint8_t &apsOffset);
    bool zbDecryptApsPayload(const uint8_t *nwkPayload, uint8_t nwkLen,
                             uint8_t apsOffset, const uint8_t *key,
                             uint64_t knownExtSrc,
                             uint8_t *plaintextOut, uint8_t &plaintextLen);
    bool zbParseTransportKey(const uint8_t *aps, uint8_t apsLen,
                             uint8_t *networkKeyOut, uint8_t &seqNumOut);
    bool zbCcmDecrypt(const uint8_t *key, const uint8_t *nonce,
                      const uint8_t *aad, uint8_t aadLen,
                      const uint8_t *ciphertext, uint8_t ciphertextLen,
                      uint8_t *plaintext, const uint8_t *mic, uint8_t micLen);

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
