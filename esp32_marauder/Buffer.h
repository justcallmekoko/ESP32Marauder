#pragma once

#ifndef Buffer_h
#define Buffer_h

#include "Arduino.h"
#include "FS.h"
#include "settings.h"
#include "esp_wifi_types.h"
#include "configs.h"

//#define BUF_SIZE 3 * 1024 // Had to reduce buffer size to save RAM. GG @spacehuhn
//#define SNAP_LEN 2324 // max len of each recieved packet

//extern bool useSD;

extern Settings settings_obj;

class Buffer {
  public:
    Buffer();
    void pcapOpen(const char* file_name, fs::FS* fs, bool serial);
    void logOpen(const char* file_name, fs::FS* fs, bool serial);
    void gpxOpen(const char* file_name, fs::FS* fs, bool serial);
    void append(wifi_promiscuous_pkt_t *packet, int len);
    void append(String log);
    void save();
    String getFileName();
  private:
    void createFile(const char* name, bool is_pcap, bool is_gpx = false);
    void open(bool is_pcap);
    void openFile(const char* file_name, fs::FS* fs, bool serial, bool is_pcap, bool is_gpx = false);
    void add(const uint8_t* buf, uint32_t len, bool is_pcap);
    void write(int32_t n);
    void write(uint32_t n);
    void write(uint16_t n);
    void write(const uint8_t* buf, uint32_t len);
    void saveFs(const uint8_t* buf, uint32_t len);       // drain one buffer
    void saveSerial(const uint8_t* buf, uint32_t len);   // drain one buffer
    
    uint8_t* bufA;
    uint8_t* bufB;

    // Shared between the RX-callback writer (WiFi task) and the main-task save() ->
    // volatile + guarded by buf_mux (in the .cpp). ping-pong: save() flips useA to hand
    // the writer a fresh buffer, then drains the old one.
    volatile uint32_t bufSizeA = 0;
    volatile uint32_t bufSizeB = 0;

    volatile bool writing = false; // acceppting writes to buffer
    volatile bool useA = true; // writing to bufA or bufB
    volatile bool saving = false; // (legacy; retained, no longer the sync primitive)

    String fileName = "/0.pcap";
    File file;
    fs::FS* fs;
    bool serial;
};

#endif
