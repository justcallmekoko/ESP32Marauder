#include "GhostLinkBridge.h"

#ifdef HAS_BLE_UART_BRIDGE

#include <NimBLEDevice.h>
#include <WiFi.h>
#include <vector>
#include <string>

// Marauder already pulls in a CommandLine object; unrecognized control text is
// forwarded to it so the board still responds over BLE to ordinary commands.
#include "CommandLine.h"
extern CommandLine cli_obj;

// WiFiScan gives us the canonical bracketed auth token (e.g. "[WPA2_PSK]") for
// the Biscuit DATA:AP line, matching exactly what Marauder/Biscuit emit.
extern WiFiScan wifi_scan_obj;

#ifdef HAS_BATTERY
#include "BatteryInterface.h"
extern BatteryInterface battery_obj;
#endif

// Clean-room Biscuit Manager line-protocol codec (MIT, transport-agnostic).
#include "BiscuitLink.h"

// ---------------------------------------------------------------------------
// GhostLink / WardriveESP wire protocol (clean-room, from documented spec).
//
//   12-byte little-endian frame header:
//     [0]  'G'
//     [1]  'B'
//     [2]  version      = 1
//     [3]  frame type   (see GL_TYPE_*)
//     [4]  status       (0 = OK)
//     [5]  flags        (CMD: 0x01 = first, 0x02 = more)
//     [6..9]   reply/command id (uint32 LE)
//     [10..11] payload length   (uint16 LE)
//   ...followed by <payload length> payload bytes.
//
// The companion app concatenates DATA-frame payloads and then splits the
// accumulated bytes on '\n', so a single record may span several BLE frames.
//
// This board also exposes a SECOND service on the same server: the Biscuit
// Manager GATT surface (see BiscuitLink.h). Both links share one NimBLE server
// and one Wi-Fi scan; each scanned AP is fanned out to whichever link(s) asked
// to stream, in that link's own format.
// ---------------------------------------------------------------------------

#define GL_VERSION      0x01
#define GL_TYPE_CMD     0x01
#define GL_TYPE_ACK     0x02
#define GL_TYPE_DATA    0x03
#define GL_TYPE_END     0x04
#define GL_TYPE_ERR     0x05

#define GL_STATUS_OK    0x00

// Advertised identity. Apps (e.g. WarDriveGo) match on the service UUID, not
// the name, so we keep the WardriveESP service/char UUIDs for discovery but
// advertise an honest Marauder name and report ourselves as a Marauder in
// WD:CAPS -- no pretending to be WardriveESP firmware.
#define GL_SVC_UUID     "47686F73-7445-5350-4272-696467655376"
#define GL_RX_UUID      "0147686F-7374-4553-5042-726964675852"  // phone -> C5
#define GL_TX_UUID      "0247686F-7374-4553-5042-726964675854"  // C5 -> phone
#define GL_CTRL_UUID    "0347686F-7374-4553-5042-72694C525443"  // phone -> C5

// Honest capability advertisement for what this bridge actually implements.
// Honest identity: board_name reports this as a Marauder, and fw is a sentinel
// (99.99.99) that sits above any real WardriveESP release, so WarDriveGo's
// version check never offers an OTA (which would otherwise try to flash real
// WardriveESP over this Marauder build). A fixed real version goes stale on the
// next WardriveESP release and the nag returns; the sentinel never does. There
// is no OTA characteristic either, so a mistaken OTA just fails.
#define GL_CAPS_LINE    "WD:CAPS fw=99.99.99 chip=esp32c5 bands=2g,5g ota=0 bt=0 gps=0 cap=0 usb=0 board_name=Marauder_WS_C5_35 link=ble chans=1,6,11,36,40,44,48,149,153,157,161"
#define GL_PLAN_CHANS   "1,6,11,36,40,44,48,149,153,157,161"

// Payload bytes per DATA frame. Kept well under the negotiated MTU (247) once
// the 12-byte header and the 3-byte ATT overhead are accounted for.
#define GL_PAYLOAD_MAX  180
#define GL_PREFERRED_MTU 247

// Pacing: don't launch Wi-Fi sweeps back-to-back; let touch/BLE breathe.
#define GL_SWEEP_GAP_MS   1500
#define GL_RECOVER_MS     1000

// AP dedup window: re-report a given BSSID at most once a minute so a board
// left streaming doesn't re-send the same networks on every ~1.5 s sweep.
// (Matches PURE-WARDRIVER's node behaviour; WarDriveGo dedups by BSSID anyway,
// so this is pure BLE-bandwidth savings.)
#define GL_DEDUP_N        64
#define GL_DEDUP_MS       60000UL

// RX routing: a write arrives either on the WardriveESP RX/CTRL characteristic
// or on the Biscuit CMD characteristic. The channel tag tells the loop which
// parser to use.
#define CH_WD             0
#define CH_BISCUIT        1

// Advertising: two 128-bit service UUIDs cannot share one 31-byte PDU, so while
// nobody is connected we flip the advertised UUID+name between the WardriveESP
// and Biscuit slots about this often (payload swapped live).
#define GL_ADV_ROTATE_MS  1280

// ---------------------------------------------------------------------------
// Shared state (file scope so the NimBLE callback objects and the bridge
// methods can see it). All NimBLE notify() calls happen from the main loop;
// callbacks only set flags / enqueue, which keeps the single radio core sane.
// ---------------------------------------------------------------------------
namespace {
  NimBLEServer*         s_server   = nullptr;
  NimBLECharacteristic* s_tx       = nullptr;

  // Biscuit link characteristics (second service on the same server).
  NimBLECharacteristic* s_bisc_cmd = nullptr;   // write  (phone -> C5)
  NimBLECharacteristic* s_bisc_rsp = nullptr;   // notify (C5 -> phone)
  NimBLECharacteristic* s_bisc_sta = nullptr;   // read   (status JSON)
  NimBLECharacteristic* s_bisc_set = nullptr;   // read/write (settings, unused)

  volatile bool s_connected        = false;
  volatile bool s_tx_subscribed    = false;
  volatile bool s_handshake_sent   = false;
  volatile bool s_advertise_want   = false;

  // Biscuit link state.
  volatile bool s_bisc_subscribed  = false;     // app enabled RSP notifications
  volatile bool s_bisc_greet       = false;     // send greeting on (re)subscribe
  bool     s_bisc_session     = false;          // a CMD:wardrive* is active
  bool     s_bisc_time_synced = false;          // TIME:SYNC seen (we ignore value)
  uint8_t  s_bisc_band        = 0;              // 0 all, 1 = 2.4 GHz, 2 = 5 GHz
  uint32_t s_bisc_time_req_ms = 0;              // TIME:REQUEST retry cadence

  SemaphoreHandle_t s_rx_mux = nullptr;         // guards s_rx_msgs
  struct GlRxMsg { uint8_t ch; std::string data; };
  std::vector<GlRxMsg> s_rx_msgs;               // RX writes pending for loop

  // Wardrive sweep state.
  // Every outgoing WardriveESP frame carries the "reply id" = the id of the
  // last command the phone sent (its CMD frame id), starting at 1 before any
  // command. The app waits for an ACK with that id and for DATA replies tagged
  // with it.
  uint32_t s_reply_id      = 1;

  bool     s_streaming     = false;             // WardriveESP (WarDriveGo) stream
  bool     s_scan_pending  = false;
  uint32_t s_last_sweep_ms = 0;
  uint32_t s_scans         = 0;
  uint32_t s_aps_total     = 0;
  uint32_t s_recover_ms    = 0;

  // Advertising rotation.
  char     s_adv_name[20]   = {0};              // "Marauder-XXXX"
  uint8_t  s_adv_slot       = 0;                // 0 = WardriveESP, 1 = Biscuit
  uint32_t s_last_rotate_ms = 0;

  // GAP device name (0x2A00) follows the active link: once the Biscuit app
  // subscribes to its Response characteristic the connected name reads
  // "Biscuit"; a WardriveESP TX-subscribe (and every disconnect) reverts it to
  // "Marauder-XXXX". A dual-protocol central (e.g. WarDriveGo, which speaks both
  // surfaces) classifies the device by this name after connecting, so a static
  // name makes it miscategorise the board. Callbacks only set the desired value;
  // the loop applies it (setDeviceName off the NimBLE host task).
  volatile uint8_t s_gap_want    = 0;           // 0 = wardrive name, 1 = biscuit
  uint8_t          s_gap_applied = 0xFF;         // force an apply on first pass

  uint8_t  s_frame[12 + GL_PAYLOAD_MAX];

  // BSSID -> last-sent time. "AA:BB:CC:DD:EE:FF" is 17 chars + NUL. LRU reuse
  // once full; cleared at the start of every wardrive session.
  struct GlDedup { char key[18]; uint32_t ms; };
  GlDedup s_dedup[GL_DEDUP_N];

  void rxEnqueue(uint8_t ch, const uint8_t* data, size_t len) {
    if (!data || !len || !s_rx_mux) return;
    std::string msg(reinterpret_cast<const char*>(data), len);
    // Callback runs in the NimBLE host task (not an ISR), so a blocking mutex
    // is safe here and avoids heap work inside a spinlock critical section.
    if (xSemaphoreTake(s_rx_mux, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (s_rx_msgs.size() < 16)         // bound the queue
        s_rx_msgs.push_back(GlRxMsg{ch, std::move(msg)});
      xSemaphoreGive(s_rx_mux);
    }
  }
}

// ---------------------------------------------------------------------------
// NimBLE callbacks
// ---------------------------------------------------------------------------
class GlServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* /*p*/, NimBLEConnInfo& /*info*/) override {
    s_connected        = true;
    s_handshake_sent   = false;    // re-greet each new central
    s_reply_id         = 1;        // spec: reply id starts at 1 before any command
    s_bisc_subscribed  = false;
    s_bisc_greet       = false;
    s_bisc_session     = false;
    s_bisc_time_synced = false;
    s_bisc_band        = 0;
    s_gap_want         = 0;        // default to the Marauder name until a link subscribes
  }
  void onDisconnect(NimBLEServer* /*p*/, NimBLEConnInfo& /*info*/, int /*reason*/) override {
    s_connected        = false;
    s_tx_subscribed    = false;
    s_streaming        = false;
    s_bisc_subscribed  = false;
    s_bisc_greet       = false;
    s_bisc_session     = false;
    s_bisc_time_synced = false;
    s_gap_want         = 0;        // revert the connected name on the next link
    s_gap_applied      = 0xFF;     // re-apply on reconnect
    s_advertise_want   = true;     // main loop re-arms advertising
  }
};

// Shared write handler for the WardriveESP RX/CTRL chars and the Biscuit CMD
// char; the channel is decided by which characteristic fired.
class GlRxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& /*info*/) override {
    NimBLEAttValue v = c->getValue();
    uint8_t ch = (c == s_bisc_cmd) ? CH_BISCUIT : CH_WD;
    rxEnqueue(ch, v.data(), v.length());
  }
};

class GlTxCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* /*c*/, NimBLEConnInfo& /*info*/, uint16_t subValue) override {
    s_tx_subscribed = (subValue != 0);
    if (s_tx_subscribed) {
      s_handshake_sent = false;    // greet on (re)subscribe
      s_gap_want       = 0;        // connected name -> Marauder
    }
  }
};

class GlBiscRspCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* /*c*/, NimBLEConnInfo& /*info*/, uint16_t subValue) override {
    s_bisc_subscribed = (subValue != 0);
    if (s_bisc_subscribed) {
      s_bisc_greet = true;         // TIME:REQUEST + STATUS sent from the loop
      s_gap_want   = 1;            // connected name -> Biscuit
    }
  }
};

static GlServerCallbacks s_server_cb;
static GlRxCallbacks     s_rx_cb;
static GlTxCallbacks     s_tx_cb;
static GlBiscRspCallbacks s_bisc_rsp_cb;

// ---------------------------------------------------------------------------
// Low-level frame TX (main-loop context only)
// ---------------------------------------------------------------------------
static void glSendFrame(uint8_t type, uint32_t cmd_id, uint8_t flags,
                        const uint8_t* payload, uint32_t plen) {
  if (!s_tx || !s_connected || !s_tx_subscribed) return;
  if (plen > GL_PAYLOAD_MAX) plen = GL_PAYLOAD_MAX;

  s_frame[0] = 'G';
  s_frame[1] = 'B';
  s_frame[2] = GL_VERSION;
  s_frame[3] = type;
  s_frame[4] = GL_STATUS_OK;
  s_frame[5] = flags;
  // command id: 4 bytes LE at offset 6
  s_frame[6] = (uint8_t)(cmd_id & 0xFF);
  s_frame[7] = (uint8_t)((cmd_id >> 8) & 0xFF);
  s_frame[8] = (uint8_t)((cmd_id >> 16) & 0xFF);
  s_frame[9] = (uint8_t)((cmd_id >> 24) & 0xFF);
  // payload length: 2 bytes LE at offset 10
  s_frame[10] = (uint8_t)(plen & 0xFF);
  s_frame[11] = (uint8_t)((plen >> 8) & 0xFF);
  if (payload && plen) memcpy(s_frame + 12, payload, plen);

  s_tx->notify(s_frame, 12 + plen);
  // Give the controller a moment to push the PDU on a single-core part.
  delay(3);
}

// Split text into DATA frames. Records already carry their own '\n'.
static void glSendText(const String& text) {
  const char* p = text.c_str();
  size_t rem = text.length();
  if (rem == 0) return;
  while (rem) {
    size_t n = rem > GL_PAYLOAD_MAX ? GL_PAYLOAD_MAX : rem;
    glSendFrame(GL_TYPE_DATA, s_reply_id, 0, reinterpret_cast<const uint8_t*>(p), n);
    p += n;
    rem -= n;
  }
}

// ---------------------------------------------------------------------------
// Biscuit link TX (main-loop context only). No framing: one line per notify,
// newline-terminated (the app splits inbound notifications on '\n').
// ---------------------------------------------------------------------------
static int glBatteryLevel() {
#ifdef HAS_BATTERY
  return battery_obj.getBatteryLevel();
#else
  return -1;
#endif
}

static void glBiscSend(const char* line) {
  if (!s_bisc_rsp || !s_connected || !s_bisc_subscribed || !line) return;
  std::string out(line);
  out += '\n';
  s_bisc_rsp->notify(reinterpret_cast<const uint8_t*>(out.data()), out.size());
  delay(3);
}

// Refresh the polled Status characteristic (0 booting, 1 ready, 2 busy).
static void glBiscStatus(int status) {
  if (!s_bisc_sta) return;
  char js[64];
  biscuitFormatStatus(js, sizeof js, status, glBatteryLevel());
  s_bisc_sta->setValue(std::string(js));
}

// ---------------------------------------------------------------------------
// Wardrive record helpers
// ---------------------------------------------------------------------------
static String glHexEncode(const String& s) {
  static const char* hexd = "0123456789abcdef";
  String out;
  out.reserve(s.length() * 2);
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t b = (uint8_t)s[i];
    out += hexd[b >> 4];
    out += hexd[b & 0x0F];
  }
  return out;
}

static const char* glAuthName(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2_WPA3";
    default:                        return "UNKNOWN";
  }
}

// AP dedup. Returns true when 'bssid' was sent within GL_DEDUP_MS (caller
// skips it); otherwise records 'now' against the key -- reusing the oldest slot
// when the table is full -- and returns false so the record is sent.
static bool glDedupHit(const char* bssid, uint32_t now) {
  int freeSlot = -1, oldest = 0;
  for (int i = 0; i < GL_DEDUP_N; i++) {
    if (s_dedup[i].key[0] == '\0') { if (freeSlot < 0) freeSlot = i; continue; }
    if (strncmp(s_dedup[i].key, bssid, sizeof(s_dedup[i].key) - 1) == 0) {
      if (now - s_dedup[i].ms < GL_DEDUP_MS) return true;   // seen recently
      s_dedup[i].ms = now;                                   // stale: refresh
      return false;
    }
    if (s_dedup[i].ms < s_dedup[oldest].ms) oldest = i;
  }
  int slot = (freeSlot >= 0) ? freeSlot : oldest;
  strncpy(s_dedup[slot].key, bssid, sizeof(s_dedup[slot].key) - 1);
  s_dedup[slot].key[sizeof(s_dedup[slot].key) - 1] = '\0';
  s_dedup[slot].ms = now;
  return false;
}

static void glSendHandshake() {
  String hs;
  hs += GL_CAPS_LINE;                                  hs += "\n";
  hs += "WD:MODE mode=wifi\n";
  hs += "WD:BEGIN type=wifi interval=0 channel=auto\n";
  hs += "WD:PLAN chans=" GL_PLAN_CHANS "\n";
  glSendText(hs);
  s_handshake_sent = true;
}

// ---------------------------------------------------------------------------
// GhostLinkBridge
// ---------------------------------------------------------------------------
GhostLinkBridge ghostlink_obj;

// Build (or live-swap) the advertisement for the current slot. A 31-byte PDU
// holds Flags(3) + one 128-bit service UUID(18); the name goes in the scan
// response. Slot 0 advertises the WardriveESP UUID + "Marauder-XXXX", slot 1
// the Biscuit UUID + "Biscuit".
static void glBuildAdv(uint8_t slot) {
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  if (!adv) return;
  const bool biscuit = (slot == 1);

  NimBLEAdvertisementData ad;
  ad.setFlags(0x06);                 // LE General Discoverable + BR/EDR not supported
  ad.addServiceUUID(biscuit ? BISCUIT_SVC_UUID : GL_SVC_UUID);

  NimBLEAdvertisementData sr;
  sr.setName(biscuit ? std::string(BISCUIT_ADV_NAME) : std::string(s_adv_name));

  adv->setAdvertisementData(ad);
  adv->setScanResponseData(sr);
}

static void glSetupStack() {
  if (!s_rx_mux) s_rx_mux = xSemaphoreCreateMutex();

  if (NimBLEDevice::isInitialized())
    NimBLEDevice::deinit(true);

  NimBLEDevice::init("");                 // name set after we know the address
  #ifdef HAS_NIMBLE_2
    NimBLEDevice::setPower(9);            // modest TX power; coexist with Wi-Fi
  #endif
  NimBLEDevice::setMTU(GL_PREFERRED_MTU);

  // Build "Marauder-XXXX" from the last two bytes of the BLE address. Apps find
  // the board by its service UUID, so an honest Marauder name still links fine.
  // (The WardriveESP slot's scan-response name; the Biscuit slot uses "Biscuit".)
  uint64_t addr = (uint64_t)NimBLEDevice::getAddress();
  snprintf(s_adv_name, sizeof(s_adv_name), "Marauder-%02X%02X",
           (unsigned)((addr >> 8) & 0xFF), (unsigned)(addr & 0xFF));
  NimBLEDevice::setDeviceName(s_adv_name);

  s_server = NimBLEDevice::createServer();
  s_server->setCallbacks(&s_server_cb);

  // --- WardriveESP service (WarDriveGo) ------------------------------------
  NimBLEService* svc = s_server->createService(GL_SVC_UUID);

  NimBLECharacteristic* rx = svc->createCharacteristic(
      GL_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(&s_rx_cb);

  NimBLECharacteristic* ctrl = svc->createCharacteristic(
      GL_CTRL_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  ctrl->setCallbacks(&s_rx_cb);          // same handler as RX

  s_tx = svc->createCharacteristic(
      GL_TX_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
  s_tx->setCallbacks(&s_tx_cb);

  svc->start();

  // --- Biscuit service (Biscuit Manager) -----------------------------------
  // A second service on the same server. The CMD write is routed to CH_BISCUIT
  // by pointer in GlRxCallbacks; RSP carries the subscription that triggers the
  // greeting. Status/Settings answer reads with their stored value.
  NimBLEService* bsvc = s_server->createService(BISCUIT_SVC_UUID);
  s_bisc_cmd = bsvc->createCharacteristic(
      BISCUIT_CMD_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  s_bisc_cmd->setCallbacks(&s_rx_cb);
  s_bisc_rsp = bsvc->createCharacteristic(
      BISCUIT_RSP_UUID, NIMBLE_PROPERTY::NOTIFY);
  s_bisc_rsp->setCallbacks(&s_bisc_rsp_cb);
  s_bisc_sta = bsvc->createCharacteristic(
      BISCUIT_STA_UUID, NIMBLE_PROPERTY::READ);
  s_bisc_set = bsvc->createCharacteristic(
      BISCUIT_SET_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
  {
    char js[64];
    biscuitFormatStatus(js, sizeof js, 1, glBatteryLevel());
    s_bisc_sta->setValue(std::string(js));
    s_bisc_set->setValue(std::string("{}"));
  }
  bsvc->start();

  // --- Device Information Service (0x180A) ---------------------------------
  // Biscuit Manager reads model/firmware from here; without it the app logs the
  // firmware row EMPTY and keeps retrying the connection.
  NimBLEService* dis = s_server->createService(NimBLEUUID((uint16_t)0x180A));
  if (dis) {
    NimBLECharacteristic* mfr = dis->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ);
    NimBLECharacteristic* mod = dis->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A24), NIMBLE_PROPERTY::READ);
    NimBLECharacteristic* fw = dis->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A26), NIMBLE_PROPERTY::READ);
    if (mfr && mod && fw) {
      mfr->setValue(std::string("Biscuit Shop"));   // 2A29 Manufacturer
      mod->setValue(std::string("Biscuit DIY"));    // 2A24 Model
      fw->setValue(std::string(MARAUDER_VERSION));  // 2A26 Firmware Revision
      dis->start();
    }
  }

  // --- Advertising ----------------------------------------------------------
  // Two services, two 128-bit UUIDs: a 31-byte PDU holds only one, so the slot
  // rotates while unconnected (see main()). Start on the WardriveESP slot.
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setMinInterval(0x30);             // ~30 ms: quick discovery even with
  adv->setMaxInterval(0x60);             // ~60 ms  Wi-Fi sharing the radio
  #ifdef HAS_NIMBLE_2
    adv->setConnectableMode(BLE_GAP_CONN_MODE_UND);
    adv->setDiscoverableMode(BLE_GAP_DISC_MODE_GEN);
  #endif
  s_adv_slot       = 0;
  s_last_rotate_ms = millis();
  glBuildAdv(s_adv_slot);
  adv->start();

  s_connected      = false;
  s_tx_subscribed  = false;
  s_handshake_sent = false;
  s_bisc_subscribed = false;
  s_bisc_session   = false;

  Serial.print("GhostLink BLE bridge advertising as ");
  Serial.print(s_adv_name);
  Serial.println(" (+ Biscuit)");
}

void GhostLinkBridge::RunSetup() {
  glSetupStack();
}

bool GhostLinkBridge::isConnected() { return s_connected; }
bool GhostLinkBridge::isStreaming() { return s_streaming || s_bisc_session; }

void GhostLinkBridge::startWardrive() {
  // WD:CAPS/MODE/BEGIN/PLAN are the handshake; make sure it has gone out.
  if (!s_handshake_sent) glSendHandshake();
  s_streaming     = true;
  s_scan_pending  = false;
  s_last_sweep_ms = 0;              // scan immediately
  s_scans         = 0;
  s_aps_total     = 0;
  memset(s_dedup, 0, sizeof(s_dedup));   // fresh session: re-report everything
  Serial.println("GhostLink: wardrive stream started");
}

void GhostLinkBridge::stopWardrive(const char* reason) {
  if (s_scan_pending && !s_bisc_session) {
    WiFi.scanDelete();
    s_scan_pending = false;
  }
  s_streaming = false;
  String end = "WD:END reason=";
  end += reason;
  end += "\n";
  glSendText(end);
  glSendFrame(GL_TYPE_END, s_reply_id, 0, nullptr, 0);
  Serial.printf("GhostLink: wardrive stream ended (%s)\n", reason);
}

// Handle one inbound Biscuit line (loop context).
static void glHandleBiscuitLine(const char* line, size_t len) {
  BiscuitCmd c = biscuitParseLine(line, len);
  switch (c.type) {
    case BISCUIT_CMD_TIME_SYNC:
      s_bisc_time_synced = true;
      break;

    case BISCUIT_CMD_STOPSCAN:
      glBiscSend("RSP:stopscan:OK");
      s_bisc_session = false;
      s_bisc_band    = 0;
      // Release a pending sweep only if WarDriveGo isn't also streaming.
      if (!s_streaming && s_scan_pending) {
        WiFi.scanDelete();
        s_scan_pending = false;
      }
      glBiscSend("STATUS:1:Ready");
      glBiscStatus(1);
      break;

    case BISCUIT_CMD_WARDRIVE: {
      s_bisc_session  = true;
      s_bisc_band     = c.band;
      memset(s_dedup, 0, sizeof(s_dedup));   // fresh session: re-report all
      s_last_sweep_ms = 0;                   // sweep right away
      String rsp = String("RSP:") + c.name + ":OK";
      glBiscSend(rsp.c_str());
      String st = String("STATUS:2:Scan started: ") + c.name;
      glBiscSend(st.c_str());
      glBiscStatus(2);
      break;
    }

    case BISCUIT_CMD_UNSUPPORTED: {
      String rsp = String("RSP:") + c.name + ":ERROR:unsupported";
      glBiscSend(rsp.c_str());
      break;
    }

    case BISCUIT_CMD_NONE:
    default:
      break;
  }
}

void GhostLinkBridge::emitApBatch() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  s_scan_pending = false;

  if (n < 0) {                     // failed; try again next sweep
    s_last_sweep_ms = millis();
    return;
  }

  uint32_t ts = millis();
  for (int i = 0; i < n; i++) {
    String bssid = WiFi.BSSIDstr(i);
    if (glDedupHit(bssid.c_str(), ts)) continue;   // sent recently; skip (both links)

    String ssid           = WiFi.SSID(i);
    bool   hidden         = (ssid.length() == 0);
    int    ch             = WiFi.channel(i);
    int    rssi           = WiFi.RSSI(i);
    wifi_auth_mode_t enc  = WiFi.encryptionType(i);

    // WardriveESP link (WarDriveGo): framed WD:AP record.
    if (s_streaming) {
      String rec = "WD:AP ts=";
      rec += ts;
      rec += " bssid=";
      rec += bssid;
      rec += " ssid_hex=";
      rec += hidden ? String("") : glHexEncode(ssid);
      rec += " rssi=";
      rec += rssi;
      rec += " ch=";
      rec += ch;
      rec += " auth=";
      rec += glAuthName(enc);
      rec += " hidden=";
      rec += hidden ? "1" : "0";
      rec += "\n";
      glSendText(rec);
    }

    // Biscuit link: unframed DATA:AP line, with the session's band filter.
    if (s_bisc_session && biscuitBandAllows(s_bisc_band, ch)) {
      String mac = bssid;
      mac.toUpperCase();
      String sec = wifi_scan_obj.security_int_to_string((int)enc);
      char line[220];
      biscuitFormatAp(line, sizeof line, ssid.c_str(), mac.c_str(), ch, rssi, sec.c_str());
      glBiscSend(line);
    }

    s_aps_total++;
  }
  WiFi.scanDelete();
  s_scans++;

  if (s_streaming) {
    String st = "WD:STATUS aps=";
    st += s_aps_total;
    st += " scans=";
    st += s_scans;
    st += "\n";
    glSendText(st);
  }
  if (s_bisc_session) glBiscStatus(2);   // still scanning = busy

  s_last_sweep_ms = millis();
}

void GhostLinkBridge::serviceWardrive(uint32_t now) {
  if (s_scan_pending) {
    emitApBatch();
    return;
  }
  if (now - s_last_sweep_ms < GL_SWEEP_GAP_MS) return;

  // Async scan: non-blocking so touch and the main loop keep running.
  // args: async, show_hidden, passive, max_ms_per_chan, channel(0=all)
  int rc = WiFi.scanNetworks(true, true);
  if (rc == WIFI_SCAN_RUNNING || rc == WIFI_SCAN_FAILED || rc >= 0)
    s_scan_pending = true;         // results collected next tick
}

void GhostLinkBridge::handleCommandLine(const String& line) {
  String cmd = line;
  cmd.trim();
  if (cmd.length() == 0) return;

  String lower = cmd;
  lower.toLowerCase();

  if (lower.startsWith("wdstream start") || lower == "start") {
    startWardrive();            // -ch channel limiting not implemented (scans all)
    return;
  }
  if (lower == "stop" || lower.startsWith("wdstream stop")) {
    if (s_streaming) stopWardrive("stop");
    return;
  }
  if (lower == "caps") {        // spec: `caps` (re)sends WD:CAPS + start lines
    glSendHandshake();
    return;
  }
  if (lower.startsWith("iam ")) return;  // app identifying itself; no reply

  // Bonus: forward anything else to Marauder's normal command handler so the
  // board is still controllable over BLE. (Direct CLI output is not teed back
  // in v1; the GhostLink DATA path carries wardrive records.)
  cli_obj.runCommand(cmd);
}

// Parse one received BLE write: a GhostLink CMD frame or raw text.
static bool glExtractCommand(const std::string& msg, String& out, uint32_t& cmd_id) {
  cmd_id = 0;
  if (msg.size() >= 12 && msg[0] == 'G' && msg[1] == 'B') {
    const uint8_t* b = reinterpret_cast<const uint8_t*>(msg.data());
    cmd_id = (uint32_t)b[6] | ((uint32_t)b[7] << 8) |
             ((uint32_t)b[8] << 16) | ((uint32_t)b[9] << 24);
    uint32_t plen = (uint32_t)b[10] | ((uint32_t)b[11] << 8);
    size_t avail = msg.size() - 12;
    if (plen > avail) plen = avail;
    out = String();
    out.reserve(plen);
    for (uint32_t i = 0; i < plen; i++) out += (char)b[12 + i];
    return true;                   // framed command (send ACK)
  }
  // Raw unframed text.
  out = String(msg.c_str());
  return false;
}

void GhostLinkBridge::main(uint32_t currentTime) {
  // Recover if a Marauder BLE feature (blespam/findmy/sniffbt...) tore the
  // stack down with NimBLEDevice::deinit(). Rate-limited so we never hammer it.
  if (!NimBLEDevice::isInitialized()) {
    if (currentTime - s_recover_ms >= GL_RECOVER_MS) {
      s_recover_ms   = currentTime;
      s_streaming    = false;
      s_bisc_session = false;
      glSetupStack();
    }
    return;
  }

  // Whenever no central is connected, keep advertising and rotate the advertised
  // service UUID+name (~1.28 s) so both WarDriveGo and Biscuit see the slot they
  // match on. NimBLE stops advertising on connect and after some failed
  // connection attempts, and Wi-Fi activity can disrupt it, so we also re-arm it
  // here every loop -- otherwise a board left on can quietly become
  // undiscoverable until reboot.
  if (!s_connected) {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (adv) {
      if (currentTime - s_last_rotate_ms >= GL_ADV_ROTATE_MS) {
        s_last_rotate_ms = currentTime;
        s_adv_slot ^= 1;
        glBuildAdv(s_adv_slot);      // live payload swap
      }
      if (!adv->isAdvertising()) adv->start();
    }
    s_advertise_want = false;
    return;
  }

  // Connected: make the GAP device name (0x2A00) match the active link so a
  // dual-protocol central classifies this board correctly after connecting.
  if (s_gap_applied != s_gap_want) {
    s_gap_applied = s_gap_want;
    NimBLEDevice::setDeviceName(s_gap_want == 1 ? std::string(BISCUIT_ADV_NAME)
                                                : std::string(s_adv_name));
  }

  // WarDriveGo: greet once the central subscribes to TX, and begin scanning: the
  // spec has the board start a wardrive on TX-subscribe (no explicit command).
  if (s_tx_subscribed && !s_handshake_sent) {
    glSendHandshake();
    startWardrive();
  }

  // Biscuit: greet on Response subscribe (TIME:REQUEST + STATUS:1:Ready), then
  // re-request the time every 5 s until the app answers with TIME:SYNC.
  if (s_bisc_subscribed) {
    if (s_bisc_greet) {
      s_bisc_greet       = false;
      s_bisc_time_synced = false;
      glBiscSend("TIME:REQUEST:");
      glBiscSend("STATUS:1:Ready");
      s_bisc_time_req_ms = currentTime;
    } else if (!s_bisc_time_synced && currentTime - s_bisc_time_req_ms >= 5000) {
      s_bisc_time_req_ms = currentTime;
      glBiscSend("TIME:REQUEST:");
    }
  }

  // Drain queued RX writes.
  std::vector<GlRxMsg> msgs;
  if (s_rx_mux && xSemaphoreTake(s_rx_mux, pdMS_TO_TICKS(5)) == pdTRUE) {
    if (!s_rx_msgs.empty()) msgs.swap(s_rx_msgs);
    xSemaphoreGive(s_rx_mux);
  }

  for (auto& m : msgs) {
    if (m.ch == CH_BISCUIT) {
      // One write may carry several '\n'-separated Biscuit lines.
      const std::string& s = m.data;
      size_t start = 0;
      while (start < s.size()) {
        size_t nl  = s.find('\n', start);
        size_t end = (nl == std::string::npos) ? s.size() : nl;
        if (end > start) glHandleBiscuitLine(s.data() + start, end - start);
        start = (nl == std::string::npos) ? s.size() : nl + 1;
      }
      continue;
    }

    String cmd;
    uint32_t cmd_id = 0;
    bool framed = glExtractCommand(m.data, cmd, cmd_id);
    if (framed) {
      s_reply_id = cmd_id;       // tag every later frame with this command's id
      glSendFrame(GL_TYPE_ACK, cmd_id, 0, nullptr, 0);
    }
    handleCommandLine(cmd);
  }

  if (s_streaming || s_bisc_session) serviceWardrive(currentTime);
}

#endif // HAS_BLE_UART_BRIDGE
