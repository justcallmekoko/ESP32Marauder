#pragma once
#ifndef utils_h
#define utils_h

#include <Arduino.h>
#include <vector>
#include <WiFi.h>

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

struct ProbeReqSsid {
    String essid;
    bool selected;
    uint8_t requests;
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

String hexDump(const uint8_t *buf, size_t len) {
  String out;
  out.reserve(len * 3);  // "FF " per byte (approx)

  for (size_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) {
      out += '0';
    }
    out += String(buf[i], HEX);

    if (i < len - 1) {
      out += ' ';
    }
  }

  out.toUpperCase();
  return out;
}

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

String macToString(const uint8_t macAddr[6]) {
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
/*const char* getManufacturer(const char *addr) {
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
       \
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
}*/

IPAddress getNextIP(IPAddress currentIP, IPAddress subnetMask) {
  // Convert IPAddress to uint32_t
  uint32_t ipInt = (currentIP[0] << 24) | (currentIP[1] << 16) | (currentIP[2] << 8) | currentIP[3];
  uint32_t maskInt = (subnetMask[0] << 24) | (subnetMask[1] << 16) | (subnetMask[2] << 8) | subnetMask[3];

  uint32_t networkBase = ipInt & maskInt;
  uint32_t broadcast = networkBase | ~maskInt;

  uint32_t nextIP = ipInt + 1;

  if (nextIP <= networkBase) {
    nextIP = networkBase + 1;
  }
  if (nextIP >= broadcast) {
    return IPAddress(0, 0, 0, 0); // no more IPs
  }

  return IPAddress(
    (nextIP >> 24) & 0xFF,
    (nextIP >> 16) & 0xFF,
    (nextIP >> 8) & 0xFF,
    nextIP & 0xFF
  );
}

IPAddress getPrevIP(IPAddress currentIP, IPAddress subnetMask, uint16_t stepsBack) {
  // Convert IPAddress to uint32_t
  uint32_t ipInt = (currentIP[0] << 24) | (currentIP[1] << 16) | (currentIP[2] << 8) | currentIP[3];
  uint32_t maskInt = (subnetMask[0] << 24) | (subnetMask[1] << 16) | (subnetMask[2] << 8) | subnetMask[3];

  uint32_t networkBase = ipInt & maskInt;
  uint32_t broadcast = networkBase | ~maskInt;

  uint32_t prevIP = ipInt - stepsBack;

  // Ensure prevIP is not below the usable range
  if (prevIP <= networkBase) {
    return IPAddress(0, 0, 0, 0);  // No more IPs
  }

  return IPAddress(
    (prevIP >> 24) & 0xFF,
    (prevIP >> 16) & 0xFF,
    (prevIP >> 8) & 0xFF,
    prevIP & 0xFF
  );
}

uint16_t getNextPort(uint16_t port) {
  return port + 1;
}

#endif