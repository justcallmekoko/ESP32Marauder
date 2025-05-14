#pragma once
#ifndef utils_h
#define utils_h

#include <Arduino.h>
#include <vector>

#include "configs.h"

#include "esp_heap_caps.h"

struct mac_addr {
   unsigned char bytes[6];
};

struct Station {
  uint8_t mac[6];
  bool selected;
  uint16_t packets;
  uint16_t ap;
};

const char apple_ouis[][9] PROGMEM = {
  "00:17:F2", "00:1E:C2", "00:26:08", "F8:1E:DF", "BC:92:6B", 
  "28:E0:2C", "3C:07:54", "7C:D1:C3", "DC:A9:04", "F0:D1:A9",
  "C0:2C:5C", "00:03:93", "00:03:94", "00:03:95", "00:03:96",
  "00:03:97", "00:03:98", "00:03:99", "00:03:9A", "00:03:9B",
  "00:03:9C", "00:03:9D", "00:03:9E", "00:03:9F", "00:03:A0",
  "00:03:A1", "00:03:A2", "00:03:A3", "00:03:A4", "00:03:A5",
  "00:03:A6", "00:03:A7", "00:03:A8", "00:03:A9", "00:03:AA",
  "00:03:AB", "00:03:AC", "00:03:AD", "00:03:AE", "00:03:AF",
  "00:03:B0", "00:03:B1", "00:03:B2", "00:03:B3", "00:03:B4",
  "00:03:B5", "00:03:B6", "00:03:B7", "00:03:B8", "00:03:B9",
  "00:03:BA", "00:03:BB", "00:03:BC", "00:03:BD", "00:03:BE",
  "00:03:BF", "00:03:C0", "00:03:C1", "00:03:C2", "00:03:C3",
  "00:03:C4", "00:03:C5", "00:03:C6", "00:03:C7", "00:03:C8",
  "00:03:C9", "00:03:CA", "00:03:CB", "00:03:CC", "00:03:CD",
  "00:03:CE", "00:03:CF", "00:03:D0", "00:03:D1", "00:03:D2",
  "00:03:D3", "00:03:D4", "00:03:D5", "00:03:D6", "00:03:D7",
  "00:03:D8", "00:03:D9", "00:03:DA", "00:03:DB", "00:03:DC",
  "00:03:DD", "00:03:DE", "00:03:DF", "00:03:E0", "00:03:E1",
  "00:03:E2", "00:03:E3", "00:03:E4", "00:03:E5", "00:03:E6",
  "00:03:E7", "00:03:E8", "00:03:E9", "00:03:EA", "00:03:EB",
  "00:03:EC", "00:03:ED", "00:03:EE", "00:03:EF", "00:03:F0",
  "00:03:F1", "00:03:F2", "00:03:F3", "00:03:F4", "00:03:F5",
  "00:03:F6", "00:03:F7", "00:03:F8", "00:03:F9", "00:03:FA",
  "00:03:FB", "00:03:FC", "00:03:FD", "00:03:FE", "00:03:FF"
};

const char asus_ouis[][9] PROGMEM = {
  "00:0C:6E", "00:0E:A6", "00:11:2F", "00:11:D8", "00:13:D4", "00:15:F2", "00:17:31", "00:18:F3", "00:1A:92",
  "00:1B:FC", "00:1D:60", "00:1E:8C", "00:1F:C6", "00:22:15", "00:23:54", "00:24:8C", "00:26:18", "00:E0:18",
  "04:42:1A", "04:92:26", "04:D4:C4", "04:D9:F5", "08:60:6E", "08:62:66", "08:BF:B8", "0C:9D:92", "10:7B:44",
  "10:7C:61", "10:BF:48", "10:C3:7B", "14:DA:E9", "14:DD:A9", "18:31:BF", "1C:87:2C", "1C:B7:2C", "20:CF:30",
  "24:4B:FE", "2C:4D:54", "2C:56:DC", "2C:FD:A1", "30:5A:3A", "30:85:A9", "34:97:F6", "38:2C:4A", "38:D5:47",
  "3C:7C:3F", "40:16:7E", "40:B0:76", "48:5B:39", "4C:ED:FB", "50:46:5D", "50:EB:F6", "54:04:A6", "54:A0:50",
  "58:11:22", "60:45:CB", "60:A4:4C", "60:CF:84", "70:4D:7B", "70:8B:CD", "74:D0:2B", "78:24:AF", "7C:10:C9",
  "88:D7:F6", "90:E6:BA", "9C:5C:8E", "A0:36:BC", "A8:5E:45", "AC:22:0B", "AC:9E:17", "B0:6E:BF", "BC:AE:C5",
  "BC:EE:7B", "C8:60:00", "C8:7F:54", "CC:28:AA", "D0:17:C2", "D4:5D:64", "D8:50:E6", "E0:3F:49", "E0:CB:4E",
  "E8:9C:25", "F0:2F:74", "F0:79:59", "F4:6D:04", "F8:32:E4", "FC:34:97", "FC:C2:33"
};

const char belkin_ouis[][9] PROGMEM = {
  "00:11:50", "00:17:3F", "00:30:BD", "08:BD:43", "14:91:82", "24:F5:A2", "30:23:03", "80:69:1A", "94:10:3E",
  "94:44:52", "B4:75:0E", "C0:56:27", "C4:41:1E", "D8:EC:5E", "E8:9F:80", "EC:1A:59", "EC:22:80"
};

const char cisco_ouis[][9] PROGMEM = {
  "00:1B:0D", "00:1D:45", "00:1E:7A", "00:25:9C", "00:50:56", 
  "40:55:39", "58:8D:09", "A4:4C:C8", "F8:0F:F9"
};

const char dlink_ouis[][9] PROGMEM = {
  "00:05:5D", "00:0D:88", "00:0F:3D", "00:11:95", "00:13:46", "00:15:E9", "00:17:9A", "00:19:5B", "00:1B:11",
  "00:1C:F0", "00:1E:58", "00:21:91", "00:22:B0", "00:24:01", "00:26:5A", "00:AD:24", "04:BA:D6", "08:5A:11",
  "0C:0E:76", "0C:B6:D2", "10:62:EB", "10:BE:F5", "14:D6:4D", "18:0F:76", "1C:5F:2B", "1C:7E:E5", "1C:AF:F7",
  "1C:BD:B9", "28:3B:82", "30:23:03", "34:08:04", "34:0A:33", "3C:1E:04", "3C:33:32", "40:86:CB", "40:9B:CD",
  "54:B8:0A", "5C:D9:98", "60:63:4C", "64:29:43", "6C:19:8F", "6C:72:20", "74:44:01", "74:DA:DA", "78:32:1B",
  "78:54:2E", "78:98:E8", "80:26:89", "84:C9:B2", "88:76:B9", "90:8D:78", "90:94:E4", "9C:D6:43", "A0:63:91",
  "A0:AB:1B", "A4:2A:95", "A8:63:7D", "AC:F1:DF", "B4:37:D8", "B8:A3:86", "BC:0F:9A", "BC:22:28", "BC:F6:85",
  "C0:A0:BB", "C4:A8:1D", "C4:E9:0A", "C8:78:7D", "C8:BE:19", "C8:D3:A3", "CC:B2:55", "D8:FE:E3", "DC:EA:E7",
  "E0:1C:FC", "E4:6F:13", "E8:CC:18", "EC:22:80", "EC:AD:E0", "F0:7D:68", "F0:B4:D2", "F4:8C:EB", "F8:E9:03",
  "FC:75:16"
};

const char google_ouis[][9] PROGMEM = {
  "3C:5A:B4", "5C:BF:C0", "78:4F:43", "A4:77:33", "D4:97:0B", "F0:72:8C"
};

const char huawei_ouis[][9] PROGMEM = {
  "00:1A:2B", "28:FF:3C", "5C:4C:A9", "8C:71:F8", "C8:D1:5E", 
  "E4:4C:A9", "F4:12:FA"
};

const char lg_ouis[][9] PROGMEM = {
  "00:17:C0", "18:AF:8F", "38:2D:AE", "5C:87:9C", "68:27:37", 
  "78:5D:C8", "94:65:2D", "A4:77:33", "C4:43:8F"
};

const char linksys_ouis[][9] PROGMEM = {
  "00:04:5A", "00:06:25", "00:0C:41", "00:0E:08", "00:0F:66", "00:12:17", "00:13:10", "00:14:BF", "00:16:B6",
  "00:18:39", "00:18:F8", "00:1A:70", "00:1C:10", "00:1D:7E", "00:1E:E5", "00:21:29", "00:22:6B", "00:23:69",
  "00:25:9C", "00:23:54", "00:24:B2", "00:31:92", "00:5F:67", "10:27:F5", "14:EB:B6", "1C:61:B4", "20:36:26",
  "28:87:BA", "30:5A:3A", "2C:FD:A1", "30:23:03", "30:46:9A", "40:ED:00", "48:22:54", "50:91:E3", "54:AF:97",
  "5C:A2:F4", "5C:A6:E6", "5C:E9:31", "60:A4:B7", "68:7F:F0", "6C:5A:B0", "78:8C:B5", "7C:C2:C6", "9C:53:22",
  "9C:A2:F4", "A8:42:A1", "AC:15:A2", "B0:A7:B9", "B4:B0:24", "C0:06:C3", "CC:68:B6", "E8:48:B8", "F0:A7:31"
};

const char netgear_ouis[][9] PROGMEM = {
  "00:09:5B", "00:0F:B5", "00:14:6C", "00:1B:2F", "00:1E:2A", "00:1F:33", "00:22:3F", "00:22:4B", "00:26:F2",
  "00:8E:F2", "08:02:8E", "08:36:C9", "08:BD:43", "10:0C:6B", "10:0D:7F", "10:DA:43", "14:59:C0", "20:4E:7F",
  "20:E5:2A", "28:80:88", "28:94:01", "28:C6:8E", "2C:30:33", "2C:B0:5D", "30:46:9A", "34:98:B5", "38:94:ED",
  "3C:37:86", "40:5D:82", "44:A5:6E", "4C:60:DE", "50:4A:6E", "50:6A:03", "54:07:7D", "58:EF:68", "60:38:E0",
  "6C:B0:CE", "6C:CD:D6", "74:44:01", "80:37:73", "84:1B:5E", "8C:3B:AD", "94:18:65", "9C:3D:CF", "9C:C9:EB",
  "9C:D3:6D", "A0:04:60", "A0:21:B7", "A0:40:A0", "A4:2B:8C", "B0:39:56", "B0:7F:B9", "B0:B9:8A", "BC:A5:11",
  "C0:3F:0E", "C0:FF:D4", "C4:04:15", "C4:3D:C7", "C8:9E:43", "CC:40:D0", "DC:EF:09", "E0:46:9A", "E0:46:EE",
  "E0:91:F5", "E4:F4:C6", "E8:FC:AF", "F8:73:94"
};

const char oneplus_ouis[][9] PROGMEM = {
  "08:EC:A9", "30:9C:23", "38:78:62", "64:A2:F9", "74:AC:B9", 
  "A8:14:51", "B4:86:55", "D8:CB:8A", "F4:8C:50"
};

const char samsung_ouis[][9] PROGMEM = {
  "00:12:47", "00:15:99", "00:16:6B", "00:1B:FC", "10:5F:06", 
  "18:59:36", "20:02:AF", "24:4B:03", "38:2D:3D", "40:B8:37",
  "00:1F:12", "00:1D:0F", "00:1A:9A", "00:19:E1", "00:18:FF", 
  "00:17:77", "00:16:72", "00:14:69", "00:13:65", "00:12:60",
  "30:37:0D", "40:8C:47", "50:55:5F", "60:47:A1", "70:0A:AD",
  "80:61:43", "90:48:F7", "A0:4C:CB", "B0:5E:93", "C0:3D:F5",
  "D0:31:AA", "E0:4F:02", "F0:3B:94"
};

const char sony_ouis[][9] PROGMEM = {
  "00:19:C5", "00:1B:59", "00:1E:DC", "10:68:3F", "54:42:49", 
  "A8:E3:EE", "B8:F9:34", "CC:5D:4E", "E8:89:2F"
};

const char tplink_ouis[][9] PROGMEM = {
  "00:31:92", "00:5F:67", "10:27:F5", "14:EB:B6", "1C:61:B4", "20:36:26", "28:87:BA", "30:DE:4B", "34:60:F9",
  "3C:52:A1", "40:ED:00", "48:22:54", "50:91:E3", "54:AF:97", "5C:62:8B", "5C:A6:E6", "5C:E9:31", "60:A4:B7",
  "68:7F:F0", "6C:5A:B0", "78:8C:B5", "7C:C2:C6", "9C:53:22", "9C:A2:F4", "A8:42:A1", "AC:15:A2", "B0:A7:B9",
  "B4:B0:24", "C0:06:C3", "CC:68:B6", "E8:48:B8", "F0:A7:31"
};

const char xiaomi_ouis[][9] PROGMEM = {
  "04:CF:8C", "18:59:36", "38:1A:2D", "64:B4:73", "78:02:F8", 
  "90:4E:91", "C4:0B:CB", "D0:DB:32"
};

uint8_t getDRAMUsagePercent() {
  //size_t total = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  //size_t free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  size_t free = ESP.getFreeHeap();
  size_t total = ESP.getHeapSize();
  
  if (total == 0) return 0; // Avoid division by zero

  size_t used = total - free;
  uint8_t percent = (used * 100) / total;
  return percent;
}

#ifdef HAS_PSRAM
  uint8_t getPSRAMUsagePercent() {
    //size_t total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    //size_t free  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    size_t total = ESP.getPsramSize();
    size_t free = ESP.getFreePsram();

    if (total == 0) return 0; // Avoid division by zero or PSRAM not available

    size_t used = total - free;
    uint8_t percent = (used * 100) / total;
    return percent;
  }
#endif

String byteArrayToHexString(const std::vector<uint8_t>& byteArray) {
  String result;

  for (size_t i = 0; i < byteArray.size(); i++) {
    // Append the byte in "0xXX" format
    result += "0x";
    if (byteArray[i] < 0x10) {
      result += "0"; // Add leading zero for single-digit hex values
    }
    result += String(byteArray[i], HEX);

    // Add a space between bytes, but not at the end
    if (i < byteArray.size() - 1) {
      result += " ";
    }
  }

  return result;
}

std::vector<uint8_t> hexStringToByteArray(const String& hexString) {
  std::vector<uint8_t> byteArray;

  // Split the input string by spaces
  int startIndex = 0;
  while (startIndex < hexString.length()) {
    // Find the next space or end of string
    int spaceIndex = hexString.indexOf(' ', startIndex);

    // If no space is found, process the last token
    if (spaceIndex == -1) {
      spaceIndex = hexString.length();
    }

    // Extract the "0xXX" part
    String byteString = hexString.substring(startIndex, spaceIndex);

    // Convert "0xXX" to an integer and store it in the vector
    if (byteString.startsWith("0x") || byteString.startsWith("0X")) {
      uint8_t byte = strtol(byteString.c_str() + 2, nullptr, 16);
      byteArray.push_back(byte);
    }

    // Move the start index to the next byte
    startIndex = spaceIndex + 1;
  }

  return byteArray;
}

void generateRandomName(char *name, size_t length) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    
    // Generate the first character as uppercase
    name[0] = 'A' + (rand() % 26);
    
    // Generate the remaining characters as lowercase
    for (size_t i = 1; i < length - 1; ++i) {
        name[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    name[length - 1] = '\0';  // Null-terminate the string
}

const char* generateRandomName() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = rand() % 10 + 1; // Generate a random length between 1 and 10
  char* randomName = (char*)malloc((len + 1) * sizeof(char)); // Allocate memory for the random name
  for (int i = 0; i < len; ++i) {
    randomName[i] = charset[rand() % strlen(charset)]; // Select random characters from the charset
  }
  randomName[len] = '\0'; // Null-terminate the string
  return randomName;
}

void generateRandomMac(uint8_t* mac) {
  // Set the locally administered bit and unicast bit for the first byte
  mac[0] = 0x02; // The locally administered bit is the second least significant bit

  // Generate the rest of the MAC address
  for (int i = 1; i < 6; i++) {
    mac[i] = random(0, 255);
  }
}

String macToString(const Station& station) {
  char macStr[18]; // 6 pairs of hex digits + 5 colons + null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           station.mac[0], station.mac[1], station.mac[2],
           station.mac[3], station.mac[4], station.mac[5]);
  return String(macStr);
}

String macToString(uint8_t macAddr[6]) {
  char macStr[18]; // 17 characters for "XX:XX:XX:XX:XX:XX" + 1 null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
    macAddr[0], macAddr[1], macAddr[2], 
    macAddr[3], macAddr[4], macAddr[5]);
  return String(macStr);
}

void convertMacStringToUint8(const String& macStr, uint8_t macAddr[6]) {
    // Ensure the input string is in the format "XX:XX:XX:XX:XX:XX"
    if (macStr.length() != 17) {
        Serial.println("Invalid MAC address format");
        return;
    }

    // Parse the MAC address string and fill the uint8_t array
    for (int i = 0; i < 6; i++) {
        macAddr[i] = (uint8_t)strtol(macStr.substring(i * 3, i * 3 + 2).c_str(), nullptr, 16);
    }
}

// Function to check if the given MAC address matches any known OUI
const char* getManufacturer(const char *addr) {
  static char oui[9]; // Temporary buffer for extracted OUI

  // Extract the first three bytes (OUI) from addr
  strncpy(oui, addr, 8);
  oui[8] = '\0'; // Ensure null termination

  // Convert the addr (OUI) to lowercase
  for (int i = 0; i < 8; i++) {
    oui[i] = tolower(oui[i]);
  }

  // Helper macro to check against an array stored in PROGMEM
  #define CHECK_OUI(manufacturer, list) \
    for (uint8_t i = 0; i < sizeof(list) / sizeof(list[0]); i++) { \
      char storedOUI[9]; \
      strcpy_P(storedOUI, list[i]); \
      /* Convert the stored OUI to lowercase */ \
      for (int j = 0; j < 8; j++) { \
        storedOUI[j] = tolower(storedOUI[j]); \
      } \
      if (strcmp(oui, storedOUI) == 0) return manufacturer; \
    }

  // Check against known manufacturers
  CHECK_OUI("Apple", apple_ouis);
  CHECK_OUI("Asus", asus_ouis);
  CHECK_OUI("Belkin", belkin_ouis);
  CHECK_OUI("Cisco", cisco_ouis);
  CHECK_OUI("DLink", dlink_ouis);
  CHECK_OUI("Google", google_ouis);
  CHECK_OUI("Huawei", huawei_ouis);
  CHECK_OUI("LG", lg_ouis);
  CHECK_OUI("Linksys", linksys_ouis);
  CHECK_OUI("Netgear", netgear_ouis);
  CHECK_OUI("OnePlus", oneplus_ouis);
  CHECK_OUI("Samsung", samsung_ouis);
  CHECK_OUI("Sony", sony_ouis);
  CHECK_OUI("TP-Link", tplink_ouis);
  CHECK_OUI("Xiaomi", xiaomi_ouis);

  return ""; // Return "Unknown" if no match is found
}

String replaceOUIWithManufacturer(const char *sta_addr) {
  const char *manufacturer = getManufacturer(sta_addr);

  if (manufacturer == nullptr || strlen(manufacturer) == 0) {
    return String(sta_addr); // Return original if no manufacturer found
  }

  // Skip the first 8 characters (3 bytes and 2 colons)
  const char *mac_suffix = sta_addr + 8;

  // Construct the new address: manufacturer + the remaining MAC address (after the first 3 bytes)
  return String(manufacturer) + mac_suffix;
}

#endif