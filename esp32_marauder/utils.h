#pragma once
#ifndef utils_h
#define utils_h

#include <Arduino.h>

struct mac_addr {
   unsigned char bytes[6];
};

struct Station {
  uint8_t mac[6];
  bool selected;
};

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

#endif