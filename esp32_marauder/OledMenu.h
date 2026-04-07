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

#define OLED_SUBMENU       250
#define OLED_ACTION_REBOOT 251
#define OLED_ACTION_INFO   252

class OledMenu {
  public:
    struct MenuItem {
      const char*        label;
      uint8_t            action;
      uint16_t           color;
      const MenuItem*    sub_items;
      uint8_t            sub_count;
    };

    OledMenu(InputDevice* input);
    void RunSetup();
    void main(uint32_t currentTime);

  private:
    static const uint8_t MAX_DEPTH = 4;

    InputDevice*    input;

    const MenuItem* menu_stack[MAX_DEPTH];
    uint8_t         count_stack[MAX_DEPTH];
    int8_t          sel_stack[MAX_DEPTH];
    int8_t          off_stack[MAX_DEPTH];
    uint8_t         depth;

    bool            in_scan;
    bool            needs_draw;
    uint32_t        last_input_ms;

    void pushMenu(const MenuItem* items, uint8_t count);
    void popMenu();
    void drawMenu();
    void drawScanStatus(const char* label);
};

#endif // HAS_OLED
#endif // OledMenu_h
