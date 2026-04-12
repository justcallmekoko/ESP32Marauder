#pragma once

#ifndef OledMenu_h
#define OledMenu_h

#include "configs.h"

#ifdef HAS_OLED

#include "Display.h"
#include "WiFiScan.h"
#include "InputDevice.h"

extern Display  display_obj;
extern WiFiScan wifi_scan_obj;

// ---- Special action codes (not real scan modes) ----
#define OLED_SUBMENU          250
#define OLED_ACTION_REBOOT    251
#define OLED_ACTION_INFO      252
#define OLED_ACTION_ADD_SSID  253
#define OLED_ACTION_JOIN_WIFI 254
#define OLED_ACTION_MAC_AP    255
#define OLED_ACTION_MAC_STA   256


class OledMenu {
  public:

    struct MenuItem {
      const char*      label;
      uint16_t         action;
      uint16_t         color;
      const MenuItem*  sub_items;
      uint8_t          sub_count;
    };

    // UI states — each maps to exactly one screen and one button map.
    enum class UiState : uint8_t {
      MENU,          // navigating the menu tree
      SCANNING,      // sniffer running — results stream in
      ATTACKING,     // attack running — packet rate shown in-place
      SCAN_STOPPED,  // sniffer stopped, showing AP count
      SELECT_APS,    // AP checkbox list
      ATTACK_PICKER, // pick which attack to run on selected APs
    };

    OledMenu(InputDevice* input);
    void RunSetup();
    void main(uint32_t currentTime);

  private:

    // ---- Menu tree ----
    static const uint8_t kMaxDepth   = 4;
    static const uint8_t kMaxHistory = 8;

    const MenuItem* menu_stack[kMaxDepth];
    uint8_t         count_stack[kMaxDepth];
    int8_t          sel_stack[kMaxDepth];
    int8_t          off_stack[kMaxDepth];
    uint8_t         depth;

    // ---- State machine ----
    InputDevice* input;
    UiState      state;
    UiState      history[kMaxHistory];
    uint8_t      history_depth;

    // ---- Per-state context ----
    uint16_t  pending_action;  // attack to launch after SELECT_APS (MENU path)
    int8_t    ap_sel_cursor;
    int8_t    ap_sel_offset;
    int8_t    picker_cursor;
    int8_t    picker_offset;

    // ---- Timing ----
    uint32_t  last_scan_ms;   // millis() when last sniffer completed (AP TTL)
    uint32_t  last_input_ms;  // millis() of last accepted input (debounce)

    // ---- State machine core ----
    void transition(UiState next);  // push history, set state, render
    void goBack();                  // pop history, render
    void render();                  // dispatch to renderXxx()

    // ---- Renderers (one per state) ----
    void renderMenu();
    void renderScanning();
    void renderAttacking();
    void renderScanStopped();
    void renderSelectAPs();
    void renderAttackPicker();

    // ---- Input handler ----
    void handleInput(uint32_t currentTime);

    // ---- Menu helpers ----
    void pushMenu(const MenuItem* items, uint8_t count);
    void popMenu();

    // ---- Action classification ----
    static bool actionIsSniffer(uint16_t action);
    static bool actionNeedsAPSelection(uint16_t action);
};

#endif  // HAS_OLED
#endif  // OledMenu_h
