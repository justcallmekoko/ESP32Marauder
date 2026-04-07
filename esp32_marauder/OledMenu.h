#pragma once

#ifndef OledMenu_h
#define OledMenu_h

#include "configs.h"

#ifdef HAS_OLED

#include "Display.h"
#include "WiFiScan.h"

extern Display  display_obj;
extern WiFiScan wifi_scan_obj;

// Special action values (above valid scan mode range)
#define OLED_SUBMENU       250
#define OLED_ACTION_REBOOT 251
#define OLED_ACTION_INFO   252

class OledMenu {
  public:
    struct MenuItem {
      const char*        label;
      uint8_t            action;      // scan mode or OLED_* special
      uint16_t           color;       // passed to StartScan (ignored on mono OLED)
      const MenuItem*    sub_items;   // non-null when action==OLED_SUBMENU
      uint8_t            sub_count;
    };

    OledMenu();
    void RunSetup();
    void main(uint32_t currentTime);

  private:
    static const uint8_t MAX_DEPTH = 4;

    const MenuItem* menu_stack[MAX_DEPTH];
    uint8_t         count_stack[MAX_DEPTH];
    int8_t          sel_stack[MAX_DEPTH];
    int8_t          off_stack[MAX_DEPTH];
    uint8_t         depth;

    bool     in_scan;
    bool     needs_draw;  // deferred initial draw

    // Joystick state for edge detection
    bool joy_up_last;
    bool joy_down_last;
    bool joy_left_last;
    bool joy_sw_last;
    uint32_t joy_last_ms;

    bool joyUp();
    bool joyDown();
    bool joyLeft();
    bool joySel();

    void pushMenu(const MenuItem* items, uint8_t count);
    void popMenu();
    void drawMenu();
    void drawScanStatus(const char* label);
};

#endif // HAS_OLED
#endif // OledMenu_h
