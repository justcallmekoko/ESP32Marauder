#include "configs.h"

#ifdef HAS_OLED

#include "OledMenu.h"
#include "T9Keyboard.h"
#include "CommandLine.h"

// Uncomment to print renderMenu() timing + heap metrics to Serial.
// Enable before a change, capture baseline, re-enable after to compare.
#define OLED_PERF_LOG


// ================================================================
// Banner string constants
// ================================================================

static const char kBannerResults[]    = "Results";
static const char kBannerTargets[]    = "Targets";
static const char kBannerAttackType[] = "Attack Type";
static const char kBannerAttacking[]  = "Attacking";
static const char kBannerNoTargets[]  = "No targets!";

// ================================================================
// Menu tables
// ================================================================

// Field order: label, short_label, action, color, sub_items, sub_count, icon_id

static const OledMenu::MenuItem WIFI_SNIFF[] = {
  { "Probe Sniff",       "Probe",        WIFI_SCAN_PROBE,             0, nullptr, 0, ICON_NONE },
  { "Beacon Sniff",      "Beacon",       WIFI_SCAN_AP,                0, nullptr, 0, ICON_NONE },
  { "Deauth Sniff",      "Deauth",       WIFI_SCAN_DEAUTH,            0, nullptr, 0, ICON_NONE },
  { "Station Sniff",     "Station",      WIFI_SCAN_STATION,           0, nullptr, 0, ICON_NONE },
  { "Packet Monitor",    "Pkt Monitor",  WIFI_PACKET_MONITOR,         0, nullptr, 0, ICON_NONE },
  { "Packet Rate",       "Pkt Rate",     WIFI_SCAN_PACKET_RATE,       0, nullptr, 0, ICON_NONE },
  { "EAPOL/PMKID",       "EAPOL",        WIFI_SCAN_EAPOL,             0, nullptr, 0, ICON_NONE },
  { "Detect Pwnagotchi", "Pwnagotchi",   WIFI_SCAN_PWN,               0, nullptr, 0, ICON_NONE },
  { "Detect Pineapple",  "Pineapple",    WIFI_SCAN_PINESCAN,          0, nullptr, 0, ICON_NONE },
  { "Detect MultiSSID",  "MultiSSID",    WIFI_SCAN_MULTISSID,         0, nullptr, 0, ICON_NONE },
  { "Detect Espressif",  "Espressif",    WIFI_SCAN_ESPRESSIF,         0, nullptr, 0, ICON_NONE },
  { "Detect Follow",     "Follow",       WIFI_SCAN_DETECT_FOLLOW,     0, nullptr, 0, ICON_NONE },
  { "SAE Commit Sniff",  "SAE Commit",   WIFI_SCAN_SAE_COMMIT,        0, nullptr, 0, ICON_NONE },
  { "Raw Capture",       "Raw Capture",  WIFI_SCAN_RAW_CAPTURE,       0, nullptr, 0, ICON_NONE },
  { "Signal Monitor",    "Signal Mon",   WIFI_SCAN_SIG_STREN,         0, nullptr, 0, ICON_NONE },
  { "Chan Analyzer",     "Chan Analyz",  WIFI_SCAN_CHAN_ANALYZER,     0, nullptr, 0, ICON_NONE },
  { "Chan Activity",     "Chan Act",     WIFI_SCAN_CHAN_ACT,           0, nullptr, 0, ICON_NONE },
  { "Scan APs",          "APs",          WIFI_SCAN_TARGET_AP,         0, nullptr, 0, ICON_NONE },
  { "Scan AP+Stations",  "AP+Stations",  WIFI_SCAN_AP_STA,            0, nullptr, 0, ICON_NONE },
  { "Wardrive",          "Wardrive",     WIFI_SCAN_WAR_DRIVE,         0, nullptr, 0, ICON_NONE },
  { "Station Wardrive",  "Sta Wardrive", WIFI_SCAN_STATION_WAR_DRIVE, 0, nullptr, 0, ICON_NONE },
};
static const uint8_t WIFI_SNIFF_N = sizeof(WIFI_SNIFF) / sizeof(WIFI_SNIFF[0]);

static const OledMenu::MenuItem WIFI_ATTACK[] = {
  { "Beacon Spam",      "Beacon Spam",  WIFI_ATTACK_BEACON_SPAM,       0, nullptr, 0, ICON_NONE },
  { "Beacon List",      "Beacon List",  WIFI_ATTACK_BEACON_LIST,       0, nullptr, 0, ICON_NONE },
  { "Funny Beacon",     "Funny Beacon", WIFI_ATTACK_FUNNY_BEACON,      0, nullptr, 0, ICON_NONE },
  { "Rick Roll",        "Rick Roll",    WIFI_ATTACK_RICK_ROLL,         0, nullptr, 0, ICON_NONE },
  { "Auth Flood",       "Auth Flood",   WIFI_ATTACK_AUTH,              0, nullptr, 0, ICON_NONE },
  { "Deauth",           "Deauth",       WIFI_ATTACK_DEAUTH,            0, nullptr, 0, ICON_NONE },
  { "Deauth Targeted",  "Deauth Tgt",   WIFI_ATTACK_DEAUTH_TARGETED,   0, nullptr, 0, ICON_NONE },
  { "AP Spam",          "AP Spam",      WIFI_ATTACK_AP_SPAM,           0, nullptr, 0, ICON_NONE },
  { "Bad Msg",          "Bad Msg",      WIFI_ATTACK_BAD_MSG,           0, nullptr, 0, ICON_NONE },
  { "Bad Msg Targeted", "Bad Msg Tgt",  WIFI_ATTACK_BAD_MSG_TARGETED,  0, nullptr, 0, ICON_NONE },
  { "Sleep",            "Sleep",        WIFI_ATTACK_SLEEP,             0, nullptr, 0, ICON_NONE },
  { "Sleep Targeted",   "Sleep Tgt",    WIFI_ATTACK_SLEEP_TARGETED,    0, nullptr, 0, ICON_NONE },
  { "SAE Commit",       "SAE Commit",   WIFI_ATTACK_SAE_COMMIT,        0, nullptr, 0, ICON_NONE },
  { "CSA",              "CSA",          WIFI_ATTACK_CSA,               0, nullptr, 0, ICON_NONE },
  { "Quiet",            "Quiet",        WIFI_ATTACK_QUIET,             0, nullptr, 0, ICON_NONE },
  { "Evil Portal",      "Evil Portal",  WIFI_SCAN_EVIL_PORTAL,         0, nullptr, 0, ICON_NONE },
};
static const uint8_t WIFI_ATTACK_N = sizeof(WIFI_ATTACK) / sizeof(WIFI_ATTACK[0]);

static const OledMenu::MenuItem WIFI_MENU[] = {
  { "Sniffers",  nullptr, OLED_SUBMENU,          0, WIFI_SNIFF,  WIFI_SNIFF_N,  ICON_MAGNIFIER },
  { "Attacks",   nullptr, OLED_SUBMENU,          0, WIFI_ATTACK, WIFI_ATTACK_N, ICON_BOLT      },
  { "Add SSID",  nullptr, OLED_ACTION_ADD_SSID,  0, nullptr,     0,             ICON_PENCIL    },
  { "Join WiFi", nullptr, OLED_ACTION_JOIN_WIFI, 0, nullptr,     0,             ICON_WIFI_JOIN },
};
static const uint8_t WIFI_MENU_N = sizeof(WIFI_MENU) / sizeof(WIFI_MENU[0]);

static const OledMenu::MenuItem BT_SNIFF[] = {
  { "Sniff BT",        "BT",         BT_SCAN_ALL,            0, nullptr, 0, ICON_NONE },
  { "Detect Skimmers", "Skimmers",   BT_SCAN_SKIMMERS,       0, nullptr, 0, ICON_NONE },
  { "Scan Airtag",     "Airtag",     BT_SCAN_AIRTAG,         0, nullptr, 0, ICON_NONE },
  { "Monitor Airtag",  "Airtag Mon", BT_SCAN_AIRTAG_MON,     0, nullptr, 0, ICON_NONE },
  { "Scan Flipper",    "Flipper",    BT_SCAN_FLIPPER,         0, nullptr, 0, ICON_NONE },
  { "BT Analyzer",     "Analyzer",   BT_SCAN_ANALYZER,        0, nullptr, 0, ICON_NONE },
  { "Flock",           "Flock",      BT_SCAN_FLOCK,           0, nullptr, 0, ICON_NONE },
  { "Flock Wardrive",  "Flock WD",   BT_SCAN_FLOCK_WARDRIVE,  0, nullptr, 0, ICON_NONE },
  { "RayBan",          "RayBan",     BT_SCAN_RAYBAN,          0, nullptr, 0, ICON_NONE },
  { "BT Wardrive",     "BT WD",      BT_SCAN_WAR_DRIVE,       0, nullptr, 0, ICON_NONE },
};
static const uint8_t BT_SNIFF_N = sizeof(BT_SNIFF) / sizeof(BT_SNIFF[0]);

static const OledMenu::MenuItem BT_SPAM[] = {
  { "Spam All",  "Spam All",  BT_ATTACK_SPAM_ALL,        0, nullptr, 0, ICON_NONE },
  { "Apple",     "Apple",     BT_ATTACK_SOUR_APPLE,      0, nullptr, 0, ICON_NONE },
  { "SwiftPair", "SwiftPair", BT_ATTACK_SWIFTPAIR_SPAM,  0, nullptr, 0, ICON_NONE },
  { "Samsung",   "Samsung",   BT_ATTACK_SAMSUNG_SPAM,    0, nullptr, 0, ICON_NONE },
  { "Google",    "Google",    BT_ATTACK_GOOGLE_SPAM,     0, nullptr, 0, ICON_NONE },
  { "Flipper",   "Flipper",   BT_ATTACK_FLIPPER_SPAM,    0, nullptr, 0, ICON_NONE },
};
static const uint8_t BT_SPAM_N = sizeof(BT_SPAM) / sizeof(BT_SPAM[0]);

static const OledMenu::MenuItem BT_MENU[] = {
  { "Sniffers", nullptr, OLED_SUBMENU, 0, BT_SNIFF, BT_SNIFF_N, ICON_MAGNIFIER },
  { "Spam",     nullptr, OLED_SUBMENU, 0, BT_SPAM,  BT_SPAM_N,  ICON_BOLT      },
};
static const uint8_t BT_MENU_N = sizeof(BT_MENU) / sizeof(BT_MENU[0]);

static const OledMenu::MenuItem MAC_MENU[] = {
  { "Random AP MAC",  nullptr, OLED_ACTION_MAC_AP,  0, nullptr, 0, ICON_NONE },
  { "Random STA MAC", nullptr, OLED_ACTION_MAC_STA, 0, nullptr, 0, ICON_NONE },
};
static const uint8_t MAC_MENU_N = sizeof(MAC_MENU) / sizeof(MAC_MENU[0]);

static const OledMenu::MenuItem DEVICE_MENU[] = {
  { "Device Info", nullptr, OLED_ACTION_INFO,   0, nullptr,  0,          ICON_NONE   },
  { "MAC Address", nullptr, OLED_SUBMENU,       0, MAC_MENU, MAC_MENU_N, ICON_NONE   },
  { "Reboot",      nullptr, OLED_ACTION_REBOOT, 0, nullptr,  0,          ICON_REBOOT },
};
static const uint8_t DEVICE_MENU_N = sizeof(DEVICE_MENU) / sizeof(DEVICE_MENU[0]);

static const OledMenu::MenuItem MAIN_MENU[] = {
  { "WiFi",      nullptr, OLED_SUBMENU,       0, WIFI_MENU,   WIFI_MENU_N,   ICON_WIFI    },
  { "Bluetooth", nullptr, OLED_SUBMENU,       0, BT_MENU,     BT_MENU_N,     ICON_BT      },
  { "Device",    nullptr, OLED_SUBMENU,       0, DEVICE_MENU, DEVICE_MENU_N, ICON_GEAR    },
  { "Reboot",    nullptr, OLED_ACTION_REBOOT, 0, nullptr,     0,             ICON_REBOOT  },
};
static const uint8_t MAIN_MENU_N = sizeof(MAIN_MENU) / sizeof(MAIN_MENU[0]);

// Attacks available after AP selection (SCAN_STOPPED → SELECT_APS → ATTACK_PICKER).
// File-scope so renderAttackPicker() can reference it.
static const OledMenu::MenuItem POST_SCAN_ATTACKS[] = {
  { "Deauth",          "Deauth",      WIFI_ATTACK_DEAUTH,           0, nullptr, 0, ICON_NONE },
  { "Deauth Targeted", "Deauth Tgt",  WIFI_ATTACK_DEAUTH_TARGETED,  0, nullptr, 0, ICON_NONE },
  { "Bad Msg",         "Bad Msg",     WIFI_ATTACK_BAD_MSG,          0, nullptr, 0, ICON_NONE },
  { "Bad Msg Target",  "Bad Msg Tgt", WIFI_ATTACK_BAD_MSG_TARGETED, 0, nullptr, 0, ICON_NONE },
  { "Sleep",           "Sleep",       WIFI_ATTACK_SLEEP,            0, nullptr, 0, ICON_NONE },
  { "Sleep Targeted",  "Sleep Tgt",   WIFI_ATTACK_SLEEP_TARGETED,   0, nullptr, 0, ICON_NONE },
  { "CSA",             "CSA",         WIFI_ATTACK_CSA,              0, nullptr, 0, ICON_NONE },
  { "Quiet",           "Quiet",       WIFI_ATTACK_QUIET,            0, nullptr, 0, ICON_NONE },
};
static const uint8_t POST_SCAN_ATTACKS_N =
    sizeof(POST_SCAN_ATTACKS) / sizeof(POST_SCAN_ATTACKS[0]);


// ================================================================
// Constructor / setup
// ================================================================

OledMenu::OledMenu(InputDevice* input)
  : input(input),
    depth(0),
    state(UiState::MENU),
    history_depth(0),
    pending_action(0),
    pending_label(nullptr),
    ap_sel_cursor(0),
    ap_sel_offset(0),
    picker_cursor(0),
    picker_offset(0),
    last_scan_ms(0),
    last_input_ms(0),
    active_icon_id(ICON_NONE),
    icon_frame(0),
    last_icon_ms(0)
{}

void OledMenu::RunSetup() {
  pushMenu(MAIN_MENU, MAIN_MENU_N);
  render();
}


// ================================================================
// State machine core
// ================================================================

void OledMenu::transition(UiState next) {
  // SCANNING → SCAN_STOPPED is a replacement, not a push.
  // We want SCAN_STOPPED's BACK to go to MENU, not back to SCANNING.
  if (state != UiState::SCANNING && history_depth < kMaxHistory) {
    history[history_depth++] = state;
  }

  state = next;
  render();
}

void OledMenu::goBack() {
  if (history_depth == 0) return;
  state = history[--history_depth];
  render();
}

void OledMenu::render() {
  switch (state) {
    case UiState::MENU:          renderMenu();         break;
    case UiState::SCANNING:      renderScanning();     break;
    case UiState::ATTACKING:     renderAttacking();    break;
    case UiState::SCAN_STOPPED:  renderScanStopped();  break;
    case UiState::SELECT_APS:    renderSelectAPs();    break;
    case UiState::ATTACK_PICKER: renderAttackPicker(); break;
  }
}


// ================================================================
// Renderers — one per state, display only, no logic
// ================================================================

void OledMenu::renderMenu() {
#ifdef OLED_PERF_LOG
  uint32_t _t0        = micros();
  uint32_t _heap_pre  = ESP.getFreeHeap();
#endif

  const MenuItem* items = menu_stack[depth - 1];
  uint8_t  count        = count_stack[depth - 1];
  int8_t   sel          = sel_stack[depth - 1];
  int8_t   off          = off_stack[depth - 1];

  // Banner: root shows device name, submenus show their parent label
  if (depth == 1) {
    display_obj.updateBanner("MARAUDER");
  } else {
    const MenuItem* parent = menu_stack[depth - 2];
    int8_t          pidx   = sel_stack[depth - 2];
    display_obj.updateBanner(String(parent[pidx].label));
  }

  display_obj.clearScreen();
  display_obj.oled.setTextSize(1);

  for (uint8_t row = 0; row < OLED_MAX_LINES; row++) {
    int8_t idx = off + row;
    if (idx >= count) break;

    int16_t  y         = OLED_BANNER_H + 1 + row * OLED_LINE_HEIGHT;
    bool     selected  = (idx == sel);
    bool     has_icon  = (items[idx].icon_id != ICON_NONE);
    int16_t  text_x    = has_icon ? 12 : 2;
    uint8_t  max_chars = (128 - text_x) / 6;

    if (selected) {
      display_obj.oled.fillRect(0, y, 128, OLED_LINE_HEIGHT, OLED_WHITE);
      display_obj.oled.setTextColor(OLED_BLACK);
    } else {
      display_obj.oled.setTextColor(OLED_WHITE);
    }

    if (has_icon) {
      uint16_t icon_color = selected ? (uint16_t)OLED_BLACK : (uint16_t)OLED_WHITE;
      display_obj.drawIcon(items[idx].icon_id, 2, y + 1, icon_color);
    }

    display_obj.oled.setCursor(text_x, y + 1);
    String lbl = String(items[idx].label);
    if (lbl.length() > max_chars) lbl = lbl.substring(0, max_chars);
    display_obj.oled.print(lbl);
  }

  display_obj.oled.setTextColor(OLED_WHITE);  // reset before bottom bar
  display_obj.drawBottomBar("", depth > 1 ? "Back" : "");

#ifdef OLED_PERF_LOG
  uint32_t _dt = micros() - _t0;
  Serial.printf("[MENU] render=%uus  heap_before=%u  heap_after=%u  delta=%d  min_heap=%u\n",
    _dt, _heap_pre, ESP.getFreeHeap(),
    (int32_t)(ESP.getFreeHeap() - _heap_pre),
    ESP.getMinFreeHeap());
#endif
}

void OledMenu::renderScanning() {
  // Drain any stale buffer from a previous scan before showing fresh state
  while (display_obj.display_buffer->size() > 0) {
    display_obj.display_buffer->shift();
  }

  display_obj.clearScreen();
  display_obj.display_buffer->add("Scanning...");
  display_obj.displayBuffer();
  display_obj.drawBottomBar("", "Stop");
}

void OledMenu::renderAttacking() {
  // Drain stale buffer — displayTransmitRate() will overwrite via drawStatusLine()
  while (display_obj.display_buffer->size() > 0) {
    display_obj.display_buffer->shift();
  }

  display_obj.clearScreen();
  display_obj.drawStatusLine("Starting...");
  display_obj.drawBottomBar("", "Stop");
}

void OledMenu::renderScanStopped() {
  int n = access_points->size();

  display_obj.updateBanner(kBannerResults);
  display_obj.clearScreen();
  display_obj.display_buffer->add(String(n) + " APs found.");

  if (n > 0) {
    display_obj.display_buffer->add("SEL: Select APs");
  }

  display_obj.displayBuffer();
  display_obj.drawBottomBar(n > 0 ? "Select" : "", "Back");
}

void OledMenu::renderSelectAPs() {
  int ap_count = access_points->size();

  // Count selected to determine right button label
  int selected_count = 0;
  for (int i = 0; i < ap_count; i++) {
    if (access_points->get(i).selected) selected_count++;
  }

  display_obj.updateBanner(kBannerTargets);
  display_obj.clearScreen();
  display_obj.oled.setTextSize(1);

  for (uint8_t row = 0; row < OLED_MAX_LINES; row++) {
    int8_t idx = ap_sel_offset + row;
    if (idx >= ap_count) break;

    int16_t     y        = OLED_BANNER_H + 1 + row * OLED_LINE_HEIGHT;
    bool        selected = (idx == ap_sel_cursor);
    AccessPoint ap       = access_points->get(idx);

    if (selected) {
      display_obj.oled.fillRect(0, y, 128, OLED_LINE_HEIGHT, OLED_WHITE);
      display_obj.oled.setTextColor(OLED_BLACK);
    } else {
      display_obj.oled.setTextColor(OLED_WHITE);
    }

    display_obj.oled.setCursor(2, y + 1);
    String ssid = ap.essid;
    if (ssid.length() > 17) ssid = ssid.substring(0, 17);
    display_obj.oled.print((ap.selected ? "[*] " : "[ ] ") + ssid);
  }

  display_obj.oled.setTextColor(OLED_WHITE);
  display_obj.drawBottomBar(selected_count > 0 ? "Go" : "Select", "Back");
}

void OledMenu::renderAttackPicker() {
  display_obj.updateBanner(kBannerAttackType);
  display_obj.clearScreen();
  display_obj.oled.setTextSize(1);

  for (uint8_t row = 0; row < OLED_MAX_LINES; row++) {
    int8_t idx = picker_offset + row;
    if (idx >= (int8_t)POST_SCAN_ATTACKS_N) break;

    int16_t y        = OLED_BANNER_H + 1 + row * OLED_LINE_HEIGHT;
    bool    selected = (idx == picker_cursor);

    if (selected) {
      display_obj.oled.fillRect(0, y, 128, OLED_LINE_HEIGHT, OLED_WHITE);
      display_obj.oled.setTextColor(OLED_BLACK);
    } else {
      display_obj.oled.setTextColor(OLED_WHITE);
    }

    display_obj.oled.setCursor(2, y + 1);
    String lbl = String(POST_SCAN_ATTACKS[idx].label);
    if (lbl.length() > 21) lbl = lbl.substring(0, 21);
    display_obj.oled.print(lbl);
  }

  display_obj.oled.setTextColor(OLED_WHITE);
  display_obj.drawBottomBar("", "Back");
}


// ================================================================
// Input handler — reads buttons, fires transitions
// ================================================================

void OledMenu::handleInput(uint32_t currentTime) {
  bool gated = (currentTime - last_input_ms) < (uint32_t)JOY_DEBOUNCE_MS;

  switch (state) {

    // ---- MENU ----
    case UiState::MENU: {
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
        last_input_ms = currentTime;
        renderMenu();

      } else if (!gated && input->down()) {
        if (sel < count - 1) {
          sel++;
          if (sel >= off + OLED_MAX_LINES) off = sel - OLED_MAX_LINES + 1;
        } else {
          sel = 0;
          off = 0;
        }
        last_input_ms = currentTime;
        renderMenu();

      } else if (!gated && input->sel()) {
        last_input_ms = currentTime;
        const MenuItem& item = items[sel];

        if (item.action == OLED_SUBMENU) {
          pushMenu(item.sub_items, item.sub_count);
          renderMenu();

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
          display_obj.drawBottomBar("", "Back");
          input->waitBackRelease();
          renderMenu();

        } else if (item.action == OLED_ACTION_MAC_AP) {
          wifi_scan_obj.RunGenerateRandomMac(true);
          display_obj.updateBanner("AP MAC Set");
          display_obj.clearScreen();
          display_obj.display_buffer->add("Random AP MAC");
          display_obj.display_buffer->add("applied.");
          display_obj.displayBuffer();
          display_obj.drawBottomBar("", "Back");
          input->waitBackRelease();
          renderMenu();

        } else if (item.action == OLED_ACTION_MAC_STA) {
          wifi_scan_obj.RunGenerateRandomMac(false);
          display_obj.updateBanner("STA MAC Set");
          display_obj.clearScreen();
          display_obj.display_buffer->add("Random STA MAC");
          display_obj.display_buffer->add("applied.");
          display_obj.displayBuffer();
          display_obj.drawBottomBar("", "Back");
          input->waitBackRelease();
          renderMenu();

        } else if (item.action == OLED_ACTION_ADD_SSID) {
          String ssid = display_obj.getInput("SSID:", input);
          ssid.trim();
          if (ssid.length() > 0) wifi_scan_obj.addSSID(ssid);
          renderMenu();

        } else if (item.action == OLED_ACTION_JOIN_WIFI) {
          String ssid = display_obj.getInput("SSID:", input);
          ssid.trim();
          if (ssid.length() > 0) {
            String pass = display_obj.getInput("Password:", input);
            pass.trim();
            wifi_scan_obj.joinWiFi(ssid, pass);
          }
          renderMenu();

        } else if (actionIsSniffer(item.action)) {
          // Apply AP TTL: clear stale APs older than 60 seconds
          if (millis() - last_scan_ms > 60000UL) {
            while (access_points->size() > 0) access_points->shift();
          }

          {
            const char* lbl = item.short_label ? item.short_label : item.label;
            display_obj.updateBannerWithIcons(String(lbl), ICON_MAGNIFIER);
            active_icon_id = actionIsBT(item.action) ? ICON_SCAN_BT : ICON_SCAN_WIFI;
            icon_frame    = 0;
            last_icon_ms  = 0;  // force immediate first tick in main()
          }
          wifi_scan_obj.StartScan(item.action, item.color);
          transition(UiState::SCANNING);

        } else if (actionNeedsAPSelection(item.action)) {
          if (access_points->size() == 0) {
            // No APs available — show warning and stay in MENU
            display_obj.updateBanner(kBannerNoTargets);
            display_obj.clearScreen();
            display_obj.display_buffer->add("Run a sniffer");
            display_obj.display_buffer->add("first to find");
            display_obj.display_buffer->add("target APs.");
            display_obj.displayBuffer();
            display_obj.drawBottomBar("", "Back");
            delay(1500);
            renderMenu();
          } else {
            // Store which attack to launch after AP selection
            pending_action = item.action;
            pending_label  = item.short_label ? item.short_label : item.label;
            ap_sel_cursor  = 0;
            ap_sel_offset  = 0;
            transition(UiState::SELECT_APS);
          }

        } else {
          // Regular attack — no AP selection needed
          {
            const char* lbl = item.short_label ? item.short_label : item.label;
            display_obj.updateBannerWithIcons(String(lbl), ICON_BOLT);
            active_icon_id = ICON_BOLT_ANIM;
            icon_frame    = 0;
            last_icon_ms  = 0;
          }
          wifi_scan_obj.StartScan(item.action, item.color);
          transition(UiState::ATTACKING);
        }

      } else if (!gated && input->back()) {
        last_input_ms = currentTime;
        if (depth > 1) {
          popMenu();
          renderMenu();
        }
        // At root: back is a noop
      }
      break;
    }

    // ---- SCANNING ----
    case UiState::SCANNING: {
      if (!gated && input->back()) {
        last_input_ms = currentTime;
        last_scan_ms  = millis();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

        // Replace history entry (don't push SCANNING — SCAN_STOPPED's back goes to MENU)
        transition(UiState::SCAN_STOPPED);
      }
      break;
    }

    // ---- ATTACKING ----
    case UiState::ATTACKING: {
      if (!gated && input->back()) {
        last_input_ms = currentTime;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        goBack();
      }
      break;
    }

    // ---- SCAN_STOPPED ----
    case UiState::SCAN_STOPPED: {
      if (!gated && input->sel() && access_points->size() > 0) {
        last_input_ms = currentTime;
        ap_sel_cursor = 0;
        ap_sel_offset = 0;
        transition(UiState::SELECT_APS);

      } else if (!gated && input->back()) {
        last_input_ms = currentTime;
        goBack();
      }
      break;
    }

    // ---- SELECT_APS ----
    case UiState::SELECT_APS: {
      int ap_count = access_points->size();

      if (input->upHeld()) {
        // Long press UP — toggle checkbox on current item
        AccessPoint ap = access_points->get(ap_sel_cursor);
        ap.selected = !ap.selected;
        access_points->set(ap_sel_cursor, ap);
        last_input_ms = currentTime;
        renderSelectAPs();

      } else if (input->downHeld()) {
        // Long press DOWN — toggle checkbox on current item
        AccessPoint ap = access_points->get(ap_sel_cursor);
        ap.selected = !ap.selected;
        access_points->set(ap_sel_cursor, ap);
        last_input_ms = currentTime;
        renderSelectAPs();

      } else if (!gated && input->up()) {
        if (ap_sel_cursor > 0) {
          ap_sel_cursor--;
          if (ap_sel_cursor < ap_sel_offset) ap_sel_offset = ap_sel_cursor;
        } else {
          // Wrap to bottom
          ap_sel_cursor = ap_count - 1;
          ap_sel_offset = (ap_count > OLED_MAX_LINES) ? ap_count - OLED_MAX_LINES : 0;
        }
        last_input_ms = currentTime;
        renderSelectAPs();

      } else if (!gated && input->down()) {
        if (ap_sel_cursor < ap_count - 1) {
          ap_sel_cursor++;
          if (ap_sel_cursor >= ap_sel_offset + OLED_MAX_LINES)
            ap_sel_offset = ap_sel_cursor - OLED_MAX_LINES + 1;
        } else {
          // Wrap to top
          ap_sel_cursor = 0;
          ap_sel_offset = 0;
        }
        last_input_ms = currentTime;
        renderSelectAPs();

      } else if (!gated && input->sel()) {
        // SEL = Go / Confirm — consistent with every other screen
        last_input_ms = currentTime;

        int selected_count = 0;
        for (int i = 0; i < ap_count; i++) {
          if (access_points->get(i).selected) selected_count++;
        }

        if (selected_count == 0) {
          // Nothing selected — warn briefly then stay
          display_obj.updateBanner("Select APs");
          display_obj.clearScreen();
          display_obj.display_buffer->add("Hold UP/DOWN");
          display_obj.display_buffer->add("to select an AP");
          display_obj.display_buffer->add("first.");
          display_obj.displayBuffer();
          display_obj.drawBottomBar("Select", "Back");
          delay(1500);
          renderSelectAPs();

        } else if (history_depth > 0 &&
                   history[history_depth - 1] == UiState::SCAN_STOPPED) {
          // Came from SCAN_STOPPED — go to attack picker
          goBack();
          transition(UiState::ATTACK_PICKER);

        } else {
          // Came from MENU — launch the pending attack directly
          {
            const char* lbl = pending_label ? pending_label : kBannerAttacking;
            display_obj.updateBannerWithIcons(String(lbl), ICON_BOLT);
            active_icon_id = ICON_BOLT_ANIM;
            icon_frame    = 0;
            last_icon_ms  = 0;
          }
          wifi_scan_obj.StartScan(pending_action, 0);
          goBack();
          transition(UiState::ATTACKING);
        }

      } else if (!gated && input->back()) {
        // BACK = cancel, always go back regardless of selection
        last_input_ms = currentTime;
        goBack();
      }
      break;
    }

    // ---- ATTACK_PICKER ----
    case UiState::ATTACK_PICKER: {
      if (!gated && input->up()) {
        if (picker_cursor > 0) {
          picker_cursor--;
          if (picker_cursor < picker_offset) picker_offset = picker_cursor;
        } else {
          picker_cursor = POST_SCAN_ATTACKS_N - 1;
          picker_offset = (POST_SCAN_ATTACKS_N > OLED_MAX_LINES)
                        ? POST_SCAN_ATTACKS_N - OLED_MAX_LINES : 0;
        }
        last_input_ms = currentTime;
        renderAttackPicker();

      } else if (!gated && input->down()) {
        if (picker_cursor < (int8_t)POST_SCAN_ATTACKS_N - 1) {
          picker_cursor++;
          if (picker_cursor >= picker_offset + OLED_MAX_LINES)
            picker_offset = picker_cursor - OLED_MAX_LINES + 1;
        } else {
          picker_cursor = 0;
          picker_offset = 0;
        }
        last_input_ms = currentTime;
        renderAttackPicker();

      } else if (!gated && input->sel()) {
        last_input_ms = currentTime;
        const MenuItem& chosen = POST_SCAN_ATTACKS[picker_cursor];
        {
          const char* lbl = chosen.short_label ? chosen.short_label : chosen.label;
          display_obj.updateBannerWithIcons(String(lbl), ICON_BOLT);
          active_icon_id = ICON_BOLT_ANIM;
          icon_frame    = 0;
          last_icon_ms  = 0;
        }
        wifi_scan_obj.StartScan(chosen.action, chosen.color);
        transition(UiState::ATTACKING);

      } else if (!gated && input->back()) {
        last_input_ms = currentTime;
        goBack();  // → SELECT_APS
      }
      break;
    }
  }
}


// ================================================================
// main() — called every loop iteration
// ================================================================

void OledMenu::main(uint32_t currentTime) {
  handleInput(currentTime);

  // Tick animated banner icon during active states
  if (state == UiState::SCANNING || state == UiState::ATTACKING) {
    icon_frame = display_obj.tickBannerIcon(
        active_icon_id, icon_frame, currentTime, last_icon_ms);
  }

  // Drain display_buffer each frame while scanning — WiFiScan callbacks write here
  if (state == UiState::SCANNING) {
    if (display_obj.display_buffer->size() > 0) {
      display_obj.displayBuffer();
      display_obj.drawBottomBar("", "Stop");
    }
  }
}


// ================================================================
// Menu stack helpers
// ================================================================

void OledMenu::pushMenu(const MenuItem* items, uint8_t count) {
  if (depth >= kMaxDepth) return;

  menu_stack[depth]  = items;
  count_stack[depth] = count;
  sel_stack[depth]   = 0;
  off_stack[depth]   = 0;
  depth++;
}

void OledMenu::popMenu() {
  if (depth > 1) depth--;
}


// ================================================================
// Action classification
// ================================================================

bool OledMenu::actionIsSniffer(uint16_t action) {
  switch (action) {
    case WIFI_SCAN_PROBE:
    case WIFI_SCAN_AP:
    case WIFI_SCAN_DEAUTH:
    case WIFI_SCAN_STATION:
    case WIFI_PACKET_MONITOR:
    case WIFI_SCAN_PACKET_RATE:
    case WIFI_SCAN_EAPOL:
    case WIFI_SCAN_PWN:
    case WIFI_SCAN_PINESCAN:
    case WIFI_SCAN_MULTISSID:
    case WIFI_SCAN_ESPRESSIF:
    case WIFI_SCAN_DETECT_FOLLOW:
    case WIFI_SCAN_SAE_COMMIT:
    case WIFI_SCAN_RAW_CAPTURE:
    case WIFI_SCAN_SIG_STREN:
    case WIFI_SCAN_CHAN_ANALYZER:
    case WIFI_SCAN_CHAN_ACT:
    case WIFI_SCAN_TARGET_AP:
    case WIFI_SCAN_AP_STA:
    case WIFI_SCAN_WAR_DRIVE:
    case WIFI_SCAN_STATION_WAR_DRIVE:
    case BT_SCAN_ALL:
    case BT_SCAN_SKIMMERS:
    case BT_SCAN_AIRTAG:
    case BT_SCAN_AIRTAG_MON:
    case BT_SCAN_FLIPPER:
    case BT_SCAN_ANALYZER:
    case BT_SCAN_FLOCK:
    case BT_SCAN_FLOCK_WARDRIVE:
    case BT_SCAN_RAYBAN:
    case BT_SCAN_WAR_DRIVE:
      return true;
    default:
      return false;
  }
}

bool OledMenu::actionIsBT(uint16_t action) {
  switch (action) {
    case BT_SCAN_ALL:
    case BT_SCAN_SKIMMERS:
    case BT_SCAN_AIRTAG:
    case BT_SCAN_AIRTAG_MON:
    case BT_SCAN_FLIPPER:
    case BT_SCAN_ANALYZER:
    case BT_SCAN_FLOCK:
    case BT_SCAN_FLOCK_WARDRIVE:
    case BT_SCAN_RAYBAN:
    case BT_SCAN_WAR_DRIVE:
    case BT_ATTACK_SPAM_ALL:
    case BT_ATTACK_SOUR_APPLE:
    case BT_ATTACK_SWIFTPAIR_SPAM:
    case BT_ATTACK_SAMSUNG_SPAM:
    case BT_ATTACK_GOOGLE_SPAM:
    case BT_ATTACK_FLIPPER_SPAM:
      return true;
    default:
      return false;
  }
}

bool OledMenu::actionNeedsAPSelection(uint16_t action) {
  switch (action) {
    case WIFI_ATTACK_DEAUTH:
    case WIFI_ATTACK_DEAUTH_TARGETED:
    case WIFI_ATTACK_SLEEP:
    case WIFI_ATTACK_SLEEP_TARGETED:
    case WIFI_ATTACK_BAD_MSG:
    case WIFI_ATTACK_BAD_MSG_TARGETED:
    case WIFI_ATTACK_CSA:
    case WIFI_ATTACK_QUIET:
      return true;
    default:
      return false;
  }
}

#endif  // HAS_OLED
