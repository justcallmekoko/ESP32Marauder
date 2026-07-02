#include "JsonSerial.h"

#include "WiFiScan.h"
#include "EvilPortal.h"
#include "utils.h"

// Globals defined in WiFiScan.cpp / esp32_marauder.ino (mirrors CommandLine.h)
extern LinkedList<AccessPoint>* access_points;
extern LinkedList<Station>* stations;
extern LinkedList<ssid>* ssids;
extern LinkedList<IPAddress>* ipList;
extern LinkedList<ProbeReqSsid>* probe_req_ssids;
extern LinkedList<AirTag>* airtags;
extern WiFiScan wifi_scan_obj;
extern const String version_number;

// When false, the continuous analyzer / channel-activity streaming is
// suppressed so a plain serial monitor (no JSON host) is never flooded with
// "@J" lines. Enabled by the `jsoninfo` handshake or an explicit `jsonmode 1`.
static bool g_jsonMode = false;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Escape a string for safe inclusion inside a JSON string literal.
//
// 802.11 SSIDs are arbitrary byte strings, so this is UTF-8 aware: valid UTF-8
// sequences are passed through verbatim (JSON text is UTF-8); ", \, and control
// characters are escaped; and any byte that is not part of a valid UTF-8
// sequence is replaced with U+FFFD. The result is therefore always valid JSON,
// even for non-UTF-8 / random-byte SSIDs.
static String jsonEscape(const String& in) {
  String out;
  unsigned int n = in.length();
  out.reserve(n + 8);
  for (unsigned int i = 0; i < n; ) {
    uint8_t c = (uint8_t)in.charAt(i);

    if (c < 0x80) { // ASCII
      switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
          if (c < 0x20 || c == 0x7F) {
            char buf[7];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
          } else {
            out += (char)c;
          }
      }
      i++;
      continue;
    }

    // Multi-byte UTF-8: validate length, continuation bytes, AND the decoded
    // code point (reject overlong encodings, UTF-16 surrogates U+D800..U+DFFF,
    // and values > U+10FFFF). This guarantees the emitted string is always
    // well-formed UTF-8, i.e. valid JSON, for arbitrary 802.11 SSID bytes.
    int seqLen = 0;
    if ((c & 0xE0) == 0xC0) seqLen = 2;
    else if ((c & 0xF0) == 0xE0) seqLen = 3;
    else if ((c & 0xF8) == 0xF0) seqLen = 4;

    bool valid = (seqLen >= 2) && (i + (unsigned int)seqLen <= n);
    uint32_t cp = 0;
    if (valid) {
      cp = (uint32_t)(c & (0xFF >> (seqLen + 1)));
      for (int k = 1; k < seqLen; k++) {
        uint8_t cc = (uint8_t)in.charAt(i + k);
        if ((cc & 0xC0) != 0x80) { valid = false; break; }
        cp = (cp << 6) | (uint32_t)(cc & 0x3F);
      }
    }
    if (valid) {
      static const uint32_t minCp[5] = {0, 0, 0x80, 0x800, 0x10000};
      if (cp < minCp[seqLen] || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
        valid = false;
    }

    if (valid) {
      for (int k = 0; k < seqLen; k++) out += in.charAt(i + k);
      i += (unsigned int)seqLen;
    } else {
      out += "\\ufffd"; // invalid byte -> U+FFFD replacement character
      i++;
    }
  }
  return out;
}

// Emit  "key":true,  (or false). Pass comma=false to omit the trailing comma.
static inline void boolField(const __FlashStringHelper* key, bool v, bool comma = true) {
  Serial.print(key);
  Serial.print(v ? F("true") : F("false"));
  if (comma) Serial.print(F(","));
}

// ---------------------------------------------------------------------------
// jsoninfo - handshake / capability advertisement
// ---------------------------------------------------------------------------
static void printInfo() {
  Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"info\",\"fw\":\""));
  Serial.print(version_number);
  Serial.print(F("\",\"proto\":"));
  Serial.print(MARAUDER_JSON_PROTO);
  Serial.print(F(",\"board\":\""));
  Serial.print(F(HARDWARE_NAME));
  Serial.print(F("\",\"caps\":["));

  // WiFi is present on every supported board, so it is always emitted first;
  // every other capability is conditionally appended with a leading comma.
  Serial.print(F("\"wifi\""));
  #ifdef HAS_BT
    Serial.print(F(",\"bt\""));
  #endif
  #ifdef HAS_GPS
    Serial.print(F(",\"gps\""));
  #endif
  #ifdef HAS_SD
    Serial.print(F(",\"sd\""));
  #endif
  #ifdef HAS_SCREEN
    Serial.print(F(",\"screen\""));
  #endif

  Serial.println(F("]}"));
}

// ---------------------------------------------------------------------------
// jsonstatus - lightweight, poll friendly device state
// ---------------------------------------------------------------------------
static void printStatus() {
  Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"status\",\"mode\":"));
  Serial.print(wifi_scan_obj.currentScanMode);
  Serial.print(F(",")); boolField(F("\"running\":"), wifi_scan_obj.scanning());
  Serial.print(F("\"free\":"));
  Serial.print(ESP.getFreeHeap());
  Serial.print(F(",\"aps\":"));
  Serial.print(access_points ? access_points->size() : 0);
  Serial.print(F(",\"stas\":"));
  Serial.print(stations ? stations->size() : 0);
  Serial.print(F(",\"ssids\":"));
  Serial.print(ssids ? ssids->size() : 0);
  Serial.print(F(",\"ips\":"));
  Serial.print(ipList ? ipList->size() : 0);
  Serial.print(F(",\"probes\":"));
  Serial.print(probe_req_ssids ? probe_req_ssids->size() : 0);
  Serial.print(F(",\"airtags\":"));
  Serial.print(airtags ? airtags->size() : 0);
  Serial.println(F("}"));
}

// ---------------------------------------------------------------------------
// jsonlist emitters. Each returns the number of objects it actually emitted so
// the closing {"t":"end"} "n" always matches the streamed rows.
// ---------------------------------------------------------------------------
static int listAps() {
  int n = access_points ? access_points->size() : 0;
  for (int i = 0; i < n; i++) {
    AccessPoint ap = access_points->get(i);
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"ap\",\"i\":"));
    Serial.print(i);
    Serial.print(F(",\"ch\":"));   Serial.print(ap.channel);
    Serial.print(F(",\"rssi\":")); Serial.print(ap.rssi);
    Serial.print(F(",")); boolField(F("\"sel\":"), ap.selected);
    Serial.print(F("\"pkts\":"));  Serial.print(ap.packets);
    Serial.print(F(",\"sec\":"));  Serial.print(ap.sec);
    Serial.print(F(",")); boolField(F("\"wps\":"), ap.wps);
    Serial.print(F("\"nsta\":"));  Serial.print(ap.stations ? ap.stations->size() : 0);
    Serial.print(F(",\"bssid\":\"")); Serial.print(macToString(ap.bssid));
    Serial.print(F("\",\"essid\":\"")); Serial.print(jsonEscape(ap.essid));
    Serial.println(F("\"}"));
  }
  return n;
}

static int listStations() {
  if (!access_points || !stations) return 0;
  int emitted = 0;
  for (int x = 0; x < access_points->size(); x++) {
    AccessPoint ap = access_points->get(x);
    if (!ap.stations) continue;
    for (int j = 0; j < ap.stations->size(); j++) {
      uint16_t idx = ap.stations->get(j);
      Station st = stations->get(idx);
      Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"sta\",\"ap\":"));
      Serial.print(x);
      Serial.print(F(",\"i\":"));   Serial.print(idx);
      Serial.print(F(",")); boolField(F("\"sel\":"), st.selected);
      Serial.print(F("\"pkts\":")); Serial.print(st.packets);
      Serial.print(F(",\"mac\":\"")); Serial.print(macToString(st.mac));
      Serial.println(F("\"}"));
      emitted++;
    }
  }
  return emitted;
}

static int listSsids() {
  int n = ssids ? ssids->size() : 0;
  for (int i = 0; i < n; i++) {
    ssid s = ssids->get(i);
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"ssid\",\"i\":"));
    Serial.print(i);
    Serial.print(F(",\"ch\":")); Serial.print(s.channel);
    Serial.print(F(",")); boolField(F("\"sel\":"), s.selected);
    Serial.print(F("\"essid\":\"")); Serial.print(jsonEscape(s.essid));
    Serial.println(F("\"}"));
  }
  return n;
}

static int listIps() {
  int n = ipList ? ipList->size() : 0;
  for (int i = 0; i < n; i++) {
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"ip\",\"i\":"));
    Serial.print(i);
    Serial.print(F(",\"ip\":\""));
    Serial.print(ipList->get(i).toString());
    Serial.println(F("\"}"));
  }
  return n;
}

static int listProbes() {
  int n = probe_req_ssids ? probe_req_ssids->size() : 0;
  for (int i = 0; i < n; i++) {
    ProbeReqSsid p = probe_req_ssids->get(i);
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"probe\",\"i\":"));
    Serial.print(i);
    Serial.print(F(",\"req\":")); Serial.print(p.requests);
    Serial.print(F(",")); boolField(F("\"sel\":"), p.selected);
    Serial.print(F("\"essid\":\"")); Serial.print(jsonEscape(p.essid));
    Serial.println(F("\"}"));
  }
  return n;
}

static int listAirtags() {
  int n = airtags ? airtags->size() : 0;
  for (int i = 0; i < n; i++) {
    AirTag a = airtags->get(i);
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"airtag\",\"i\":"));
    Serial.print(i);
    Serial.print(F(",\"rssi\":")); Serial.print(a.rssi);
    Serial.print(F(",")); boolField(F("\"sel\":"), a.selected);
    Serial.print(F("\"mac\":\"")); Serial.print(jsonEscape(a.mac));
    Serial.println(F("\"}"));
  }
  return n;
}

static void printList(char which) {
  int count;
  switch (which) {
    case 'a': count = listAps();      break;
    case 's': count = listSsids();    break;
    case 'c': count = listStations(); break;
    case 'i': count = listIps();      break;
    case 'p': count = listProbes();   break;
    case 't': count = listAirtags();  break;
    default:
      // Unknown list type: report an explicit error instead of silently
      // returning the AP list under the wrong label.
      Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"err\",\"cmd\":\"jsonlist\",\"arg\":\""));
      Serial.print(jsonEscape(String(which))); // escape so e.g. `jsonlist \` stays valid JSON
      Serial.println(F("\"}"));
      return;
  }
  Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"end\",\"list\":\""));
  Serial.print(which);
  Serial.print(F("\",\"n\":"));
  Serial.print(count);
  Serial.println(F("}"));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
bool JsonSerial::jsonModeEnabled() {
  return g_jsonMode;
}

bool JsonSerial::handle(LinkedList<String>* cmd_args) {
  if (!cmd_args || cmd_args->size() == 0) return false;
  String cmd = cmd_args->get(0);

  if (cmd == JSON_INFO_CMD) {
    g_jsonMode = true; // a host is talking to us -> enable streaming
    printInfo();
    return true;
  }
  if (cmd == JSON_STATUS_CMD) {
    printStatus();
    return true;
  }
  if (cmd == JSON_MODE_CMD) {
    if (cmd_args->size() > 1) {
      String a = cmd_args->get(1);
      g_jsonMode = (a == "1" || a == "on" || a == "true");
    }
    Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"jsonmode\","));
    boolField(F("\"on\":"), g_jsonMode, false);
    Serial.println(F("}"));
    return true;
  }
  if (cmd == JSON_LIST_CMD) {
    char which = 'a';
    if (cmd_args->size() > 1) {
      String a = cmd_args->get(1);
      a.replace("-", "");
      a.trim();
      if (a.length() > 0) which = a.charAt(0);
    }
    printList(which);
    return true;
  }
  return false;
}

void JsonSerial::emitAnalyzerSample(uint8_t mode, int ch, int16_t value) {
  if (!g_jsonMode) return;
  Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"asample\",\"mode\":"));
  Serial.print(mode);
  Serial.print(F(",\"ch\":"));
  Serial.print(ch);
  Serial.print(F(",\"v\":"));
  Serial.print(value);
  Serial.println(F("}"));
}

void JsonSerial::emitChannelActivity(const int* channels, const uint8_t* values, int count, int page) {
  if (!g_jsonMode || !channels || !values || count <= 0) return;
  Serial.print(F(JSON_LINE_PREFIX "{\"t\":\"chan\",\"page\":"));
  Serial.print(page);
  Serial.print(F(",\"ch\":["));
  for (int i = 0; i < count; i++) {
    if (i) Serial.print(',');
    Serial.print(channels[i]);
  }
  Serial.print(F("],\"v\":["));
  for (int i = 0; i < count; i++) {
    if (i) Serial.print(',');
    Serial.print(values[i]);
  }
  Serial.println(F("]}"));
}
