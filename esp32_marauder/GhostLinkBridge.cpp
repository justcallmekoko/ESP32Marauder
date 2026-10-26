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
//     [6..7]   command id      (uint16 LE)
//     [8..11]  payload length  (uint32 LE)
//   ...followed by <payload length> payload bytes.
//
// The companion app concatenates DATA-frame payloads and then splits the
// accumulated bytes on '\n', so a single record may span several BLE frames.
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

// ---------------------------------------------------------------------------
// Shared state (file scope so the NimBLE callback objects and the bridge
// methods can see it). All NimBLE notify() calls happen from the main loop;
// callbacks only set flags / enqueue, which keeps the single radio core sane.
// ---------------------------------------------------------------------------
namespace {
  NimBLEServer*         s_server   = nullptr;
  NimBLECharacteristic* s_tx       = nullptr;

  volatile bool s_connected        = false;
  volatile bool s_tx_subscribed    = false;
  volatile bool s_handshake_sent   = false;
  volatile bool s_advertise_want   = false;

  SemaphoreHandle_t s_rx_mux = nullptr;          // guards s_rx_msgs
  std::vector<std::string> s_rx_msgs;            // RX writes pending for loop

  // Wardrive sweep state.
  // Every outgoing frame carries the "reply id" = the id of the last command
  // the phone sent (its CMD frame id), starting at 1 before any command. The
  // app waits for an ACK with that id and for DATA replies tagged with it.
  uint32_t s_reply_id      = 1;

  bool     s_streaming     = false;
  bool     s_scan_pending  = false;
  uint32_t s_last_sweep_ms = 0;
  uint32_t s_scans         = 0;
  uint32_t s_aps_total     = 0;
  uint32_t s_recover_ms    = 0;

  uint8_t  s_frame[12 + GL_PAYLOAD_MAX];

  void rxEnqueue(const uint8_t* data, size_t len) {
    if (!data || !len || !s_rx_mux) return;
    std::string msg(reinterpret_cast<const char*>(data), len);
    // Callback runs in the NimBLE host task (not an ISR), so a blocking mutex
    // is safe here and avoids heap work inside a spinlock critical section.
    if (xSemaphoreTake(s_rx_mux, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (s_rx_msgs.size() < 16)         // bound the queue
        s_rx_msgs.push_back(std::move(msg));
      xSemaphoreGive(s_rx_mux);
    }
  }
}

// ---------------------------------------------------------------------------
// NimBLE callbacks
// ---------------------------------------------------------------------------
class GlServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* /*p*/, NimBLEConnInfo& /*info*/) override {
    s_connected      = true;
    s_handshake_sent = false;      // re-greet each new central
    s_reply_id       = 1;          // spec: reply id starts at 1 before any command
  }
  void onDisconnect(NimBLEServer* /*p*/, NimBLEConnInfo& /*info*/, int /*reason*/) override {
    s_connected     = false;
    s_tx_subscribed = false;
    s_streaming     = false;
    s_advertise_want = true;       // main loop re-arms advertising
  }
};

class GlRxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& /*info*/) override {
    NimBLEAttValue v = c->getValue();
    rxEnqueue(v.data(), v.length());
  }
};

class GlTxCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* /*c*/, NimBLEConnInfo& /*info*/, uint16_t subValue) override {
    s_tx_subscribed = (subValue != 0);
    if (s_tx_subscribed)
      s_handshake_sent = false;    // greet on (re)subscribe
  }
};

static GlServerCallbacks s_server_cb;
static GlRxCallbacks     s_rx_cb;
static GlTxCallbacks     s_tx_cb;

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
  uint64_t addr = (uint64_t)NimBLEDevice::getAddress();
  char name[20];
  snprintf(name, sizeof(name), "Marauder-%02X%02X",
           (unsigned)((addr >> 8) & 0xFF), (unsigned)(addr & 0xFF));
  NimBLEDevice::setDeviceName(name);

  s_server = NimBLEDevice::createServer();
  s_server->setCallbacks(&s_server_cb);

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

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(GL_SVC_UUID);
  adv->setName(name);
  adv->enableScanResponse(true);
  adv->setMinInterval(0x30);             // ~30 ms: quick discovery even with
  adv->setMaxInterval(0x60);             // ~60 ms  Wi-Fi sharing the radio
  adv->start();

  s_connected      = false;
  s_tx_subscribed  = false;
  s_handshake_sent = false;

  Serial.print("GhostLink BLE bridge advertising as ");
  Serial.println(name);
}

void GhostLinkBridge::RunSetup() {
  glSetupStack();
}

bool GhostLinkBridge::isConnected() { return s_connected; }
bool GhostLinkBridge::isStreaming() { return s_streaming; }

void GhostLinkBridge::startWardrive() {
  // WD:CAPS/MODE/BEGIN/PLAN are the handshake; make sure it has gone out.
  if (!s_handshake_sent) glSendHandshake();
  s_streaming     = true;
  s_scan_pending  = false;
  s_last_sweep_ms = 0;              // scan immediately
  s_scans         = 0;
  s_aps_total     = 0;
  Serial.println("GhostLink: wardrive stream started");
}

void GhostLinkBridge::stopWardrive(const char* reason) {
  if (s_scan_pending) {
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
    String ssid   = WiFi.SSID(i);
    bool   hidden = (ssid.length() == 0);
    String rec    = "WD:AP ts=";
    rec += ts;
    rec += " bssid=";
    rec += WiFi.BSSIDstr(i);
    rec += " ssid_hex=";
    rec += hidden ? String("") : glHexEncode(ssid);
    rec += " rssi=";
    rec += WiFi.RSSI(i);
    rec += " ch=";
    rec += WiFi.channel(i);
    rec += " auth=";
    rec += glAuthName(WiFi.encryptionType(i));
    rec += " hidden=";
    rec += hidden ? "1" : "0";
    rec += "\n";
    glSendText(rec);
    s_aps_total++;
  }
  WiFi.scanDelete();
  s_scans++;

  String st = "WD:STATUS aps=";
  st += s_aps_total;
  st += " scans=";
  st += s_scans;
  st += "\n";
  glSendText(st);

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
      s_recover_ms = currentTime;
      s_streaming  = false;
      glSetupStack();
    }
    return;
  }

  // Whenever no central is connected, make sure we are advertising. NimBLE
  // stops advertising on connect and after some failed connection attempts, and
  // Wi-Fi activity can disrupt it, so re-arm it here every loop instead of only
  // on a disconnect event -- otherwise a board left on can quietly become
  // undiscoverable until reboot (symptom: app struggles to connect to a board
  // that was already powered on).
  if (!s_connected) {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (adv && !adv->isAdvertising()) adv->start();
    s_advertise_want = false;
    return;
  }

  // Greet once the central subscribes to TX, and begin scanning: the spec has
  // the board start a wardrive on TX-subscribe, which is what WarDriveGo waits
  // for (no explicit command needed).
  if (s_tx_subscribed && !s_handshake_sent) {
    glSendHandshake();
    startWardrive();
  }

  // Drain queued RX writes.
  std::vector<std::string> msgs;
  if (s_rx_mux && xSemaphoreTake(s_rx_mux, pdMS_TO_TICKS(5)) == pdTRUE) {
    if (!s_rx_msgs.empty()) msgs.swap(s_rx_msgs);
    xSemaphoreGive(s_rx_mux);
  }

  for (auto& m : msgs) {
    String cmd;
    uint32_t cmd_id = 0;
    bool framed = glExtractCommand(m, cmd, cmd_id);
    if (framed) {
      s_reply_id = cmd_id;       // tag every later frame with this command's id
      glSendFrame(GL_TYPE_ACK, cmd_id, 0, nullptr, 0);
    }
    handleCommandLine(cmd);
  }

  if (s_streaming) serviceWardrive(currentTime);
}

#endif // HAS_BLE_UART_BRIDGE
