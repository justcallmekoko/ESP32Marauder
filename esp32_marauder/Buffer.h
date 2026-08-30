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
    void saveFs();
    void saveSerial();

    uint8_t* bufA;
    uint8_t* bufB;

    uint32_t bufSizeA = 0;
    uint32_t bufSizeB = 0;

    bool writing = false; // acceppting writes to buffer
    bool useA = true; // writing to bufA or bufB
    bool saving = false; // currently saving onto the SD card

    // Host-streaming framing (see saveSerial): each flush is emitted as one
    // self-describing, length-prefixed binary frame so a host can carve the pcap
    // out of the mixed text/binary serial stream without scanning the payload.
    uint8_t stream_type = STREAM_PCAP; // what the current file holds
    uint32_t seq_no = 0;               // per-open frame counter (gap detection on the host)
    volatile uint32_t dropped = 0;     // packets dropped because the ring was full

    String fileName = "/0.pcap";
    File file;
    fs::FS* fs;
    bool serial;

  public:
    // Binary-frame stream kinds, mirrored by the host demultiplexer.
    enum : uint8_t { STREAM_PCAP = 0, STREAM_LOG = 1, STREAM_GPX = 2 };

    // Number of packets dropped since the last call, then resets the counter.
    // The main loop reports these as an {"t":"drop","n":N} line so loss is
    // explicit and byte-accurate instead of silent.
    uint32_t takeDropped();
};

#endif
