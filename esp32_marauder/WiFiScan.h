#pragma once

#ifndef WiFiScan_h
#define WiFiScan_h

#include "configs.h"
#include "utils.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <vector>

#ifdef HAS_BT
  #include <NimBLEDevice.h> // 1.3.8
#endif

#include <WiFi.h>
#include <ESP32Ping.h>
#include "EvilPortal.h"
#include <math.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#ifdef HAS_DUAL_BAND
  #include "esp_system.h"
#endif
#ifdef HAS_BT
  #include "esp_bt.h"
#endif
#ifdef HAS_SCREEN
  #include "Display.h"
#endif
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"
#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif
#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif
#include "settings.h"
#include "Assets.h"
#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
#elif defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
#elif defined(MARAUDER_M5STICKC)
  #include "stickcLED.h"
#else
  #include "LedInterface.h"
#endif

#define bad_list_length 3

#define OTA_UPDATE 100
#define SHOW_INFO 101
#define ESP_UPDATE 102
#define WIFI_SCAN_OFF 0
#define WIFI_SCAN_PROBE 1
#define WIFI_SCAN_AP 2
#define WIFI_SCAN_PWN 3
#define WIFI_SCAN_EAPOL 4
#define WIFI_SCAN_DEAUTH 5
#define WIFI_SCAN_ALL 6
#define WIFI_PACKET_MONITOR 7
#define WIFI_ATTACK_BEACON_SPAM 8
#define WIFI_ATTACK_RICK_ROLL 9
#define BT_SCAN_ALL 10
#define BT_SCAN_SKIMMERS 11
#define WIFI_SCAN_ESPRESSIF 12
#define LV_JOIN_WIFI 13
#define LV_ADD_SSID 14
#define WIFI_ATTACK_BEACON_LIST 15
#define WIFI_SCAN_TARGET_AP 16
#define LV_SELECT_AP 17
#define WIFI_ATTACK_AUTH 18
#define WIFI_ATTACK_MIMIC 19
#define WIFI_ATTACK_DEAUTH 20
#define WIFI_ATTACK_AP_SPAM 21
#define WIFI_SCAN_TARGET_AP_FULL 22
#define WIFI_SCAN_ACTIVE_EAPOL 23
#define WIFI_ATTACK_DEAUTH_MANUAL 24
#define WIFI_SCAN_RAW_CAPTURE 25
#define WIFI_SCAN_STATION 26
#define WIFI_ATTACK_DEAUTH_TARGETED 27
#define WIFI_SCAN_ACTIVE_LIST_EAPOL 28
#define WIFI_SCAN_SIG_STREN 29
#define WIFI_SCAN_EVIL_PORTAL 30
#define WIFI_SCAN_GPS_DATA 31
#define WIFI_SCAN_WAR_DRIVE 32
#define WIFI_SCAN_STATION_WAR_DRIVE 33
#define BT_SCAN_WAR_DRIVE 34
#define BT_SCAN_WAR_DRIVE_CONT 35
#define BT_ATTACK_SOUR_APPLE 36
#define BT_ATTACK_SWIFTPAIR_SPAM 37
#define BT_ATTACK_SPAM_ALL 38
#define BT_ATTACK_SAMSUNG_SPAM 39
#define WIFI_SCAN_GPS_NMEA 40
#define BT_ATTACK_GOOGLE_SPAM 41
#define BT_ATTACK_FLIPPER_SPAM 42
#define BT_SCAN_AIRTAG 43
#define BT_SPOOF_AIRTAG 44
#define BT_SCAN_FLIPPER 45
#define WIFI_SCAN_CHAN_ANALYZER 46
#define BT_SCAN_ANALYZER 47
#define WIFI_SCAN_PACKET_RATE 48
#define WIFI_SCAN_AP_STA 49
#define WIFI_SCAN_PINESCAN 50
#define WIFI_SCAN_MULTISSID 51
#define WIFI_CONNECTED 52
#define WIFI_PING_SCAN 53
#define WIFI_PORT_SCAN_ALL 54

#define BASE_MULTIPLIER 4

#define ANALYZER_NAME_REFRESH 100 // Number of events to refresh the name

// PineScan and Multi SSID
#define MULTISSID_THRESHOLD 3 // Threshold For Multi SSID
#define MAX_MULTISSID_ENTRIES 100 // Max number of confirmed MultiSSIDs to store
#define MAX_AP_ENTRIES 100 // Max number of APs to track for analysis
#define MAX_DISPLAY_ENTRIES 1 // Max Unique MACs to display
#define MAX_PINESCAN_ENTRIES 100 // PineScan Max Entries

#define MAX_CHANNEL     14

#define MAX_PORT 65535

#define WIFI_SECURITY_OPEN   0
#define WIFI_SECURITY_WEP    1
#define WIFI_SECURITY_WPA    2
#define WIFI_SECURITY_WPA2   3
#define WIFI_SECURITY_WPA3   4
#define WIFI_SECURITY_WPA_WPA2_MIXED 5
#define WIFI_SECURITY_WPA2_ENTERPRISE 6
#define WIFI_SECURITY_WPA3_ENTERPRISE 7
#define WIFI_SECURITY_WAPI 8
#define WIFI_SECURITY_UNKNOWN 255

#define WPS_CONFIG_USBA              0x0001
#define WPS_CONFIG_ETHERNET          0x0002
#define WPS_CONFIG_LABEL             0x0004
#define WPS_CONFIG_DISPLAY           0x0008
#define WPS_CONFIG_EXT_NFC_TOKEN     0x0010
#define WPS_CONFIG_INT_NFC_TOKEN     0x0020
#define WPS_CONFIG_NFC_INTERFACE     0x0040
#define WPS_CONFIG_PUSH_BUTTON       0x0080
#define WPS_CONFIG_KEYPAD            0x0100
#define WPS_CONFIG_VIRT_PUSH_BUTTON  0x1000
#define WPS_CONFIG_PHY_PUSH_BUTTON   0x2000
#define WPS_CONFIG_VIRT_DISPLAY      0x4000
#define WPS_CONFIG_PHY_DISPLAY       0x8000

extern EvilPortal evil_portal_obj;

#ifdef HAS_SCREEN
  extern Display display_obj;
#endif
#ifdef HAS_SD
  extern SDInterface sd_obj;
#endif
#ifdef HAS_GPS
  extern GpsInterface gps_obj;
#endif
extern Buffer buffer_obj;
#ifdef HAS_BATTERY
  extern BatteryInterface battery_obj;
#endif
extern Settings settings_obj;
#ifdef HAS_FLIPPER_LED
  extern flipperLED flipper_led;
#elif defined(XIAO_ESP32_S3)
  extern xiaoLED xiao_led;
#elif defined(MARAUDER_M5STICKC)
  extern stickcLED stickc_led;
#else
  extern LedInterface led_obj;
#endif

esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

#ifdef HAS_DUAL_BAND
  esp_err_t esp_base_mac_addr_set(uint8_t *Mac);
#endif

struct AirTag {
    String mac;                  // MAC address of the AirTag
    std::vector<uint8_t> payload; // Payload data
    uint16_t payloadSize;
    bool selected;
};

struct Flipper {
  String mac;
  String name;
};

#ifdef HAS_PSRAM
  extern struct mac_addr* mac_history;
#endif

class WiFiScan
{
  private:
    // Wardriver thanks to https://github.com/JosephHewitt
    #ifndef HAS_PSRAM
      struct mac_addr mac_history[mac_history_len];
    #endif

    uint8_t ap_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    uint8_t sta_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

    uint8_t dual_band_channels[DUAL_BAND_CHANNELS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 149, 153, 157, 161, 165, 169, 173, 177};

    uint8_t dual_band_channel_index = 0;

    // Settings
    uint mac_history_cursor = 0;
    uint8_t channel_hop_delay = 1;
  
    int x_pos; //position along the graph x axis
    float y_pos_x; //current graph y axis position of X value
    float y_pos_x_old = 120; //old y axis position of X value
    float y_pos_y; //current graph y axis position of Y value
    float y_pos_y_old = 120; //old y axis position of Y value
    float y_pos_z; //current graph y axis position of Z value
    float y_pos_z_old = 120; //old y axis position of Z value
    int midway = 0;
    byte x_scale = 1; //scale of graph x axis, controlled by touchscreen buttons
    byte y_scale = 1;

    bool do_break = false;

    bool wsl_bypass_enabled = false;

    bool scan_complete = false;

    //int num_beacon = 0; // GREEN
    //int num_probe = 0; // BLUE
    //int num_deauth = 0; // RED

    uint32_t initTime = 0;
    bool run_setup = true;
    void initWiFi(uint8_t scan_mode);
    uint8_t bluetoothScanTime = 5;
    int packets_sent = 0;
    const wifi_promiscuous_filter_t filt = {.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA};
    #ifdef HAS_BT
      NimBLEScan* pBLEScan;
    #endif

    //String connected_network = "";
    //const String alfa = "1234567890qwertyuiopasdfghjkklzxcvbnm QWERTYUIOPASDFGHJKLZXCVBNM_";

    const char* rick_roll[8] = {
      "01 Never gonna give you up",
      "02 Never gonna let you down",
      "03 Never gonna run around",
      "04 and desert you",
      "05 Never gonna make you cry",
      "06 Never gonna say goodbye",
      "07 Never gonna tell a lie",
      "08 and hurt you"
    };

    char* prefix = "G";

    typedef struct
    {
      int16_t fctl;
      int16_t duration;
      uint8_t da;
      uint8_t sa;
      uint8_t bssid;
      int16_t seqctl;
      unsigned char payload[];
    } __attribute__((packed)) WifiMgmtHdr;
    
    typedef struct {
      uint8_t payload[0];
      WifiMgmtHdr hdr;
    } wifi_ieee80211_packet_t;

		// Tracking structures for PineScan (similar to MultiSSID)
    struct PineScanTracker {
        uint8_t mac[6];
        bool suspicious_oui;
        bool tag_and_susp_cap;
        uint8_t channel;
        int8_t rssi;
        bool reported;
    };

    // For confirmed Pineapple devices
    struct ConfirmedPineScan {
        uint8_t mac[6];
        String detection_type;
        String essid;
        uint8_t channel;
        int8_t rssi;
        bool displayed;
    };
    LinkedList<PineScanTracker>* pinescan_trackers;
    LinkedList<ConfirmedPineScan>* confirmed_pinescan;
    bool pinescan_list_full_reported;
    
    // Security Conditions For Pineapple detection
    enum SecurityCondition {
        NONE = 0x00,
        SUSPICIOUS_WHEN_OPEN = 0x01,
        SUSPICIOUS_WHEN_PROTECTED = 0x02,
        SUSPICIOUS_ALWAYS = 0x04
    };

    // SuspiciousVendor struct
    struct SuspiciousVendor {
        const char* vendor_name;
        uint8_t security_flags;
        uint32_t ouis[20];                 // Array of OUIs (max 20 per vendor)
        uint8_t oui_count;                 // Number of OUIs for this vendor
    };

    // Declare the table for Pineapple
    static const SuspiciousVendor suspicious_vendors[];
    static const int NUM_SUSPICIOUS_VENDORS;

    // Track for AP list limit (Uninitialised, Done in RunSetup)
    bool ap_list_full_reported;

    // MULTI SSID STRUCTS

    struct MultiSSIDTracker {
        uint8_t mac[6];
        uint16_t ssid_hashes[MULTISSID_THRESHOLD];
        uint8_t unique_ssid_count;
        bool reported;
    };

    // New struct for confirmed MultiSSID devices
    struct ConfirmedMultiSSID {
        uint8_t mac[6];
        String essid;
        uint8_t channel;
        int8_t rssi;
        uint8_t ssid_count;
        bool displayed;
    };
    LinkedList<MultiSSIDTracker>* multissid_trackers;
    LinkedList<ConfirmedMultiSSID>* confirmed_multissid;
    bool multissid_list_full_reported;

    // barebones packet
    uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, //Frame Control, Duration
                    /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                    /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //Source address - overwritten later
                    /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //BSSID - overwritten to the same as the source address
                    /*22*/  0xc0, 0x6c, //Seq-ctl
                    /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
                    /*32*/  0x64, 0x00, //Beacon interval
                    /*34*/  0x01, 0x04, //Capability info
                    /* SSID */
                    /*36*/  0x00
                    };

    uint8_t prob_req_packet[128] = {0x40, 0x00, 0x00, 0x00, 
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
                                  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Dest
                                  0x01, 0x00, // Sequence
                                  0x00, // SSID Parameter
                                  0x00, // SSID Length
                                  /* SSID */
                                  };

    uint8_t deauth_frame_default[26] = {
                              0xc0, 0x00, 0x3a, 0x01,
                              0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0xf0, 0xff, 0x02, 0x00
                          };

    enum EBLEPayloadType
    {
      Microsoft,
      Apple,
      Samsung,
      Google,
      FlipperZero,
      Airtag
    };

      #ifdef HAS_BT

      struct BLEData
      {
        NimBLEAdvertisementData AdvData;
        NimBLEAdvertisementData ScanData;
      };

      struct WatchModel
      {
          uint8_t value;
          const char *name;
      };

      WatchModel* watch_models = nullptr;

      static void scanCompleteCB(BLEScanResults scanResults);
      NimBLEAdvertisementData GetUniversalAdvertisementData(EBLEPayloadType type);
    #endif

    void pingScan();
    void portScan(uint8_t scan_mode = WIFI_PORT_SCAN_ALL);
    bool isHostAlive(IPAddress ip);
    bool checkHostPort(IPAddress ip, uint16_t port, uint16_t timeout = 100);
    String extractManufacturer(const uint8_t* payload);
    int checkMatchAP(char addr[]);
    bool beaconHasWPS(const uint8_t* payload, int len);
    uint8_t getSecurityType(const uint8_t* beacon, uint16_t len);
    void addAnalyzerValue(int16_t value, int rssi_avg, int16_t target_array[], int array_size);
    bool seen_mac(unsigned char* mac);
    bool mac_cmp(struct mac_addr addr1, struct mac_addr addr2);
    void save_mac(unsigned char* mac);
    void clearMacHistory();
    void executeWarDrive();
    void executeSourApple();
    void executeSpoofAirtag();
    void executeSwiftpairSpam(EBLEPayloadType type);
    void startWardriverWiFi();
    //void generateRandomMac(uint8_t* mac);
    //void generateRandomName(char *name, size_t length);
    String processPwnagotchiBeacon(const uint8_t* frame, int length);

    void startWiFiAttacks(uint8_t scan_mode, uint16_t color, String title_string);

    void signalAnalyzerLoop(uint32_t tick);
    void channelAnalyzerLoop(uint32_t tick);
    void packetRateLoop(uint32_t tick);
    void packetMonitorMain(uint32_t currentTime);
    void eapolMonitorMain(uint32_t currentTime);
    void updateMidway();
    void tftDrawXScalButtons();
    void tftDrawYScaleButtons();
    void tftDrawChannelScaleButtons();
    void tftDrawColorKey();
    void tftDrawGraphObjects();
    void sendProbeAttack(uint32_t currentTime);
    void sendDeauthAttack(uint32_t currentTime, String dst_mac_str = "ff:ff:ff:ff:ff:ff");
    void sendDeauthFrame(uint8_t bssid[6], int channel, String dst_mac_str = "ff:ff:ff:ff:ff:ff");
    void sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]);
    void broadcastRandomSSID(uint32_t currentTime);
    void broadcastCustomBeacon(uint32_t current_time, ssid custom_ssid);
    void broadcastCustomBeacon(uint32_t current_time, AccessPoint custom_ssid);
    void broadcastSetSSID(uint32_t current_time, const char* ESSID);
    void RunAPScan(uint8_t scan_mode, uint16_t color);
    void RunGPSInfo();
    void RunGPSNmea();
    void RunMimicFlood(uint8_t scan_mode, uint16_t color);
    void RunPwnScan(uint8_t scan_mode, uint16_t color);
    void RunPineScan(uint8_t scan_mode, uint16_t color);
    void RunMultiSSIDScan(uint8_t scan_mode, uint16_t color);
    void RunBeaconScan(uint8_t scan_mode, uint16_t color);
    void RunRawScan(uint8_t scan_mode, uint16_t color);
    void RunStationScan(uint8_t scan_mode, uint16_t color);
    void RunDeauthScan(uint8_t scan_mode, uint16_t color);
    void RunEapolScan(uint8_t scan_mode, uint16_t color);
    void RunProbeScan(uint8_t scan_mode, uint16_t color);
    void RunPacketMonitor(uint8_t scan_mode, uint16_t color);
    void RunBluetoothScan(uint8_t scan_mode, uint16_t color);
    void RunSourApple(uint8_t scan_mode, uint16_t color);
    void RunSwiftpairSpam(uint8_t scan_mode, uint16_t color);
    void RunLvJoinWiFi(uint8_t scan_mode, uint16_t color);
    void RunEvilPortal(uint8_t scan_mode, uint16_t color);
    void RunPingScan(uint8_t scan_mode, uint16_t color);
    void RunPortScanAll(uint8_t scan_mode, uint16_t color);
    bool checkMem();
    void parseBSSID(const char* bssidStr, uint8_t* bssid);


  public:
    WiFiScan();

    //AccessPoint ap_list;

    //LinkedList<ssid>* ssids;

    // Stuff for RAW stats
    uint32_t mgmt_frames = 0;
    uint32_t data_frames = 0;
    uint32_t beacon_frames = 0;
    uint32_t req_frames = 0;
    uint32_t resp_frames = 0;
    uint32_t deauth_frames = 0;
    uint32_t eapol_frames = 0;
    int8_t min_rssi = 0;
    int8_t max_rssi = -128;

    bool force_pmkid = false;
    bool force_probe = false;
    bool save_pcap = false;

    String analyzer_name_string = "";
    
    uint8_t analyzer_frames_recvd = 0;

    bool analyzer_name_update = false;

    uint8_t set_channel = 1;

    uint8_t old_channel = 0;

    int16_t _analyzer_value = 0;

    bool orient_display = false;
    bool wifi_initialized = false;
    bool ble_initialized = false;
    bool wifi_connected = false;

    String free_ram = "";
    String old_free_ram = "";
    String connected_network = "";

    IPAddress ip_addr;
    IPAddress gateway;
    IPAddress subnet;

    IPAddress current_scan_ip;

    uint16_t current_scan_port = 1;

    String dst_mac = "ff:ff:ff:ff:ff:ff";
    byte src_mac[6] = {};

    #ifdef HAS_SCREEN
      int16_t _analyzer_values[TFT_WIDTH];
      int16_t _temp_analyzer_values[TFT_WIDTH];
    #endif

    String current_mini_kb_ssid = "";

    const String alfa = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789-=[];',./`\\_+{}:\"<>?~|!@#$%^&*()";

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    #ifndef HAS_DUAL_BAND
      wifi_init_config_t cfg2 = { \
          .event_handler = &esp_event_send_internal, \
          .osi_funcs = &g_wifi_osi_funcs, \
          .wpa_crypto_funcs = g_wifi_default_wpa_crypto_funcs, \
          .static_rx_buf_num = 6,\
          .dynamic_rx_buf_num = 6,\
          .tx_buf_type = 0,\
          .static_tx_buf_num = 1,\
          .dynamic_tx_buf_num = WIFI_DYNAMIC_TX_BUFFER_NUM,\
          .cache_tx_buf_num = 0,\
          .csi_enable = false,\
          .ampdu_rx_enable = false,\
          .ampdu_tx_enable = false,\
          .amsdu_tx_enable = false,\
          .nvs_enable = false,\
          .nano_enable = WIFI_NANO_FORMAT_ENABLED,\
          .rx_ba_win = 6,\
          .wifi_task_core_id = WIFI_TASK_CORE_ID,\
          .beacon_max_len = 752, \
          .mgmt_sbuf_num = 8, \
          .feature_caps = g_wifi_feature_caps, \
          .sta_disconnected_pm = WIFI_STA_DISCONNECTED_PM_ENABLED,  \
          .espnow_max_encrypt_num = 0, \
          .magic = WIFI_INIT_CONFIG_MAGIC\
      };
    #else
      wifi_init_config_t cfg2 = WIFI_INIT_CONFIG_DEFAULT();
    #endif

    wifi_config_t ap_config;

    #ifdef HAS_SCREEN
      int8_t checkAnalyzerButtons(uint32_t currentTime);
    #endif
    void setMac();
    void renderRawStats();
    void renderPacketRate();
    void displayAnalyzerString(String str);
    String security_int_to_string(int security_type);
    char* stringToChar(String string);
    void RunSetup();
    int clearSSIDs();
    int clearAPs();
    int clearIPs();
    int clearAirtags();
    int clearFlippers();
    int clearStations();
    int clearPineScanTrackers();
    int clearMultiSSID();
    bool addSSID(String essid);
    int generateSSIDs(int count = 20);
    bool shutdownWiFi();
    bool shutdownBLE();
    bool scanning();
    bool joinWiFi(String ssid, String password, bool gui = true);
    String getStaMAC();
    String getApMAC();
    String freeRAM();
    void changeChannel();
    void changeChannel(int chan);
    void RunAPInfo(uint16_t index, bool do_display = true);
    void RunInfo();
    //void RunShutdownBLE();
    void RunSetMac(uint8_t * mac, bool ap = true);
    void RunGenerateRandomMac(bool ap = true);
    void RunGenerateSSIDs(int count = 20);
    void RunClearSSIDs();
    void RunClearAPs();
    void RunClearStations();
    void RunSaveSSIDList(bool save_as = true);
    void RunLoadSSIDList();
    void RunSaveAPList(bool save_as = true);
    void RunLoadAPList();
    void RunSaveATList(bool save_as = true);
    void RunLoadATList();
    void channelHop();
    uint8_t currentScanMode = 0;
    void main(uint32_t currentTime);
    void StartScan(uint8_t scan_mode, uint16_t color = 0);
    void StopScan(uint8_t scan_mode);
    void setBaseMacAddress(uint8_t macAddr[6]);
    //const char* generateRandomName();

    bool save_serial = false;
    void startPcap(String file_name);
    void startLog(String file_name);
    //String macToString(const Station& station);

    static void getMAC(char *addr, uint8_t* data, uint16_t offset);
    static void pwnSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void rawSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void stationSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void apSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type);
    static void deauthSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void probeSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void beaconListSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void activeEapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void eapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static void pineScanSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type); // Pineapple
    static int extractPineScanChannel(const uint8_t* payload, int len); // Pineapple
    static void multiSSIDSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type); // MultiSSID

    /*#ifdef HAS_BT
      enum EBLEPayloadType
      {
        Microsoft,
        Apple,
        Samsung,
        Google
      };

      struct BLEData
      {
        NimBLEAdvertisementData AdvData;
        NimBLEAdvertisementData ScanData;
      };

      struct WatchModel
      {
          uint8_t value;
          const char *name;
      };

      WatchModel* watch_models = nullptr;

      const WatchModel watch_models[] = {
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
    #endif*/
};
#endif
