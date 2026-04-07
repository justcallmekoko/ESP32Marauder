#include "configs.h"

#ifdef HAS_OLED

#include "OledMenu.h"

// ----------------------------------------------------------------
// Menu tables
// ----------------------------------------------------------------
static const OledMenu::MenuItem WIFI_SNIFF[] = {
  { "Probe Sniff",       WIFI_SCAN_PROBE,              0, nullptr, 0 },
  { "Beacon Sniff",      WIFI_SCAN_AP,                 0, nullptr, 0 },
  { "Deauth Sniff",      WIFI_SCAN_DEAUTH,             0, nullptr, 0 },
  { "Station Sniff",     WIFI_SCAN_STATION,            0, nullptr, 0 },
  { "Packet Monitor",    WIFI_PACKET_MONITOR,          0, nullptr, 0 },
  { "Packet Rate",       WIFI_SCAN_PACKET_RATE,        0, nullptr, 0 },
  { "EAPOL/PMKID",       WIFI_SCAN_EAPOL,              0, nullptr, 0 },
  { "Detect Pwnagotchi", WIFI_SCAN_PWN,                0, nullptr, 0 },
  { "Detect Pineapple",  WIFI_SCAN_PINESCAN,           0, nullptr, 0 },
  { "Detect MultiSSID",  WIFI_SCAN_MULTISSID,          0, nullptr, 0 },
  { "Detect Espressif",  WIFI_SCAN_ESPRESSIF,          0, nullptr, 0 },
  { "Detect Follow",     WIFI_SCAN_DETECT_FOLLOW,      0, nullptr, 0 },
  { "SAE Commit Sniff",  WIFI_SCAN_SAE_COMMIT,         0, nullptr, 0 },
  { "Raw Capture",       WIFI_SCAN_RAW_CAPTURE,        0, nullptr, 0 },
  { "Signal Monitor",    WIFI_SCAN_SIG_STREN,          0, nullptr, 0 },
  { "Chan Analyzer",     WIFI_SCAN_CHAN_ANALYZER,      0, nullptr, 0 },
  { "Chan Activity",     WIFI_SCAN_CHAN_ACT,            0, nullptr, 0 },
  { "Scan APs",          WIFI_SCAN_TARGET_AP,          0, nullptr, 0 },
  { "Scan AP+Stations",  WIFI_SCAN_AP_STA,             0, nullptr, 0 },
  { "Wardrive",          WIFI_SCAN_WAR_DRIVE,          0, nullptr, 0 },
  { "Station Wardrive",  WIFI_SCAN_STATION_WAR_DRIVE,  0, nullptr, 0 },
};
static const uint8_t WIFI_SNIFF_N = sizeof(WIFI_SNIFF)/sizeof(WIFI_SNIFF[0]);

static const OledMenu::MenuItem WIFI_ATTACK[] = {
  { "Beacon Spam",       WIFI_ATTACK_BEACON_SPAM,        0, nullptr, 0 },
  { "Beacon List",       WIFI_ATTACK_BEACON_LIST,        0, nullptr, 0 },
  { "Funny Beacon",      WIFI_ATTACK_FUNNY_BEACON,       0, nullptr, 0 },
  { "Rick Roll",         WIFI_ATTACK_RICK_ROLL,          0, nullptr, 0 },
  { "Auth Flood",        WIFI_ATTACK_AUTH,               0, nullptr, 0 },
  { "Deauth",            WIFI_ATTACK_DEAUTH,             0, nullptr, 0 },
  { "Deauth Targeted",   WIFI_ATTACK_DEAUTH_TARGETED,    0, nullptr, 0 },
  { "AP Spam",           WIFI_ATTACK_AP_SPAM,            0, nullptr, 0 },
  { "Bad Msg",           WIFI_ATTACK_BAD_MSG,            0, nullptr, 0 },
  { "Bad Msg Targeted",  WIFI_ATTACK_BAD_MSG_TARGETED,   0, nullptr, 0 },
  { "Sleep",             WIFI_ATTACK_SLEEP,              0, nullptr, 0 },
  { "Sleep Targeted",    WIFI_ATTACK_SLEEP_TARGETED,     0, nullptr, 0 },
  { "SAE Commit",        WIFI_ATTACK_SAE_COMMIT,         0, nullptr, 0 },
  { "CSA",               WIFI_ATTACK_CSA,                0, nullptr, 0 },
  { "Quiet",             WIFI_ATTACK_QUIET,              0, nullptr, 0 },
  { "Evil Portal",       WIFI_SCAN_EVIL_PORTAL,          0, nullptr, 0 },
};
static const uint8_t WIFI_ATTACK_N = sizeof(WIFI_ATTACK)/sizeof(WIFI_ATTACK[0]);

static const OledMenu::MenuItem WIFI_MENU[] = {
  { "Sniffers", OLED_SUBMENU, 0, WIFI_SNIFF,  WIFI_SNIFF_N  },
  { "Attacks",  OLED_SUBMENU, 0, WIFI_ATTACK, WIFI_ATTACK_N },
};
static const uint8_t WIFI_MENU_N = sizeof(WIFI_MENU)/sizeof(WIFI_MENU[0]);

static const OledMenu::MenuItem BT_SNIFF[] = {
  { "Sniff BT",        BT_SCAN_ALL,            0, nullptr, 0 },
  { "Detect Skimmers", BT_SCAN_SKIMMERS,       0, nullptr, 0 },
  { "Scan Airtag",     BT_SCAN_AIRTAG,         0, nullptr, 0 },
  { "Monitor Airtag",  BT_SCAN_AIRTAG_MON,     0, nullptr, 0 },
  { "Scan Flipper",    BT_SCAN_FLIPPER,        0, nullptr, 0 },
  { "BT Analyzer",     BT_SCAN_ANALYZER,       0, nullptr, 0 },
  { "Flock",           BT_SCAN_FLOCK,          0, nullptr, 0 },
  { "Flock Wardrive",  BT_SCAN_FLOCK_WARDRIVE, 0, nullptr, 0 },
  { "RayBan",          BT_SCAN_RAYBAN,         0, nullptr, 0 },
  { "BT Wardrive",     BT_SCAN_WAR_DRIVE,      0, nullptr, 0 },
};
static const uint8_t BT_SNIFF_N = sizeof(BT_SNIFF)/sizeof(BT_SNIFF[0]);

static const OledMenu::MenuItem BT_SPAM[] = {
  { "Spam All",  BT_ATTACK_SPAM_ALL,        0, nullptr, 0 },
  { "Apple",     BT_ATTACK_SOUR_APPLE,      0, nullptr, 0 },
  { "SwiftPair", BT_ATTACK_SWIFTPAIR_SPAM,  0, nullptr, 0 },
  { "Samsung",   BT_ATTACK_SAMSUNG_SPAM,    0, nullptr, 0 },
  { "Google",    BT_ATTACK_GOOGLE_SPAM,     0, nullptr, 0 },
  { "Flipper",   BT_ATTACK_FLIPPER_SPAM,    0, nullptr, 0 },
};
static const uint8_t BT_SPAM_N = sizeof(BT_SPAM)/sizeof(BT_SPAM[0]);

static const OledMenu::MenuItem BT_MENU[] = {
  { "Sniffers", OLED_SUBMENU, 0, BT_SNIFF, BT_SNIFF_N },
  { "Spam",     OLED_SUBMENU, 0, BT_SPAM,  BT_SPAM_N  },
};
static const uint8_t BT_MENU_N = sizeof(BT_MENU)/sizeof(BT_MENU[0]);

static const OledMenu::MenuItem DEVICE_MENU[] = {
  { "Device Info", OLED_ACTION_INFO,   0, nullptr, 0 },
  { "Reboot",      OLED_ACTION_REBOOT, 0, nullptr, 0 },
};
static const uint8_t DEVICE_MENU_N = sizeof(DEVICE_MENU)/sizeof(DEVICE_MENU[0]);

static const OledMenu::MenuItem MAIN_MENU[] = {
  { "WiFi",      OLED_SUBMENU,       0, WIFI_MENU,   WIFI_MENU_N   },
  { "Bluetooth", OLED_SUBMENU,       0, BT_MENU,     BT_MENU_N     },
  { "Device",    OLED_SUBMENU,       0, DEVICE_MENU, DEVICE_MENU_N },
  { "Reboot",    OLED_ACTION_REBOOT, 0, nullptr,     0             },
};
static const uint8_t MAIN_MENU_N = sizeof(MAIN_MENU)/sizeof(MAIN_MENU[0]);

// ----------------------------------------------------------------

OledMenu::OledMenu(InputDevice* input)
  : input(input), depth(0), in_scan(false),
    needs_draw(true), last_input_ms(0) {}

void OledMenu::RunSetup() {
  pushMenu(MAIN_MENU, MAIN_MENU_N);
  // drawMenu() deferred to first main() so setup() can't overwrite it
}

// ---- navigation helpers ----------------------------------------

void OledMenu::pushMenu(const MenuItem* items, uint8_t count) {
  if (depth >= MAX_DEPTH) return;
  menu_stack[depth]  = items;
  count_stack[depth] = count;
  sel_stack[depth]   = 0;
  off_stack[depth]   = 0;
  depth++;
}

void OledMenu::popMenu() {
  if (depth > 1) depth--;
}

// ---- rendering -------------------------------------------------

void OledMenu::drawMenu() {
  const MenuItem* items = menu_stack[depth - 1];
  uint8_t  count        = count_stack[depth - 1];
  int8_t   sel          = sel_stack[depth - 1];
  int8_t   off          = off_stack[depth - 1];

  if (depth == 1) {
    display_obj.updateBanner("[ MARAUDER ]");
  } else {
    const MenuItem* parent = menu_stack[depth - 2];
    int8_t          pidx   = sel_stack[depth - 2];
    display_obj.updateBanner(String(parent[pidx].label));
  }

  display_obj.clearScreen();

  for (uint8_t row = 0; row < OLED_MAX_LINES; row++) {
    int8_t idx = off + row;
    if (idx >= count) break;
    String line = String(idx == sel ? ">" : " ") + String(items[idx].label);
    display_obj.display_buffer->add(line);
  }
  display_obj.displayBuffer();
  display_obj.drawBottomBar(depth > 1 ? "< Back" : "");
}

void OledMenu::drawScanStatus(const char* label) {
  display_obj.updateBanner(String(label));
  display_obj.clearScreen();
  display_obj.display_buffer->add("Scanning...");
  display_obj.displayBuffer();
  display_obj.drawBottomBar("< Stop");
}

// ---- main loop -------------------------------------------------

void OledMenu::main(uint32_t currentTime) {
  if (needs_draw) {
    needs_draw = false;
    drawMenu();
  }

  // Debounce: don't act on inputs faster than JOY_DEBOUNCE_MS
  bool gated = (currentTime - last_input_ms) < JOY_DEBOUNCE_MS;

  if (in_scan) {
    if (!gated && input->back()) {
      wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
      in_scan = false;
      last_input_ms = currentTime;
      drawMenu();
      return;
    }
    if (display_obj.display_buffer && display_obj.display_buffer->size() > 0) {
      display_obj.displayBuffer();
      display_obj.drawBottomBar("< Stop");
    }
    return;
  }

  // ---- idle: navigate menu ----
  bool changed = false;
  const MenuItem* items = menu_stack[depth - 1];
  uint8_t  count        = count_stack[depth - 1];
  int8_t&  sel          = sel_stack[depth - 1];
  int8_t&  off          = off_stack[depth - 1];

  if (!gated && input->up()) {
    if (sel > 0) {
      sel--;
      if (sel < off) off = sel;
    } else {
      sel = count - 1;
      off = (count > OLED_MAX_LINES) ? count - OLED_MAX_LINES : 0;
    }
    changed = true;
    last_input_ms = currentTime;
  } else if (!gated && input->down()) {
    if (sel < count - 1) {
      sel++;
      if (sel >= off + OLED_MAX_LINES) off = sel - OLED_MAX_LINES + 1;
    } else {
      sel = 0;
      off = 0;
    }
    changed = true;
    last_input_ms = currentTime;
  }

  if (changed) { drawMenu(); return; }

  if (!gated && input->sel()) {
    last_input_ms = currentTime;
    const MenuItem& item = items[sel];
    if (item.action == OLED_SUBMENU) {
      pushMenu(item.sub_items, item.sub_count);
      drawMenu();
    } else if (item.action == OLED_ACTION_REBOOT) {
      display_obj.updateBanner("Rebooting...");
      display_obj.clearScreen();
      display_obj.displayBuffer();
      delay(500);
      ESP.restart();
    } else if (item.action == OLED_ACTION_INFO) {
      display_obj.updateBanner("Device Info");
      display_obj.clearScreen();
      display_obj.display_buffer->add("Marauder");
      display_obj.display_buffer->add(display_obj.version_number);
      display_obj.display_buffer->add("ESP32 OLED");
      display_obj.displayBuffer();
      display_obj.drawBottomBar("< Back");
      input->waitBackRelease();
      drawMenu();
    } else {
      in_scan = true;
      drawScanStatus(item.label);
      wifi_scan_obj.StartScan(item.action, item.color);
    }
    return;
  }

  if (!gated && input->back()) {
    last_input_ms = currentTime;
    if (depth > 1) {
      popMenu();
      drawMenu();
    }
  }
}

#endif // HAS_OLED
