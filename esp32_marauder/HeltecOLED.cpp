#include "HeltecOLED.h"

#ifdef HELTEC_WIFI_LORA_32_V4

#include "WiFiScan.h"
#include "CommandLine.h"

// Marauder globals we read for the status view
extern WiFiScan wifi_scan_obj;
extern LinkedList<AccessPoint>* access_points;
extern LinkedList<Station>* stations;
extern LinkedList<ssid>* ssids;

// Live sniff counters (file-scope globals in WiFiScan.cpp)
extern int num_beacon;
extern int num_deauth;
extern int num_probe;

HeltecOLED heltec_oled;

void HeltecOLED::begin() {
  // Power the peripheral rail. On Heltec boards Vext is ACTIVE-LOW.
  pinMode(HELTEC_VEXT, OUTPUT);
  digitalWrite(HELTEC_VEXT, LOW);
  delay(50);

  // Hardware reset pulse for the SSD1306
  pinMode(HELTEC_OLED_RST, OUTPUT);
  digitalWrite(HELTEC_OLED_RST, HIGH); delay(1);
  digitalWrite(HELTEC_OLED_RST, LOW);  delay(10);
  digitalWrite(HELTEC_OLED_RST, HIGH); delay(10);

  // OLED sits on its own I2C bus (17/18), not the default Wire pins (3/4)
  Wire.begin(HELTEC_OLED_SDA, HELTEC_OLED_SCL);

  _ok = _d.begin(SSD1306_SWITCHCAPVCC, HELTEC_OLED_ADDR);
  if (!_ok) {
    Serial.println(F("[OLED] SSD1306 init FAILED (Heltec V4)"));
    return;
  }

  _d.clearDisplay();
  _d.setTextColor(SSD1306_WHITE);
  _d.setTextSize(1);
  _d.setCursor(0, 0);
  _d.println(F("ESP32 Marauder"));
  _d.println(F(HARDWARE_NAME));
  _d.print(F("fw "));
  _d.println(F(MARAUDER_VERSION));
  _d.display();
  Serial.println(F("[OLED] Heltec V4 status display up"));
}

const char* HeltecOLED::modeStr(uint8_t m) {
  switch (m) {
    case WIFI_SCAN_OFF:         return "Idle";
    case WIFI_SCAN_AP:          return "Scan AP";
    case WIFI_SCAN_STATION:     return "Scan STA";
    case WIFI_SCAN_AP_STA:      return "Scan AP+STA";
    case WIFI_SCAN_ALL:         return "Scan ALL";
    case WIFI_SCAN_PROBE:       return "Sniff Probe";
    case WIFI_SCAN_DEAUTH:      return "Sniff Deauth";
    case WIFI_SCAN_EAPOL:       return "Sniff PMKID";
    case WIFI_SCAN_ACTIVE_EAPOL:return "Actv EAPOL";
    case WIFI_SCAN_RAW_CAPTURE: return "Raw Cap";
    case WIFI_SCAN_EVIL_PORTAL: return "EvilPortal";
    case WIFI_SCAN_PACKET_RATE: return "Pkt Rate";
    case WIFI_SCAN_SIG_STREN:   return "Signal";
    case WIFI_SCAN_MULTISSID:   return "MultiSSID";
    default:                    return "Busy";
  }
}

void HeltecOLED::update(uint32_t now) {
  if (!_ok) return;
  if (now - _last < 500) return;   // ~2 Hz refresh
  _last = now;
  _spin = (uint8_t)((_spin + 1) & 3);
  draw();
}

void HeltecOLED::draw() {
  int ap  = access_points ? access_points->size() : 0;
  int sta = stations      ? stations->size()      : 0;
  long rx = (long)num_beacon + (long)num_probe + (long)num_deauth; // sniffed frames
  long tx = wifi_scan_obj.getPacketsSent();                        // frames sent (attacks)
  uint8_t mode = wifi_scan_obj.currentScanMode;
  uint8_t ch   = wifi_scan_obj.set_channel;
  uint32_t up  = millis() / 1000UL;
  uint32_t freeK = ESP.getFreeHeap() / 1024UL;
  static const char spin[4] = {'|','/','-','\\'};

  _d.clearDisplay();

  // Inverted title bar
  _d.fillRect(0, 0, HELTEC_OLED_W, 11, SSD1306_WHITE);
  _d.setTextColor(SSD1306_BLACK);
  _d.setCursor(2, 2);
  _d.print(F("Marauder "));
  _d.print(F(MARAUDER_VERSION));
  _d.setCursor(HELTEC_OLED_W - 8, 2);
  _d.print(spin[_spin]);

  _d.setTextColor(SSD1306_WHITE);

  _d.setCursor(0, 14);
  _d.print(F("Mode: "));
  _d.print(modeStr(mode));

  _d.setCursor(0, 24);
  _d.print(F("AP:"));   _d.print(ap);
  _d.print(F("  STA:")); _d.print(sta);

  _d.setCursor(0, 34);
  _d.print(F("RX:")); _d.print(rx);
  _d.print(F("  TX:")); _d.print(tx);

  _d.setCursor(0, 44);
  _d.print(F("CH:")); _d.print(ch);
  _d.print(F("  Up ")); _d.print(up / 60UL); _d.print('m');
  _d.print(up % 60UL); _d.print('s');

  _d.setCursor(0, 54);
  _d.print(F("Heap ")); _d.print(freeK); _d.print(F("K"));

  _d.display();
}

#endif // HELTEC_WIFI_LORA_32_V4
