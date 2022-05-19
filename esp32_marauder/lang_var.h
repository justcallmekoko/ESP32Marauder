#ifndef lang_var_h
#define lang_var_h

#include "configs.h"

//Starting window texts
const char text0_0[] PROGMEM = "Giving room for HardwareSerial...";
const char text0_1[] PROGMEM = "Started Serial";
const char text0_2[] PROGMEM = "Checked RAM";
const char text0_3[] PROGMEM = "Initialized SD Card";
const char text0_4[] PROGMEM = "Failed to Initialize SD Card";
const char text0_5[] PROGMEM = "Checked battery configuration";
const char text0_6[] PROGMEM = "Initialized temperature interface";
const char text0_7[] PROGMEM = "Initialized LED Interface";
const char text0_8[] PROGMEM = "Starting...";

//Single library (action) texts/Often used
const char text00[] PROGMEM = "Battery Level changed: ";
const char text01[] PROGMEM = "file closed";
const char text02[] PROGMEM = "Failed to open file '";
const char text03[] PROGMEM = "ON";
const char text04[] PROGMEM = "OFF";
const char text05[] PROGMEM = "Load";
const char text06[] PROGMEM = "Save As";
const char text07[] PROGMEM = "Exit";
const char text08[] PROGMEM = "Settings";
const char text09[] PROGMEM = "Back";
const char text10[] PROGMEM = "Channel:";
const char text11[] PROGMEM = "Touch screen to exit";
const char text12[] PROGMEM = "Cancel";
const char text13[] PROGMEM = "Save";
const char text14[] PROGMEM = "Yes";
const char text15[] PROGMEM = "Opening /update.bin...";
const char text16[] PROGMEM = "Close";
const char text17[] PROGMEM = "FAIL";
const char text18[] PROGMEM = "packets/sec: ";


//Menufunctions.cpp texts
const char text1_0[] PROGMEM = "SSID List";
const char text1_1[] PROGMEM = "Add SSIDs";
const char text1_2[] PROGMEM = "SSID: ";
const char text1_3[] PROGMEM = "Password:";
const char text1_4[] PROGMEM = "Setting disabled";
const char text1_5[] PROGMEM = "Setting on";
const char text1_6[] PROGMEM = "ESP32 Marauder ";
const char text1_7[] PROGMEM = "WiFi ";
const char text1_8[] PROGMEM = "Bad USB ";
const char text1_9[] PROGMEM = "Device ";
const char text1_10[] PROGMEM = "General Apps ";
const char text1_11[] PROGMEM = "Updating... ";
const char text1_12[] PROGMEM = "Select Method ";
const char text1_13[] PROGMEM = "Confirm Update ";
const char text1_14[] PROGMEM = "ESP8266 Update ";
const char text1_15[] PROGMEM = "Update Firmware ";
const char text1_16[] PROGMEM = "Language ";
const char text1_17[] PROGMEM = "Device Info ";
const char text1_18[] PROGMEM = "Settings ";
const char text1_19[] PROGMEM = "Bluetooth ";
const char text1_20[] PROGMEM = "WiFi Sniffers ";
const char text1_21[] PROGMEM = "WiFi Attacks ";
const char text1_22[] PROGMEM = "WiFi General ";
const char text1_23[] PROGMEM = "Bluetooth Sniffers ";
const char text1_24[] PROGMEM = "Bluetooth General ";
const char text1_25[] PROGMEM = "Shutdown WiFi ";
const char text1_26[] PROGMEM = "Shutdown BLE ";
const char text1_27[] PROGMEM = "Generate SSIDs ";
const char text1_28[] PROGMEM = "Clear SSIDs ";
const char text1_29[] PROGMEM = "Clear APs ";
const char text1_30[] PROGMEM = "Reboot";
const char text1_31[] PROGMEM = "Sniffers";
const char text1_32[] PROGMEM = "Attacks";
const char text1_33[] PROGMEM = "General";
const char text1_34[] PROGMEM = "Bluetooth Sniffer";
const char text1_35[] PROGMEM = "Detect Card Skimmers";
const char text1_36[] PROGMEM = "Test BadUSB";
const char text1_37[] PROGMEM = "Run Ducky Script";
const char text1_38[] PROGMEM = "Draw";
const char text1_39[] PROGMEM = "Web Update";
const char text1_40[] PROGMEM = "SD Update";
const char text1_41[] PROGMEM = "ESP8266 Update";
const char text1_42[] PROGMEM = "Probe Request Sniff";
const char text1_43[] PROGMEM = "Beacon Sniff";
const char text1_44[] PROGMEM = "Deauth Sniff";
const char text1_45[] PROGMEM = "Packet Monitor";
const char text1_46[] PROGMEM = "EAPOL/PMKID Scan";
const char text1_47[] PROGMEM = "Detect Pwnagotchi";
const char text1_48[] PROGMEM = "Detect Espressif";
const char text1_49[] PROGMEM = "Scan APs";
const char text1_50[] PROGMEM = "Beacon Spam List";
const char text1_51[] PROGMEM = "Beacon Spam Random";
const char text1_52[] PROGMEM = "Rick Roll Beacon";
const char text1_53[] PROGMEM = "Probe Req Flood";
const char text1_54[] PROGMEM = "Deauth Flood";
const char text1_55[] PROGMEM = "Join WiFi";
const char text1_56[] PROGMEM = "Select APs";


//SDInterface.cpp texts
const char text2_0[] PROGMEM = "Error, could not find update.bin";
const char text2_1[] PROGMEM = "Starting SD Update...";
const char text2_2[] PROGMEM = "Error, update.bin is empty";
const char text2_3[] PROGMEM = "\nRebooting...\n";
const char text2_4[] PROGMEM = "Could not load update.bin from /";
const char text2_5[] PROGMEM = "File size: ";
const char text2_6[] PROGMEM = "Writing file to partition...";
const char text2_7[] PROGMEM = "Written: ";
const char text2_8[] PROGMEM = "Written only : ";
const char text2_9[] PROGMEM = ". Retry?";
const char text2_10[] PROGMEM = " successfully";
const char text2_11[] PROGMEM = "Update complete";
const char text2_12[] PROGMEM = "Update could not complete";
const char text2_13[] PROGMEM = "Error Occurred. Error #: ";
const char text2_14[] PROGMEM = "Not enough space to begin OTA";

//Web.cpp texts
const char text3_0[] PROGMEM = "Configuring update server...\n\n";
const char text3_1[] PROGMEM = "IP address: ";
const char text3_2[] PROGMEM = "Update: ";
const char text3_3[] PROGMEM = "Bytes complete: ";
const char text3_4[] PROGMEM = "Update Success: ";
const char text3_5[] PROGMEM = "\nCompleted update server setup";

//WiFiScan.cpp texts
const char text4_0[] PROGMEM = " RSSI: ";
const char text4_1[] PROGMEM = "Potential Skimmer: ";
const char text4_2[] PROGMEM = "Already Connected";
const char text4_3[] PROGMEM = "Failed to connect";
const char text4_4[] PROGMEM = "Connected";
const char text4_5[] PROGMEM = "Force PMKID";
const char text4_6[] PROGMEM = "Force Probe";
const char text4_7[] PROGMEM = "Save PCAP";
const char text4_8[] PROGMEM = "Probe Flood";
const char text4_9[] PROGMEM = "Clearing APs...";
const char text4_10[] PROGMEM = "APs Cleared: ";
const char text4_11[] PROGMEM = "Clearing SSIDs...";
const char text4_12[] PROGMEM = "SSIDs Cleared: ";
const char text4_13[] PROGMEM = "Generating SSIDs...";
const char text4_14[] PROGMEM = "SSIDs Generated: ";        //Add spaces before to match : [15]
const char text4_15[] PROGMEM = "    Total SSIDs: ";        //Add spaces beforer to match : [14]
const char text4_16[] PROGMEM = "Shutting down WiFi...";
const char text4_17[] PROGMEM = "WiFi not currently initialized";
const char text4_18[] PROGMEM = "Shutting down BLE...";
const char text4_19[] PROGMEM = "BLE not currently initialized";
const char text4_20[] PROGMEM = "     Firmware: Marauder";      //From 20 to 35 add spaces so : is in line like it is now
const char text4_21[] PROGMEM = "      Version: ";
const char text4_22[] PROGMEM = "      ESP-IDF: ";
const char text4_23[] PROGMEM = "   WSL Bypass: enabled\n";
const char text4_24[] PROGMEM = "   WSL Bypass: disabled\n";
const char text4_25[] PROGMEM = "  Station MAC: ";
const char text4_26[] PROGMEM = "       AP MAC: ";
const char text4_27[] PROGMEM = "     ";
const char text4_28[] PROGMEM = "      SD Card: Connected";
const char text4_29[] PROGMEM = " SD Card Size: ";
const char text4_30[] PROGMEM = "      SD Card: Not Connected";
const char text4_31[] PROGMEM = " SD Card Size: 0";
const char text4_32[] PROGMEM = "   IP5306 I2C: supported";
const char text4_33[] PROGMEM = "  Battery Lvl: ";
const char text4_34[] PROGMEM = "   IP5306 I2C: not supported";
const char text4_35[] PROGMEM = "Internal temp: ";
const char text4_36[] PROGMEM = " Detect Espressif ";
const char text4_37[] PROGMEM = " Detect Pwnagotchi ";
const char text4_38[] PROGMEM = " Beacon Sniffer ";
const char text4_39[] PROGMEM = " Deauthentication Sniffer ";
const char text4_40[] PROGMEM = " Probe Request Sniffer ";
const char text4_41[] PROGMEM = " Bluetooth Sniff ";
const char text4_42[] PROGMEM = " Detect Card Skimmers ";
const char text4_43[] PROGMEM = "Scanning for\nBluetooth-enabled skimmers\nHC-03, HC-05, and HC-06...";
const char text4_44[] PROGMEM = " AP Scan ";

//Making tables
const char *text_table0[] PROGMEM = {text0_0,text0_1, text0_2, text0_3, text0_4, text0_5, text0_6, text0_7, text0_8};
const char *text_table1[] PROGMEM = {text1_0,text1_1,text1_2,text1_3,text1_4,text1_5,text1_6,text1_7,text1_8,text1_9,text1_10,text1_11,text1_12,text1_13,text1_14,text1_15,text1_16,text1_17,text1_18,text1_19,text1_20,text1_21,text1_22,text1_23,text1_24,text1_25,text1_26,text1_27,text1_28,text1_29,text1_30,text1_31,text1_32,text1_33,text1_34,text1_35,text1_36,text1_37,text1_38,text1_39,text1_40,text1_41,text1_42,text1_43,text1_44,text1_45,text1_46,text1_47,text1_48,text1_49,text1_50,text1_51,text1_52,text1_53,text1_54,text1_55,text1_56};
const char *text_table2[] PROGMEM = {text2_0,text2_1,text2_2,text2_3,text2_4,text2_5,text2_6,text2_7,text2_8,text2_9,text2_10,text2_11,text2_12,text2_13,text2_14};
const char *text_table3[] PROGMEM = {text3_0,text3_1,text3_2,text3_3,text3_4,text3_5};
const char *text_table4[] PROGMEM = {text4_0,text4_1,text4_2,text4_3,text4_4,text4_5,text4_6,text4_7,text4_8,text4_9,text4_10,text4_11,text4_12,text4_13,text4_14,text4_15,text4_16,text4_17,text4_18,text4_19,text4_20,text4_21,text4_22,text4_23,text4_24,text4_25,text4_26,text4_27,text4_28,text4_29,text4_30,text4_31,text4_32,text4_33,text4_34,text4_35,text4_36,text4_37,text4_38,text4_39,text4_40,text4_41,text4_42,text4_43,text4_44};

#endif
