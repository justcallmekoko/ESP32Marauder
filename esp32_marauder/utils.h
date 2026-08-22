#pragma once
#ifndef utils_h
#define utils_h

#include <Arduino.h>
#include <vector>
#include <WiFi.h>

#include "configs.h"
#include "IPv4Range.h"
#include "MarauderMacAddress.h"

#include "esp_heap_caps.h"
#include "mbedtls/base64.h"

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

inline uint8_t getDRAMUsagePercent() {
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
  inline uint8_t getPSRAMUsagePercent() {
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

inline uint16_t extract16BitFromUUID(String uuid) {
  if(uuid.length() == 36) {
    // Extract characters at positions 4-7 (the XXXX part)
    String hex = uuid.substring(4, 8);
    return (uint16_t)strtol(hex.c_str(), NULL, 16);
  }
  return 0;
}

inline String hexDump(const uint8_t *buf, size_t len) {
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

inline String byteArrayToHexString(const std::vector<uint8_t>& byteArray) {
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

inline std::vector<uint8_t> hexStringToByteArray(const String& hexString) {
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

inline void generateRandomName(char *name, size_t length) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    
    // Generate the first character as uppercase
    name[0] = 'A' + (rand() % 26);
    
    // Generate the remaining characters as lowercase
    for (size_t i = 1; i < length - 1; ++i) {
        name[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    name[length - 1] = '\0';  // Null-terminate the string
}

inline const char* generateRandomName() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = rand() % 10 + 1; // Generate a random length between 1 and 10
  char* randomName = (char*)malloc((len + 1) * sizeof(char)); // Allocate memory for the random name
  for (int i = 0; i < len; ++i) {
    randomName[i] = charset[rand() % strlen(charset)]; // Select random characters from the charset
  }
  randomName[len] = '\0'; // Null-terminate the string
  return randomName;
}

inline void generateRandomMac(uint8_t* mac) {
  // Set the locally administered bit and unicast bit for the first byte
  mac[0] = 0x02; // The locally administered bit is the second least significant bit

  // Generate the rest of the MAC address
  for (int i = 1; i < 6; i++) {
    mac[i] = random(0, 255);
  }
}

inline String macToString(const Station& station) {
  char macStr[marauder::kMacAddressTextLength + 1];
  marauder::formatMacAddress(station.mac, macStr);
  return String(macStr);
}

inline String macToString(uint8_t macAddr[6]) {
  char macStr[marauder::kMacAddressTextLength + 1];
  marauder::formatMacAddress(macAddr, macStr);
  return String(macStr);
}

inline String macToString(const uint8_t macAddr[6]) {
  char macStr[marauder::kMacAddressTextLength + 1];
  marauder::formatMacAddress(macAddr, macStr);
  return String(macStr);
}

inline void convertMacStringToUint8(const String& macStr, uint8_t macAddr[6]) {
  marauder::parseMacAddress(macStr.c_str(), macAddr);
}


inline uint32_t ipAddressToUint32(const IPAddress& address) {
  return (static_cast<uint32_t>(address[0]) << 24) |
         (static_cast<uint32_t>(address[1]) << 16) |
         (static_cast<uint32_t>(address[2]) << 8) |
         static_cast<uint32_t>(address[3]);
}

inline IPAddress uint32ToIPAddress(uint32_t address) {
  return IPAddress(
    (address >> 24) & 0xFF,
    (address >> 16) & 0xFF,
    (address >> 8) & 0xFF,
    address & 0xFF
  );
}

inline marauder::IPv4HostRange getIPHostRange(const IPAddress& address,
                                               const IPAddress& subnetMask) {
  return marauder::ipv4HostRange(ipAddressToUint32(address),
                                 ipAddressToUint32(subnetMask));
}

inline IPAddress getNetworkIP(const IPAddress& address,
                              const IPAddress& subnetMask) {
  return uint32ToIPAddress(getIPHostRange(address, subnetMask).network);
}

inline IPAddress getNextIP(const IPAddress& currentIP,
                           const IPAddress& subnetMask) {
  const marauder::IPv4HostRange range = getIPHostRange(currentIP, subnetMask);
  return uint32ToIPAddress(
      marauder::nextIPv4Host(ipAddressToUint32(currentIP), range));
}

inline IPAddress getPrevIP(const IPAddress& currentIP,
                           const IPAddress& subnetMask, uint16_t stepsBack) {
  const marauder::IPv4HostRange range = getIPHostRange(currentIP, subnetMask);
  return uint32ToIPAddress(marauder::previousIPv4Host(
      ipAddressToUint32(currentIP), stepsBack, range));
}

inline uint16_t getNextPort(uint16_t port) {
  return port + 1;
}

inline String base64Encode(const String& input) {
  size_t outputLen;
  unsigned char output[256];
  mbedtls_base64_encode(output, sizeof(output), &outputLen,
                        (const unsigned char*)input.c_str(), input.length());
  return String((char*)output).substring(0, outputLen);
}

inline static void printHex(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    if (i + 1 < len) Serial.print(' ');
  }
}

inline static void printStringData(const char *label, const std::string &data) {
  Serial.printf("%s [%u]: ", label, (unsigned)data.length());
  printHex((const uint8_t *)data.data(), data.length());
  Serial.println();
}

inline static uint8_t rssiToMenuColor(int rssi) {
  if (rssi >= -60) {
    return TFTGREEN;
  } else if (rssi >= -70) {
    return TFTYELLOW;
  } else if (rssi >= -80) {
    return TFTORANGE;
  } else {
    return TFTRED;
  }
}

inline static uint16_t rssiToColorScaled(int rssi) {
  // Clamp to expected BLE RSSI range
  rssi = constrain(rssi, -100, -40);

  // Map RSSI to 0-255
  uint8_t green = map(rssi, -100, -40, 0, 255);
  uint8_t red   = 255 - green;

  // RGB888 -> RGB565
  return ((red & 0xF8) << 8) |
          ((green & 0xFC) << 3);
}

inline static int rssiToBarWidth(int rssi) {
    // Clamp to the full possible RSSI range
    rssi = constrain(rssi, -127, 0);

    // Map RSSI to a width between 0 and TFT_WIDTH
    return map(rssi, -127, 0, 0, TFT_WIDTH);
}

#endif
