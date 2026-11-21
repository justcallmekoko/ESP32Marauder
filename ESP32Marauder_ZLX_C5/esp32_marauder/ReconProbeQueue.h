#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t RECON_PROBE_NAME_MAX = 24;
constexpr size_t RECON_PROBE_QUEUE_SIZE = 8;
constexpr size_t RECON_REPEAT_TRACKED = 24;

struct ReconProbeEvent {
  uint32_t elapsed_ms;
  uint8_t mac[6];
  int8_t rssi;
  uint8_t channel;
  uint8_t name_length;
  char name[RECON_PROBE_NAME_MAX];
};

class ReconProbeQueue {
 public:
  void reset() { head = tail = dropped_count = 0; }

  bool push(const ReconProbeEvent& event) {
    const uint8_t next = (head + 1) % RECON_PROBE_QUEUE_SIZE;
    if (next == tail) {
      dropped_count++;
      return false;
    }
    entries[head] = event;
    head = next;
    return true;
  }

  bool pop(ReconProbeEvent& event) {
    if (tail == head) return false;
    event = entries[tail];
    tail = (tail + 1) % RECON_PROBE_QUEUE_SIZE;
    return true;
  }

  uint16_t dropped() const { return dropped_count; }

 private:
  ReconProbeEvent entries[RECON_PROBE_QUEUE_SIZE] = {};
  uint8_t head = 0;
  uint8_t tail = 0;
  uint16_t dropped_count = 0;
};

struct ReconRepeatEvent {
  uint8_t mac[6];
  int8_t rssi;
  uint8_t channel;
  char type;
};

class ReconRepeatQueue {
 public:
  void reset() { head = tail = dropped_count = 0; }
  bool push(const ReconRepeatEvent& event) {
    const uint8_t next = (head + 1) % RECON_PROBE_QUEUE_SIZE;
    if (next == tail) { dropped_count++; return false; }
    entries[head] = event;
    head = next;
    return true;
  }
  bool pop(ReconRepeatEvent& event) {
    if (tail == head) return false;
    event = entries[tail];
    tail = (tail + 1) % RECON_PROBE_QUEUE_SIZE;
    return true;
  }
  uint16_t dropped() const { return dropped_count; }

 private:
  ReconRepeatEvent entries[RECON_PROBE_QUEUE_SIZE] = {};
  uint8_t head = 0;
  uint8_t tail = 0;
  uint16_t dropped_count = 0;
};

struct ReconDeauthEvent {
  uint8_t transmitter[6];
  uint8_t bssid[6];
  int8_t rssi;
  uint8_t channel;
  uint16_t reason;
};

class ReconDeauthQueue {
 public:
  void reset() { head = tail = dropped_count = 0; }
  bool push(const ReconDeauthEvent& event) {
    const uint8_t next = (head + 1) % RECON_PROBE_QUEUE_SIZE;
    if (next == tail) { dropped_count++; return false; }
    entries[head] = event;
    head = next;
    return true;
  }
  bool pop(ReconDeauthEvent& event) {
    if (tail == head) return false;
    event = entries[tail];
    tail = (tail + 1) % RECON_PROBE_QUEUE_SIZE;
    return true;
  }
  uint16_t dropped() const { return dropped_count; }

 private:
  ReconDeauthEvent entries[RECON_PROBE_QUEUE_SIZE] = {};
  uint8_t head = 0;
  uint8_t tail = 0;
  uint16_t dropped_count = 0;
};

class ReconRepeatGate {
 public:
  void reset() { memset(entries, 0, sizeof(entries)); }
  bool accept(const uint8_t mac[6], int8_t rssi, uint32_t now) {
    Entry* free_entry = nullptr;
    Entry* oldest = &entries[0];
    for (Entry& entry : entries) {
      if (!entry.used) { if (!free_entry) free_entry = &entry; continue; }
      if (entry.last_seen < oldest->last_seen) oldest = &entry;
      if (memcmp(entry.mac, mac, 6) != 0) continue;
      const bool changed = rssi != -128 && entry.rssi != -128 &&
                           abs(static_cast<int>(rssi) - entry.rssi) >= 12;
      const bool returned = now - entry.last_seen >= 30000;
      if (changed || returned) {
        entry.last_seen = now;
        entry.rssi = rssi;
        return true;
      }
      return false;
    }
    Entry* entry = free_entry ? free_entry : oldest;
    memcpy(entry->mac, mac, 6);
    entry->last_seen = now;
    entry->rssi = rssi;
    entry->used = true;
    return false;
  }

 private:
  struct Entry {
    uint8_t mac[6];
    uint32_t last_seen;
    int8_t rssi;
    bool used;
  };
  Entry entries[RECON_REPEAT_TRACKED] = {};
};
