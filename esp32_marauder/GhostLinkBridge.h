// GhostLinkBridge -- WarDriveGo-compatible wardrive BLE bridge for the
// Waveshare ESP32-C5-Touch-LCD-3.5 (MARAUDER_WS_C5_35).
//
// Speaks the published WardriveESP / GhostLink BLE wire protocol so the
// WarDriveGo iOS app can discover this board by its service UUID and stream
// Marauder's Wi-Fi scan results over BLE. The board identifies honestly as a
// Marauder (name Marauder-XXXX, board_name=Marauder_WS_C5_35 in WD:CAPS); it is
// not WardriveESP firmware. This is a clean-room implementation of the
// documented wire protocol only -- no third-party firmware source is copied
// here. See docs/ble_ghostlink_bridge.md for the contract and coexistence notes.
//
// The same BLE server also exposes the Biscuit Manager GATT surface (a second
// service + the standard Device Information Service), so the Biscuit app can
// drive the board as a pure wardriver too. That line protocol is handled by the
// standalone clean-room codec in BiscuitLink.h/.cpp (MIT, no dependencies). The
// two links share one Wi-Fi scan; each AP is fanned out in each link's format.
//
// Scoped entirely behind HAS_BLE_UART_BRIDGE, which is only defined for the
// Waveshare C5 3.5 target, so no other Marauder board is affected.

#pragma once
#ifndef GhostLinkBridge_h
#define GhostLinkBridge_h

#include "configs.h"

#ifdef HAS_BLE_UART_BRIDGE

#include <Arduino.h>

class GhostLinkBridge {
  public:
    void RunSetup();                   // init NimBLE peripheral + advertising
    void main(uint32_t currentTime);   // pump: recover, drain RX, stream APs

    bool isConnected();
    bool isStreaming();

  private:
    void startWardrive();
    void stopWardrive(const char *reason);
    void serviceWardrive(uint32_t now);
    void emitApBatch();
    void handleCommandLine(const String &line);
};

extern GhostLinkBridge ghostlink_obj;

#endif // HAS_BLE_UART_BRIDGE
#endif // GhostLinkBridge_h
