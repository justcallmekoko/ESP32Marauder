#include "MenuFunctions.h"
#include "lang_var.h"

#ifdef HAS_SCREEN

extern const unsigned char menu_icons[][66];

MenuFunctions::MenuFunctions()
{
}

void MenuFunctions::buttonNotSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
    display_obj.key[b].drawButton(false, current_menu->list->get(x).name);
  #endif

  uint16_t color = this->getColor(current_menu->list->get(x).color);

  #ifdef HAS_FULL_SCREEN
    display_obj.tft.setFreeFont(MENU_FONT);
    display_obj.key[b].drawButton(false, current_menu->list->get(x).name);
    if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(x).icon],
                                      ICON_W,
                                      ICON_H,
                                      TFT_BLACK,
                                      color);
    display_obj.tft.setFreeFont(NULL);
  #endif
}

void MenuFunctions::buttonSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  uint16_t color = this->getColor(current_menu->list->get(x).color);

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
    display_obj.key[b].drawButton(true, current_menu->list->get(x).name);
  #endif

  #ifdef HAS_FULL_SCREEN
    display_obj.tft.setFreeFont(MENU_FONT);
    display_obj.key[b].drawButton(true, current_menu->list->get(x).name);
    if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(x).icon],
                                      ICON_W,
                                      ICON_H,
                                      TFT_BLACK,
                                      color);
    display_obj.tft.setFreeFont(NULL);
  #endif
}

void MenuFunctions::displayMenuButtons() {
  #ifdef HAS_ILI9341
    // Draw lines to show each menu button
    for (int i = 0; i < 3; i++) {

      // Draw horizontal line on left
      display_obj.tft.drawLine(0, 
                              TFT_HEIGHT / 3 * (i),
                              (TFT_WIDTH / 12) / 2,
                              TFT_HEIGHT / 3 * (i),
                              TFT_FARTGRAY);

      // Draw horizontal line on right
      display_obj.tft.drawLine(TFT_WIDTH - 1 - ((TFT_WIDTH / 12) / 2), 
                              TFT_HEIGHT / 3 * (i),
                              TFT_WIDTH,
                              TFT_HEIGHT / 3 * (i),
                              TFT_FARTGRAY);

      // Draw vertical line on left
      display_obj.tft.drawLine(0, 
                              (TFT_HEIGHT / 3 * (i)) - ((TFT_WIDTH / 12) / 2),
                              0,
                              (TFT_HEIGHT / 3 * (i)) + ((TFT_WIDTH / 12) / 2),
                              TFT_FARTGRAY);

      // Draw vertical line on right
      display_obj.tft.drawLine(TFT_WIDTH - 1, 
                              (TFT_HEIGHT / 3 * (i)) - ((TFT_WIDTH / 12) / 2),
                              TFT_WIDTH - 1,
                              (TFT_HEIGHT / 3 * (i)) + ((TFT_WIDTH / 12) / 2),
                              TFT_FARTGRAY);
    }
  #endif
}

// Function to check menu input
void MenuFunctions::main(uint32_t currentTime)
{
  // Some function exited and we need to go back to normal
  if (display_obj.exit_draw) {
    if (wifi_scan_obj.currentScanMode != WIFI_CONNECTED)
      wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    display_obj.exit_draw = false;
    this->orientDisplay();
  }
  if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
      (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
      (wifi_scan_obj.currentScanMode == OTA_UPDATE) ||
      (wifi_scan_obj.currentScanMode == ESP_UPDATE) ||
      (wifi_scan_obj.currentScanMode == SHOW_INFO) ||
      (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_DATA) ||
      (wifi_scan_obj.currentScanMode == GPS_POI) ||
      (wifi_scan_obj.currentScanMode == GPS_TRACKER) ||
      (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_NMEA)) {
    if (wifi_scan_obj.orient_display) {
      this->orientDisplay();
      wifi_scan_obj.orient_display = false;
    }
  }

  if (currentTime != 0) {
    if (currentTime - initTime >= BANNER_TIME) {
      this->initTime = millis();
      if ((wifi_scan_obj.currentScanMode != LV_JOIN_WIFI) &&
          (wifi_scan_obj.currentScanMode != LV_ADD_SSID))
        this->updateStatusBar();
      
      // Do channel analyzer stuff
      if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)){
        #ifdef HAS_SCREEN
          this->setGraphScale(this->graphScaleCheck(wifi_scan_obj._analyzer_values));

          this->drawGraph(wifi_scan_obj._analyzer_values);
        #endif
      }

      if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
        #ifdef HAS_SCREEN
          this->setGraphScale(this->graphScaleCheckSmall(wifi_scan_obj.channel_activity));

          this->drawGraphSmall(wifi_scan_obj.channel_activity);

        #endif
      }
    }
  }


  boolean pressed = false;
  // This is code from bodmer's keypad example
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

  // Get the display buffer out of the way
  if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF ) &&
      (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP_TARGETED) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
	  (wifi_scan_obj.currentScanMode != WIFI_ATTACK_FUNNY_BEACON) &&
      (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
    display_obj.displayBuffer();


  int pre_getTouch = millis();

  #ifdef HAS_ILI9341
    if (!this->disable_touch)
      pressed = display_obj.updateTouch(&t_x, &t_y);
  #endif


  // This is if there are scans/attacks going on
  #ifdef HAS_ILI9341
    if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
        (pressed) &&
        (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
        (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
        (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
        (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
        (wifi_scan_obj.currentScanMode != GPS_POI) &&
        (wifi_scan_obj.currentScanMode != GPS_TRACKER) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA))
    {
      // Stop the current scan
      if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SAE_COMMIT) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DETECT_FOLLOW) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
          (wifi_scan_obj.currentScanMode == WIFI_PING_SCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_ARP_SCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_PORT_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SSH) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_TELNET) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DNS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMTP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTPS) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_RDP) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_PINESCAN) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_MULTISSID) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP_TARGETED) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SAE_COMMIT) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
		      (wifi_scan_obj.currentScanMode == WIFI_ATTACK_FUNNY_BEACON) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
          (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK_WARDRIVE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE_TWO) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
          (wifi_scan_obj.currentScanMode == BT_SPOOF_AIRTAG) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
          (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER))
      {
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  
        // If we don't do this, the text and button coordinates will be off
        display_obj.init();
  
        // Take us back to the menu
        changeMenu(current_menu, true);
      }
  
      x = -1;
      y = -1;
  
      return;
    }
  #endif

  #ifdef HAS_BUTTONS

    #if (C_BTN >= 0) && !defined(MARAUDER_CARDPUTER)
      bool c_btn_press = c_btn.justPressed();
    #elif defined(MARAUDER_CARDPUTER)
      bool c_btn_press = this->isKeyPressed('(');
    #endif

    #ifndef HAS_ILI9341
    
      if ((c_btn_press) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
          (wifi_scan_obj.currentScanMode != WIFI_CONNECTED) &&
          (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
          (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
          (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
          (wifi_scan_obj.currentScanMode != GPS_POI) &&
          (wifi_scan_obj.currentScanMode != GPS_TRACKER) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA))
      {
        // Stop the current scan
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SAE_COMMIT) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DETECT_FOLLOW) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
            (wifi_scan_obj.currentScanMode == WIFI_PING_SCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_ARP_SCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_PORT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SSH) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TELNET) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DNS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SMTP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_HTTPS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_RDP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PINESCAN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_MULTISSID) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BAD_MSG) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SLEEP_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_SAE_COMMIT) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
			      (wifi_scan_obj.currentScanMode == WIFI_ATTACK_FUNNY_BEACON) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG_MON) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLOCK_WARDRIVE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SIMPLE_TWO) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_SPOOF_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER))
        {
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    
          // If we don't do this, the text and button coordinates will be off
          display_obj.init();
    
          // Take us back to the menu
          changeMenu(current_menu);
        }
    
        x = -1;
        y = -1;
    
        return;
      }
    #endif

  #endif


  // Check if any key coordinate boxes contain the touch coordinates
  // This is for when on a menu
  // Make sure to add certain scanning functions here or else
  // menu items will be selected while scans and attacks are running
  #ifdef HAS_ILI9341
    if ((wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BAD_MSG) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SLEEP_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_SAE_COMMIT) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_PACKET_RATE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_RAW_CAPTURE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ANALYZER) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ACT) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_SIG_STREN) &&
		    (wifi_scan_obj.currentScanMode != WIFI_ATTACK_FUNNY_BEACON) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_EAPOL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
    {
      // Need this to set all keys to false
      /*for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++) {
        if (pressed && display_obj.key[b].contains(t_x, t_y)) {
          display_obj.key[b].press(true);  // tell the button it is pressed
        } else {
          display_obj.key[b].press(false);  // tell the button it is NOT pressed
        }
      }*/

      // Detect up, down, select
      uint8_t menu_button = display_obj.menuButton(&t_x, &t_y, pressed);

      if (menu_button > -1) {
        if (menu_button == UP_BUTTON) {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
              (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
            if (current_menu->selected > 0) {
              current_menu->selected--;
              // Page up
              if (current_menu->selected < this->menu_start_index) {
                this->buildButtons(current_menu, current_menu->selected);
                this->displayCurrentMenu(current_menu->selected);
              }
              this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
              if (!current_menu->list->get(current_menu->selected + 1).selected)
                this->buttonNotSelected(current_menu->selected + 1 - this->menu_start_index, current_menu->selected + 1);
            }
            // Loop to end
            else {
              current_menu->selected = current_menu->list->size() - 1;
              if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              }
              this->buttonSelected(current_menu->selected, current_menu->selected);
              if (!current_menu->list->get(0).selected)
                this->buttonNotSelected(0, this->menu_start_index);
            }
          }
          else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.set_channel < 14)
                wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
              else
                wifi_scan_obj.changeChannel(1);
            #else
              if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
                wifi_scan_obj.dual_band_channel_index++;
              else
                wifi_scan_obj.dual_band_channel_index = 0;

              wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
            #endif
          }
          else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.activity_page < MAX_CHANNEL / CHAN_PER_PAGE) {
                wifi_scan_obj.activity_page++;
              }
            #else
              if (wifi_scan_obj.activity_page < DUAL_BAND_CHANNELS / CHAN_PER_PAGE) {
                wifi_scan_obj.activity_page++;
              }
            #endif
            wifi_scan_obj.drawChannelLine();
          }
        }
        if (menu_button == DOWN_BUTTON) {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
              (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
            if (current_menu->selected < current_menu->list->size() - 1) {
              current_menu->selected++;
              // Page down
              if (current_menu->selected - this->menu_start_index >= BUTTON_SCREEN_LIMIT) {
                this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              }
              else
                this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
              if (!current_menu->list->get(current_menu->selected - 1).selected)
                this->buttonNotSelected(current_menu->selected - 1 - this->menu_start_index, current_menu->selected - 1);
            }
            // Loop to beginning
            else {
              if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                current_menu->selected = 0;
                this->buildButtons(current_menu);
                this->displayCurrentMenu();
                this->buttonSelected(current_menu->selected);
              }
              else {
                current_menu->selected = 0;
                this->buttonSelected(current_menu->selected);
                if (!current_menu->list->get(current_menu->list->size() - 1).selected)
                  this->buttonNotSelected(current_menu->list->size() - 1);
              }
            }
          }
          else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.set_channel > 1)
                wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
              else
                wifi_scan_obj.changeChannel(14);
            #else
              if (wifi_scan_obj.dual_band_channel_index > 0)
                wifi_scan_obj.dual_band_channel_index--;
              else
                wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

              wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
            #endif
          }
          else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
            #ifndef HAS_DUAL_BAND
              if (wifi_scan_obj.activity_page > 1) {
                wifi_scan_obj.activity_page--;
              }
            #else
              if (wifi_scan_obj.activity_page > 0) {
                wifi_scan_obj.activity_page--;
              }
            #endif
            wifi_scan_obj.drawChannelLine();
          }
        }
        if(menu_button == SELECT_BUTTON) {
          current_menu->list->get(current_menu->selected).callable();
        }
        else {
          if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
              (wifi_scan_obj.currentScanMode == WIFI_CONNECTED))
            this->displayMenuButtons();
        }
      }
  
      // Check if any key has changed state
      /*for (uint8_t b = 0; b < current_menu->list->size(); b++) {
        display_obj.tft.setFreeFont(MENU_FONT);
        if (display_obj.key[b].justPressed()) {
          display_obj.key[b].drawButton(true, current_menu->list->get(b).name);
          if (current_menu->list->get(b).name != text09)
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + b * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(b).icon],
                                        ICON_W,
                                        ICON_H,
                                        this->getColor(current_menu->list->get(b).color),
                                        TFT_BLACK);
        }
  
        // If button was just release, execute the button's function
        if ((display_obj.key[b].justReleased()) && (!pressed))
        {
          display_obj.key[b].drawButton(false, current_menu->list->get(b).name);
          current_menu->list->get(b).callable();
        }
        // This
        else if ((display_obj.key[b].justReleased()) && (pressed)) {
          display_obj.key[b].drawButton(false, current_menu->list->get(b).name);
          if (current_menu->list->get(b).name != text09)
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + b * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(b).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        this->getColor(current_menu->list->get(b).color));
        }
  
        display_obj.tft.setFreeFont(NULL);
      }*/
    }
    x = -1;
    y = -1;
  #endif

  // Menu navigation and paging
  #ifdef HAS_BUTTONS
    // Don't do this for touch screens
    #if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1) || defined(MARAUDER_CYD_MICRO) || defined(MARAUDER_CYD_GUITION) || defined(MARAUDER_CYD_2USB) || defined(MARAUDER_CYD_3_5_INCH))
      #if !defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
        #if (U_BTN >= 0 || defined(MARAUDER_CARDPUTER))
          #if (U_BTN >= 0)
            if (u_btn.justPressed()) {
          #elif defined(MARAUDER_CARDPUTER)
            if (this->isKeyPressed(';')) {
          #endif
              if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
                  (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
                  (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
                if (current_menu->selected > 0) {
                  current_menu->selected--;
                  // Page up
                  if (current_menu->selected < this->menu_start_index) {
                    this->buildButtons(current_menu, current_menu->selected);
                    this->displayCurrentMenu(current_menu->selected);
                  }
                  this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
                  if (!current_menu->list->get(current_menu->selected + 1).selected)
                    this->buttonNotSelected(current_menu->selected + 1 - this->menu_start_index, current_menu->selected + 1);
                }
                // Loop to end
                else {
                  current_menu->selected = current_menu->list->size() - 1;
                  if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                    this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                    this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                  }
                  this->buttonSelected(current_menu->selected, current_menu->selected);
                  if (!current_menu->list->get(0).selected)
                    this->buttonNotSelected(0, this->menu_start_index);
                }
              }
              else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                      (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
                #ifndef HAS_DUAL_BAND
                  if (wifi_scan_obj.set_channel < 14)
                    wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
                  else
                    wifi_scan_obj.changeChannel(1);
                #else
                  if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
                    wifi_scan_obj.dual_band_channel_index++;
                  else
                    wifi_scan_obj.dual_band_channel_index = 0;

                  wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
                #endif
              }
              else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
                #ifndef HAS_DUAL_BAND
                  if (wifi_scan_obj.activity_page < MAX_CHANNEL / CHAN_PER_PAGE) {
                    wifi_scan_obj.activity_page++;
                  }
                #else
                  if (wifi_scan_obj.activity_page < DUAL_BAND_CHANNELS / CHAN_PER_PAGE) {
                    wifi_scan_obj.activity_page++;
                  }
                #endif
                wifi_scan_obj.drawChannelLine();
              }
            }
        #endif
      #endif

      #if (D_BTN >= 0 || defined(MARAUDER_CARDPUTER))
      #if (D_BTN >= 0)
      if (d_btn.justPressed()){
      #elif defined(MARAUDER_CARDPUTER)
      if (this->isKeyPressed('.')){
      #endif
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
            (wifi_scan_obj.currentScanMode == WIFI_CONNECTED) ||
            (wifi_scan_obj.currentScanMode == OTA_UPDATE)) {
          if (current_menu->selected < current_menu->list->size() - 1) {
            current_menu->selected++;
            // Page down
            if (current_menu->selected - this->menu_start_index >= BUTTON_SCREEN_LIMIT) {
              this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
              this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
            }
            else
              this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
            if (!current_menu->list->get(current_menu->selected - 1).selected)
              this->buttonNotSelected(current_menu->selected - 1 - this->menu_start_index, current_menu->selected - 1);
          }
          // Loop to beginning
          else {
            if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
              current_menu->selected = 0;
              this->buildButtons(current_menu);
              this->displayCurrentMenu();
              this->buttonSelected(current_menu->selected);
            }
            else {
              current_menu->selected = 0;
              this->buttonSelected(current_menu->selected);
              if (!current_menu->list->get(current_menu->list->size() - 1).selected)
                this->buttonNotSelected(current_menu->list->size() - 1);
            }
          }
        }
        else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel > 1)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
            else
              wifi_scan_obj.changeChannel(14);
          #else
            if (wifi_scan_obj.dual_band_channel_index > 0)
              wifi_scan_obj.dual_band_channel_index--;
            else
              wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
        else if (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ACT) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.activity_page > 1) {
              wifi_scan_obj.activity_page--;
            }
          #else
            if (wifi_scan_obj.activity_page > 0) {
              wifi_scan_obj.activity_page--;
            }
          #endif
          wifi_scan_obj.drawChannelLine();
        }
      }
      #endif

      #if (R_BTN >= 0 || defined(MARAUDER_CARDPUTER))
      #if (R_BTN >= 0)
      if (r_btn.justPressed()) {
      #elif defined(MARAUDER_CARDPUTER)
      if (this->isKeyPressed('/')) {
      #endif
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel < 14)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
            else
              wifi_scan_obj.changeChannel(1);
          #else
            if (wifi_scan_obj.dual_band_channel_index < DUAL_BAND_CHANNELS - 1)
              wifi_scan_obj.dual_band_channel_index++;
            else
              wifi_scan_obj.dual_band_channel_index = 0;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
      }
      #endif

      #if (L_BTN >= 0 || defined(MARAUDER_CARDPUTER))
      #if (L_BTN >= 0)
      if (l_btn.justPressed()) {
      #elif defined(MARAUDER_CARDPUTER)
      if (this->isKeyPressed(',')) {
      #endif
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) {
          #ifndef HAS_DUAL_BAND
            if (wifi_scan_obj.set_channel > 1)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
            else
              wifi_scan_obj.changeChannel(14);
          #else
            if (wifi_scan_obj.dual_band_channel_index > 0)
              wifi_scan_obj.dual_band_channel_index--;
            else
              wifi_scan_obj.dual_band_channel_index = DUAL_BAND_CHANNELS - 1;

            wifi_scan_obj.changeChannel(wifi_scan_obj.dual_band_channels[wifi_scan_obj.dual_band_channel_index]);
          #endif
        }
      }
      #endif

      if(c_btn_press){
        current_menu->list->get(current_menu->selected).callable();
      }

    #endif
  #endif
}

#if BATTERY_ANALOG_ON == 1
byte battery_analog_array[10];
byte battery_count = 0;
byte battery_analog_last = 101;
#define BATTERY_CHECK 50
uint16_t battery_analog = 0;
void MenuFunctions::battery(bool initial)
{
  if (BATTERY_ANALOG_ON) {
    uint8_t n = 0;
    byte battery_analog_sample[10];
    byte deviation;
    if (battery_count == BATTERY_CHECK - 5)  digitalWrite(BATTERY_PIN, HIGH);
    else if (battery_count == 5) digitalWrite(BATTERY_PIN, LOW);
    if (battery_count == 0) {
      battery_analog = 0;
      for (n = 9; n > 0; n--)battery_analog_array[n] = battery_analog_array[n - 1];
      for (n = 0; n < 10; n++) {
        battery_analog_sample[n] = map((analogRead(ANALOG_PIN) * 5), 2400, 4200, 0, 100);
        if (battery_analog_sample[n] > 100) battery_analog_sample[n] = 100;
        else if (battery_analog_sample[n] < 0) battery_analog_sample[n] = 0;
        battery_analog += battery_analog_sample[n];
      }
      battery_analog = battery_analog / 10;
      for (n = 0; n < 10; n++) {
        deviation = abs(battery_analog - battery_analog_sample[n]);
        if (deviation >= 10) battery_analog_sample[n] = battery_analog;
      }
      battery_analog = 0;
      for (n = 0; n < 10; n++) battery_analog += battery_analog_sample[n];
      battery_analog = battery_analog / 10;
      battery_analog_array[0] = battery_analog;
      if (battery_analog_array[9] > 0 ) {
        battery_analog = 0;
        for (n = 0; n < 10; n++) battery_analog += battery_analog_array[n];
        battery_analog = battery_analog / 10;
      }
      battery_count ++;
    }
    else if (battery_count < BATTERY_CHECK) battery_count++;
    else if (battery_count >= BATTERY_CHECK) battery_count = 0;

    if (battery_analog_last != battery_analog) {
      battery_analog_last = battery_analog;
      MenuFunctions::battery2();
    }
  }
}
void MenuFunctions::battery2(bool initial)
{
  uint16_t the_color;
  if ( digitalRead(CHARGING_PIN) == 1) the_color = TFT_BLUE;
  else if (battery_analog < 20) the_color = TFT_RED;
  else if (battery_analog < 40)  the_color = TFT_YELLOW;
  else the_color = TFT_GREEN;

  display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
  display_obj.tft.fillRect(186, 0, 50, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.drawXBitmap(186,
                              0,
                              menu_icons[STATUS_BAT],
                              16,
                              16,
                              STATUSBAR_COLOR,
                              the_color);
  display_obj.tft.drawString((String) battery_analog + "%", 204, 0, 2);
}
#else
void MenuFunctions::battery(bool initial)
{
  #ifdef HAS_BATTERY
    uint16_t the_color;
    if (battery_obj.i2c_supported)
    {
      // Could use int compare maybe idk
      if (((String)battery_obj.battery_level != "25") && ((String)battery_obj.battery_level != "0"))
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;

      if ((battery_obj.battery_level != battery_obj.old_level) || (initial)) {
        battery_obj.old_level = battery_obj.battery_level;
        display_obj.tft.fillRect(204, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      }

      display_obj.tft.setCursor(0, 1);
      /*if (!this->disable_touch) {
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[STATUS_BAT],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
      }*/
      display_obj.tft.drawString((String)battery_obj.battery_level + "%", 204, 0, 2);
    }
  #endif
}
void MenuFunctions::battery2(bool initial)
{
  MenuFunctions::battery(initial);
}
#endif

void MenuFunctions::updateStatusBar()
{
  display_obj.tft.setTextSize(1);

  bool status_changed = false;
  
  #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_MINI_V3)
    display_obj.tft.setFreeFont(NULL);
  #endif
  
  uint16_t the_color; 

  #ifdef HAS_GPS
    if (this->old_gps_sat_count != gps_obj.getNumSats()) {
      this->old_gps_sat_count = gps_obj.getNumSats();
      display_obj.tft.fillRect(0, 0, TFT_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      status_changed = true;
    }
  #endif

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.drawXBitmap(4,
                                    0,
                                    menu_icons[STATUS_GPS],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR, true);

        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #elif defined(HAS_SCREEN)
        display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR, true);
        display_obj.tft.drawString("GPS", 0, 0, 1);
      #endif
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR, true);

  // WiFi Channel Stuff
  uint8_t primaryChannel;
  wifi_second_chan_t secondChannel;
  esp_err_t err = esp_wifi_get_channel(&primaryChannel, &secondChannel);

  uint8_t current_channel = wifi_scan_obj.set_channel;

  if (err == ESP_OK)
    current_channel = primaryChannel;

  if ((current_channel != wifi_scan_obj.old_channel) || (status_changed)) {
    wifi_scan_obj.old_channel = current_channel;
    #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER) || defined(MARAUDER_CARDPUTER) || defined(MARAUDER_MINI_V3)
      display_obj.tft.fillRect(TFT_WIDTH/4, 0, CHAR_WIDTH * 6, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #elif defined(HAS_DUAL_BAND)
      display_obj.tft.fillRect(50, 0, (CHAR_WIDTH / 2) * 8, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #else
      display_obj.tft.fillRect(50, 0, (CHAR_WIDTH / 2) * 7, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #endif
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, 50, 0, 2);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, TFT_WIDTH/4, 0, 1);
    #endif
  }

  // RAM Stuff
  wifi_scan_obj.freeRAM();
  if ((wifi_scan_obj.free_ram != wifi_scan_obj.old_free_ram) || (status_changed)) {
    wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
    //display_obj.tft.fillRect(100, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #ifdef HAS_FULL_SCREEN
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", 100, 0, 2);
    #else
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", 100, 0, 1);
      display_obj.tft.drawString("P:" + String(getPSRAMUsagePercent()) + "%", 100, 8, 1);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
    #else
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%" + " P:" + String(getPSRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
    #endif
  #endif
  }

  // Draw battery info
  MenuFunctions::battery(false);
  display_obj.tft.fillRect(186, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);

  // Disable touch stuff
  #ifdef HAS_ILI9341
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
      else {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_DARKGREY);
      }
    #endif
  #endif

  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;

    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR, true);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  // WiFi connection status stuff
  if (wifi_scan_obj.wifi_connected) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - 16,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - 16,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }

  // Force PMKID stuff
  if ((wifi_scan_obj.force_pmkid) || (wifi_scan_obj.ep_deauth)) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - (16 * 2),
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - (16 * 2),
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }
}

void MenuFunctions::drawStatusBar()
{
  display_obj.tft.setTextSize(1);
  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
  #endif
  display_obj.tft.fillRect(0, 0, TFT_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

  uint16_t the_color;

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.drawXBitmap(4,
                                    0,
                                    menu_icons[STATUS_GPS],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #endif
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);


  // WiFi Channel Stuff
  uint8_t primaryChannel;
  wifi_second_chan_t secondChannel;
  esp_err_t err = esp_wifi_get_channel(&primaryChannel, &secondChannel);

  if (err == ESP_OK)
    wifi_scan_obj.old_channel = primaryChannel;
  else
    wifi_scan_obj.old_channel = wifi_scan_obj.set_channel;

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.fillRect(43, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #else
    display_obj.tft.fillRect(50, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #endif
  #ifdef HAS_FULL_SCREEN
    display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, 50, 0, 2);
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.old_channel, TFT_WIDTH/4, 0, 1);
  #endif

  // RAM Stuff
  wifi_scan_obj.freeRAM();
  wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
  display_obj.tft.fillRect(100, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #ifdef HAS_FULL_SCREEN
    //display_obj.tft.setCursor(100, 0);
    //display_obj.tft.setFreeFont(2);
    //display_obj.tft.print("D:" + String(getDRAMUsagePercent()) + "%");
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", 100, 0, 2);
    #else
      //display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%" + " P:" + String(getPSRAMUsagePercent()) + "%", 100, 0, 1);
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", 100, 0, 1);
      display_obj.tft.drawString("P:" + String(getPSRAMUsagePercent()) + "%", 100, 8, 1);
    #endif
    //display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 100, 0, 2);
  #endif

  #ifdef HAS_MINI_SCREEN
    //display_obj.tft.setCursor(TFT_WIDTH/1.75, 0);
    //display_obj.tft.setFreeFont(1);
    //display_obj.tft.print("D:" + String(getDRAMUsagePercent()) + "%");
    #ifndef HAS_PSRAM
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
    #else
      display_obj.tft.drawString("D:" + String(getDRAMUsagePercent()) + "%" + " P:" + String(getPSRAMUsagePercent()) + "%", TFT_WIDTH/1.75, 0, 1);
    #endif
    //display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", TFT_WIDTH/1.75, 0, 1);
  #endif


  MenuFunctions::battery(true);
  display_obj.tft.fillRect(186, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);


  // Disable touch stuff
  #ifdef HAS_ILI9341
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
      else {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_DARKGREY);
      }
    #endif
  #endif

  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;
  

    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  // WiFi connection status stuff
  if (wifi_scan_obj.wifi_connected) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - 16,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - 16,
                                  0,
                                  menu_icons[JOINED],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }

  // Force PMKID stuff
  if ((wifi_scan_obj.force_pmkid) || (wifi_scan_obj.ep_deauth)) {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - (16 * 2),
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_GREEN);
    #endif
  } else {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.drawXBitmap(170 - (16 * 2),
                                  0,
                                  menu_icons[FORCE],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  TFT_DARKGREY);
    #endif
  }
}

void MenuFunctions::orientDisplay()
{
  display_obj.init();

  display_obj.tft.setRotation(SCREEN_ORIENTATION); // Portrait

  display_obj.tft.setCursor(0, 0);

  #ifdef HAS_ILI9341
    #ifndef HAS_CYD_TOUCH
      display_obj.setCalData();
    #else
      display_obj.touchscreen.setRotation(0);
    #endif
  #endif

  changeMenu(current_menu);
}

void MenuFunctions::runBoolSetting(String key) {
  display_obj.tftDrawRedOnOffButton();
}

String MenuFunctions::callSetting(String key) {
  specSettingMenu.name = key;
  
  String setting_type = settings_obj.getSettingType(key);

  if (setting_type == "bool") {
    return "bool";
  }
}

void MenuFunctions::displaySetting(String key, Menu* menu, int index) {
  specSettingMenu.name = key;

  bool setting_value = settings_obj.loadSetting<bool>(key);

  // Make a local copy of menu node
  MenuNode node = menu->list->get(index);

  display_obj.tft.setTextWrap(false);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setCursor(0, 100);
  display_obj.tft.setTextSize(1);

  // Set local copy value
  if (!setting_value) {
    display_obj.tft.setTextColor(TFT_RED);
    display_obj.tft.println(F(text_table1[4]));
    node.selected = false;
  }
  else {
    display_obj.tft.setTextColor(TFT_GREEN);
    display_obj.tft.println(F(text_table1[5]));
    node.selected = true;
  }

  // Put local copy back into menu
  menu->list->set(index, node);
    
}

#ifdef MARAUDER_CARDPUTER
bool MenuFunctions::isKeyPressed(char c)
{
  M5CardputerKeyboard.updateKeyList();
  M5CardputerKeyboard.updateKeysState();
  bool pressed = M5CardputerKeyboard.isKeyPressed(c);

  if (pressed)
    delay(200);

  return pressed;
}
#endif

// Function to build the menus
void MenuFunctions::RunSetup()
{
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  extern LinkedList<AirTag>* airtags;
  extern LinkedList<IPAddress>* ipList;
  extern LinkedList<ProbeReqSsid>* probe_req_ssids;
  extern LinkedList<ssid>* ssids;

  this->disable_touch = false;

  #ifdef MARAUDER_CARDPUTER
    M5CardputerKeyboard.begin();
  #endif
   
  // root menu stuff
  mainMenu.list = new LinkedList<MenuNode>(); // Get list in first menu ready

  // Main menu stuff
  wifiMenu.list = new LinkedList<MenuNode>(); // Get list in second menu ready
  bluetoothMenu.list = new LinkedList<MenuNode>(); // Get list in third menu ready
  deviceMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      gpsMenu.list = new LinkedList<MenuNode>();
      gpsInfoMenu.list = new LinkedList<MenuNode>();
    }
  #endif

  // Device menu stuff
  failedUpdateMenu.list = new LinkedList<MenuNode>();
  confirmMenu.list = new LinkedList<MenuNode>();
  updateMenu.list = new LinkedList<MenuNode>();
  settingsMenu.list = new LinkedList<MenuNode>();
  specSettingMenu.list = new LinkedList<MenuNode>();
  infoMenu.list = new LinkedList<MenuNode>();
  // WiFi menu stuff
  wifiSnifferMenu.list = new LinkedList<MenuNode>();
  wifiScannerMenu.list = new LinkedList<MenuNode>();
  wifiAttackMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_GPS
    wardrivingMenu.list = new LinkedList<MenuNode>();
  #endif
  wifiGeneralMenu.list = new LinkedList<MenuNode>();
  wifiAPMenu.list = new LinkedList<MenuNode>();
  wifiIPMenu.list = new LinkedList<MenuNode>();
  apInfoMenu.list = new LinkedList<MenuNode>();
  setMacMenu.list = new LinkedList<MenuNode>();
  genAPMacMenu.list = new LinkedList<MenuNode>();
  wifiStationMenu.list = new LinkedList<MenuNode>();
  selectProbeSSIDsMenu.list = new LinkedList<MenuNode>();

  // WiFi HTML menu stuff
  htmlMenu.list = new LinkedList<MenuNode>();
  miniKbMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_SD
    sdDeleteMenu.list = new LinkedList<MenuNode>();
  #endif

  // Bluetooth menu stuff
  bluetoothSnifferMenu.list = new LinkedList<MenuNode>();
  bluetoothAttackMenu.list = new LinkedList<MenuNode>();

  // Settings stuff
  generateSSIDsMenu.list = new LinkedList<MenuNode>();
  clearSSIDsMenu.list = new LinkedList<MenuNode>();
  clearAPsMenu.list = new LinkedList<MenuNode>();
  saveFileMenu.list = new LinkedList<MenuNode>();

  saveSSIDsMenu.list = new LinkedList<MenuNode>();
  loadSSIDsMenu.list = new LinkedList<MenuNode>();
  saveAPsMenu.list = new LinkedList<MenuNode>();
  loadAPsMenu.list = new LinkedList<MenuNode>();
  saveATsMenu.list = new LinkedList<MenuNode>();
  loadATsMenu.list = new LinkedList<MenuNode>();

  evilPortalMenu.list = new LinkedList<MenuNode>();
  ssidsMenu.list = new LinkedList<MenuNode>();

  gpsPOIMenu.list = new LinkedList<MenuNode>();

  // Work menu names
  mainMenu.name = text_table1[6];
  wifiMenu.name = text_table1[7];
  deviceMenu.name = text_table1[9];
  failedUpdateMenu.name = text_table1[11];
  confirmMenu.name = text_table1[13];
  updateMenu.name = text_table1[15];
  infoMenu.name = text_table1[17];
  settingsMenu.name = text_table1[18];
  bluetoothMenu.name = text_table1[19];
  wifiSnifferMenu.name = text_table1[20];
  wifiScannerMenu.name = "Scanners";
  wifiAttackMenu.name = text_table1[21];
  wifiGeneralMenu.name = text_table1[22];
  saveFileMenu.name = "Save/Load Files";
  saveSSIDsMenu.name = "Save SSIDs";
  loadSSIDsMenu.name = "Load SSIDs";
  saveAPsMenu.name = "Save APs";
  loadAPsMenu.name = "Load APs";
  saveATsMenu.name = "Save Airtags";
  loadATsMenu.name = "Load Airtags";

  bluetoothSnifferMenu.name = text_table1[23];
  bluetoothAttackMenu.name = "Bluetooth Attacks";
  generateSSIDsMenu.name = text_table1[27];
  clearSSIDsMenu.name = text_table1[28];
  clearAPsMenu.name = text_table1[29];
  wifiAPMenu.name = "Select";
  wifiIPMenu.name = "Active IPs";
  apInfoMenu.name = "AP Info";
  setMacMenu.name = "Set MACs";
  genAPMacMenu.name = "Generate AP MAC";
  wifiStationMenu.name = "Select Stations";
  #ifdef HAS_GPS
    gpsMenu.name = "GPS"; 
    gpsInfoMenu.name = "GPS Data";
    wardrivingMenu.name = "Wardriving";
  #endif  
  htmlMenu.name = "EP HTML List";
  miniKbMenu.name = "Mini Keyboard";
  #ifdef HAS_SD
    sdDeleteMenu.name = "Delete SD Files";
  #endif
  selectProbeSSIDsMenu.name = "Probe Requests";
  evilPortalMenu.name = "Evil Portal";
  ssidsMenu.name = "SSIDs";

  gpsPOIMenu.name = "GPS POI";

  // Build Main Menu
  mainMenu.parentMenu = NULL;
  this->addNodes(&mainMenu, text_table1[7], TFTGREEN, NULL, WIFI, [this]() {
    this->changeMenu(&wifiMenu, true);
  });
  this->addNodes(&mainMenu, text_table1[19], TFTCYAN, NULL, BLUETOOTH, [this]() {
    this->changeMenu(&bluetoothMenu, true);
  });
  #ifdef HAS_GPS
	if (gps_obj.getGpsModuleStatus()) {
    	this->addNodes(&mainMenu, text1_66, TFTRED, NULL, GPS_MENU, [this]() {
      	this->changeMenu(&gpsMenu, true);
    	});
	}
  #endif
  this->addNodes(&mainMenu, text_table1[9], TFTBLUE, NULL, DEVICE, [this]() {
    this->changeMenu(&deviceMenu, true);
  });
  this->addNodes(&mainMenu, text_table1[30], TFTLIGHTGREY, NULL, REBOOT, []() {
    ESP.restart();
  });

  // Build WiFi Menu
  wifiMenu.parentMenu = &mainMenu; // Main Menu is second menu parent
  this->addNodes(&wifiMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiMenu.parentMenu, true);
  });
  this->addNodes(&wifiMenu, text_table1[31], TFTYELLOW, NULL, SNIFFERS, [this]() {
    this->changeMenu(&wifiSnifferMenu, true);
  });
  this->addNodes(&wifiMenu, "Scanners", TFTORANGE, NULL, SCANNERS, [this]() {
    this->changeMenu(&wifiScannerMenu, true);
  });
  #ifdef HAS_GPS
    this->addNodes(&wifiMenu, "Wardriving", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
      this->changeMenu(&wardrivingMenu, true);
    });
  #endif
  this->addNodes(&wifiMenu, text_table1[32], TFTRED, NULL, ATTACKS, [this]() {
    this->changeMenu(&wifiAttackMenu, true);
  });
  this->addNodes(&wifiMenu, text_table1[33], TFTPURPLE, NULL, GENERAL_APPS, [this]() {
    this->changeMenu(&wifiGeneralMenu, true);
  });

  // Build WiFi scanner Menu
  wifiScannerMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiScannerMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiScannerMenu.parentMenu, true);
  });
  this->addNodes(&wifiScannerMenu, "Ping Scan", TFTGREEN, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_PING_SCAN, TFT_CYAN);
  });
  #ifndef HAS_DUAL_BAND
    this->addNodes(&wifiScannerMenu, "ARP Scan", TFTCYAN, NULL, SCANNERS, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_ARP_SCAN, TFT_CYAN);
    });
  #endif
  this->addNodes(&wifiScannerMenu, "Port Scan All", TFTMAGENTA, NULL, BEACON_LIST, [this](){
    // Add the back button
    wifiIPMenu.list->clear();
      this->addNodes(&wifiIPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiIPMenu.parentMenu, true);
    });

    // Populate the menu with buttons
    for (int i = 0; i < ipList->size(); i++) {
      // This is the menu node
      this->addNodes(&wifiIPMenu, ipList->get(i).toString(), TFTBLUE, NULL, 255, [this, i](){
        Serial.println("Selected: " + ipList->get(i).toString());
        wifi_scan_obj.current_scan_ip = ipList->get(i);
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(WIFI_PORT_SCAN_ALL, TFT_BLUE);
      });
    }
    this->changeMenu(&wifiIPMenu, true);
  });
  this->addNodes(&wifiScannerMenu, "SSH Scan", TFTORANGE, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SSH, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "Telnet Scan", TFTRED, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_TELNET, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "SMTP Scan", TFTWHITE, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SMTP, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "DNS Scan", TFTLIME, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_DNS, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "HTTP Scan", TFTSKYBLUE, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_HTTP, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "HTTPS Scan", TFTYELLOW, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_HTTPS, TFT_CYAN);
  });
  this->addNodes(&wifiScannerMenu, "RDP Scan", TFTPURPLE, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_RDP, TFT_CYAN);
  });

  // Build WiFi sniffer Menu
  wifiSnifferMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiSnifferMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiSnifferMenu.parentMenu, true);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[42], TFTCYAN, NULL, PROBE_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PROBE, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[43], TFTMAGENTA, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_AP, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[44], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH, TFT_RED);
  });
  this->addNodes(&wifiSnifferMenu, "Packet Count", TFTORANGE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
    wifi_scan_obj.renderPacketRate();
  });
  #ifdef HAS_ILI9341
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, NULL, EAPOL, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, NULL, PACKET_MONITOR, [this]() {
      wifi_scan_obj.StartScan(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
  #else // No touch
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, NULL, EAPOL, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, NULL, PACKET_MONITOR, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
  #endif
  this->addNodes(&wifiSnifferMenu, "Channel Analyzer", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    this->renderGraphUI(WIFI_SCAN_CHAN_ANALYZER);
    wifi_scan_obj.StartScan(WIFI_SCAN_CHAN_ANALYZER, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, "Channel Summary", TFTORANGE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    this->renderGraphUI(WIFI_SCAN_CHAN_ACT);
    wifi_scan_obj.StartScan(WIFI_SCAN_CHAN_ACT, TFT_CYAN);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[58], TFTWHITE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_RAW_CAPTURE, TFT_WHITE);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[47], TFTRED, NULL, PWNAGOTCHI, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PWN, TFT_RED);
  });
  
  this->addNodes(&wifiSnifferMenu, text_table1[63], TFTYELLOW, NULL, PINESCAN_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PINESCAN, TFT_YELLOW);
  });

  this->addNodes(&wifiSnifferMenu, text_table1[64], TFTORANGE, NULL, MULTISSID_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_MULTISSID, TFT_ORANGE);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[49], TFTMAGENTA, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, "Scan AP/STA", TFTLIME, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_AP_STA, 0x97e0);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[59], TFTORANGE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_STATION, TFT_WHITE);
  });
  this->addNodes(&wifiSnifferMenu, "Signal Monitor", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SIG_STREN, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, "MAC Monitor", TFTMAGENTA, NULL, SCANNERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_DETECT_FOLLOW, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, "SAE Commit", TFTLIME, NULL, EAPOL, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SAE_COMMIT, TFT_GREEN);
  });

  // Build Wardriving menu
  #ifdef HAS_GPS
    wardrivingMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
    this->addNodes(&wardrivingMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wardrivingMenu.parentMenu, true);
    });
    if (gps_obj.getGpsModuleStatus()) {
      this->addNodes(&wardrivingMenu, "Wardrive", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(WIFI_SCAN_WAR_DRIVE, TFT_GREEN);
      });
    }
  #endif
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      this->addNodes(&wardrivingMenu, "Station Wardrive", TFTORANGE, NULL, PROBE_SNIFF, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(WIFI_SCAN_STATION_WAR_DRIVE, TFT_ORANGE);
      });
    }
  #endif

  // Build WiFi attack menu
  wifiAttackMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiAttackMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiAttackMenu.parentMenu, true);
  });
  this->addNodes(&wifiAttackMenu, text_table1[50], TFTRED, NULL, BEACON_LIST, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_LIST, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, text_table1[51], TFTORANGE, NULL, BEACON_SPAM, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_SPAM, TFT_ORANGE);
  });
  this->addNodes(&wifiAttackMenu, text1_67, TFTCYAN, NULL, FUNNY_BEACON, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_FUNNY_BEACON, TFT_CYAN);
  });
  this->addNodes(&wifiAttackMenu, text_table1[52], TFTYELLOW, NULL, RICK_ROLL, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_RICK_ROLL, TFT_YELLOW);
  });
  this->addNodes(&wifiAttackMenu, text_table1[53], TFTRED, NULL, PROBE_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, "Evil Portal", TFTORANGE, NULL, BEACON_SNIFF, [this]() {

    wifiAPMenu.list->clear();
    ssidsMenu.list->clear();

    wifiAPMenu.parentMenu = &evilPortalMenu;
    ssidsMenu.parentMenu = &evilPortalMenu;

    this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiAPMenu.parentMenu, true);
    });
    this->addNodes(&ssidsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(ssidsMenu.parentMenu, true);
    });

    // Get AP list ready
    for (int i = 0; i < access_points->size(); i++) {
      // This is the menu node
      this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
        if (evil_portal_obj.setAP(access_points->get(i).essid)) {
          AccessPoint new_ap = access_points->get(i);
          new_ap.selected = true;
          access_points->set(i, new_ap);

          evil_portal_obj.ap_index = i;

          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&evilPortalMenu, true);
      });
    }

    for (int i = 0; i < ssids->size(); i++) {
      // This is the menu node
      this->addNodes(&ssidsMenu, ssids->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
        if (evil_portal_obj.setAP(ssids->get(i).essid)) {
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&evilPortalMenu, true);
      });
    }
    this->changeMenu(&evilPortalMenu, true);
  });
  this->addNodes(&wifiAttackMenu, text_table1[54], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, text_table1[57], TFTMAGENTA, NULL, BEACON_LIST, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_AP_SPAM, TFT_MAGENTA);
  });
  this->addNodes(&wifiAttackMenu, text_table1[62], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH_TARGETED, TFT_ORANGE);
  });

  this->addNodes(&wifiAttackMenu, "Karma", TFTORANGE, NULL, KEYBOARD_ICO, [this](){
    // Add the back button
    selectProbeSSIDsMenu.list->clear();
    this->addNodes(&selectProbeSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(&wifiAttackMenu, true);
    });

    // Populate the menu with buttons
    for (int i = 0; i < probe_req_ssids->size(); i++) {
      // This is the menu node
      this->addNodes(&selectProbeSSIDsMenu, probe_req_ssids->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
        if (evil_portal_obj.setAP(probe_req_ssids->get(i).essid)) {
          display_obj.clearScreen();
          this->drawStatusBar();
          wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
          wifi_scan_obj.setMac();
        }
        else
          this->changeMenu(&wifiAttackMenu, true);
      });
    }
    this->changeMenu(&selectProbeSSIDsMenu, true);
  });

  this->addNodes(&wifiAttackMenu, "Bad Msg", TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BAD_MSG, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, "Bad Msg Targeted", TFTYELLOW, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BAD_MSG_TARGETED, TFT_YELLOW);
  });
  this->addNodes(&wifiAttackMenu, "Assoc Sleep", TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_SLEEP, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, "Assoc Sleep Targ", TFTMAGENTA, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_SLEEP_TARGETED, TFT_MAGENTA);
  });
  this->addNodes(&wifiAttackMenu, "SAE Commit Flood", TFTLIME, NULL, EAPOL, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_SAE_COMMIT, TFT_GREEN);
  });

  evilPortalMenu.parentMenu = &wifiAttackMenu;
  this->addNodes(&evilPortalMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(evilPortalMenu.parentMenu, true);
  });
  this->addNodes(&evilPortalMenu, "Access Points", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
    this->changeMenu(&wifiAPMenu, true);
  });
  this->addNodes(&evilPortalMenu, "User SSIDs", TFTCYAN, NULL, PROBE_SNIFF, [this]() {
    this->changeMenu(&ssidsMenu, true);
  });

  // Build WiFi General menu
  wifiGeneralMenu.parentMenu = &wifiMenu;
  this->addNodes(&wifiGeneralMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiGeneralMenu.parentMenu, true);
  });
  this->addNodes(&wifiGeneralMenu, text_table1[27], TFTSKYBLUE, NULL, GENERATE, [this]() {
    this->changeMenu(&generateSSIDsMenu, true);
    wifi_scan_obj.RunGenerateSSIDs();
  });

	//Add Select probe ssid
  this->addNodes(&wifiGeneralMenu, text_table1[65], TFTCYAN, NULL, KEYBOARD_ICO, [this]() {
    selectProbeSSIDsMenu.list->clear();

    // Add the back button
    this->addNodes(&selectProbeSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(&wifiGeneralMenu, true);

      // TODO: TBD - Should probe_req_ssids have it´s own life and override ap.config and/or ssids -list for EP?
      // If so, then we should not add selected ssids to ssids list

      // Add selected ssid names to ssids list when clicking back button
      if (probe_req_ssids->size() > 0) {

        //TODO: TBD - Clear ssids list before adding new ones??

        for (int i = 0; i < probe_req_ssids->size(); i++) {
          ProbeReqSsid cur_probe_ssid = probe_req_ssids->get(i);
          if (cur_probe_ssid.selected) {
            bool ssidExists = false;
            for (int i = 0; i < ssids->size(); i++) {
              if (ssids->get(i).essid == cur_probe_ssid.essid) {
                ssidExists = true;
                break;
              }
            }
            if (!ssidExists) {
              wifi_scan_obj.addSSID(cur_probe_ssid.essid);
            }
          }
        }
      }
    });

    // Populate the menu with buttons
    for (int i = 0; i < probe_req_ssids->size(); i++) {
      ProbeReqSsid cur_ssid = probe_req_ssids->get(i);
      // This is the menu node
      this->addNodes(
        &selectProbeSSIDsMenu,
        "[" + String(cur_ssid.requests) + "]" + cur_ssid.essid,
        TFTCYAN,
        NULL,
        255,
        [this, i]() {
          ProbeReqSsid new_ssid = probe_req_ssids->get(i);
          new_ssid.selected = !probe_req_ssids->get(i).selected;

          // Change selection status of menu node
          MenuNode new_node = current_menu->list->get(i + 1);
          new_node.selected = !current_menu->list->get(i + 1).selected;
          current_menu->list->set(i + 1, new_node);

          probe_req_ssids->set(i, new_ssid);
        },
        probe_req_ssids->get(i).selected);
    }
    this->changeMenu(&selectProbeSSIDsMenu, true);
  });

  clearSSIDsMenu.parentMenu = &wifiGeneralMenu;

  #ifdef HAS_ILI9341
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      char ssidBuf[64] = {0};
      bool keep_going = true;
      while (keep_going) {
        display_obj.clearScreen(); 
        if (keyboardInput(ssidBuf, sizeof(ssidBuf), "Enter SSID")) {
          if (ssidBuf[0] != 0)
            wifi_scan_obj.addSSID(String(ssidBuf));
          for (int i = 0; i < 64; i++)
            ssidBuf[i] = NULL;
        }
        else
          keep_going = false;
      }

      this->changeMenu(current_menu);
    });
  #endif
  #if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      this->changeMenu(&miniKbMenu, true);
      #ifdef HAS_MINI_KB
        this->miniKeyboard(&miniKbMenu);
      #endif
    });
  #endif
  this->addNodes(&wifiGeneralMenu, text_table1[28], TFTSILVER, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearSSIDsMenu, true);
    wifi_scan_obj.RunClearSSIDs();
  });
  this->addNodes(&wifiGeneralMenu, text_table1[29], TFTDARKGREY, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearAPsMenu, true);
    wifi_scan_obj.RunClearAPs();
  });
  this->addNodes(&wifiGeneralMenu, text_table1[60], TFTBLUE, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearAPsMenu, true);
    wifi_scan_obj.RunClearStations();
  });
  //#else // Mini EP HTML select
    this->addNodes(&wifiGeneralMenu, "Select EP HTML File", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      // Add the back button
      htmlMenu.list->clear();
        this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(htmlMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < evil_portal_obj.html_files->size(); i++) {
        // This is the menu node
        this->addNodes(&htmlMenu, evil_portal_obj.html_files->get(i), TFTCYAN, NULL, 255, [this, i](){
          evil_portal_obj.selected_html_index = i;
          evil_portal_obj.target_html_name = evil_portal_obj.html_files->get(evil_portal_obj.selected_html_index);
          Serial.println("Set Evil Portal HTML as " + evil_portal_obj.target_html_name);
          evil_portal_obj.using_serial_html = false;
          this->changeMenu(htmlMenu.parentMenu, true);
          return;
        });
      }
      this->changeMenu(&htmlMenu, true);
    });

    //#if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
      miniKbMenu.parentMenu = &wifiGeneralMenu;
      #ifndef MARAUDER_CARDPUTER
        this->addNodes(&miniKbMenu, "a", TFTCYAN, NULL, 0, [this]() {
          this->changeMenu(miniKbMenu.parentMenu, true);
        });
      #endif
    //#endif

    htmlMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(htmlMenu.parentMenu, true);
    });

    // Select APs on Mini
    this->addNodes(&wifiGeneralMenu, "Select APs", TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      this->addNodes(&wifiAPMenu, "Select ALL", TFTGREEN, NULL, 255, [this](){

        for (int x = 0; x < access_points->size(); x++) {
          AccessPoint new_ap = access_points->get(x);
          new_ap.selected = !access_points->get(x).selected;
          access_points->set(x, new_ap);

          MenuNode new_node = current_menu->list->get(x + 2);
          new_node.selected = !current_menu->list->get(x + 2).selected;
          current_menu->list->set(x + 2, new_node);
        }

        this->changeMenu(current_menu, true);

      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
        AccessPoint new_ap = access_points->get(i);
        new_ap.selected = !access_points->get(i).selected;

        // Change selection status of menu node
        MenuNode new_node = current_menu->list->get(i + 2);
        new_node.selected = !current_menu->list->get(i + 2).selected;
        current_menu->list->set(i + 2, new_node);

        access_points->set(i, new_ap);
        }, access_points->get(i).selected);
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "View AP Info", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;
      
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
          this->changeMenu(&apInfoMenu, true);
          wifi_scan_obj.RunAPInfo(i);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    apInfoMenu.parentMenu = &wifiAPMenu;
    this->addNodes(&apInfoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(apInfoMenu.parentMenu, true);
    });

    wifiAPMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiAPMenu.parentMenu, true);
    });

    wifiIPMenu.parentMenu = &wifiScannerMenu;
    this->addNodes(&wifiIPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiIPMenu.parentMenu, true);
    });


    // Select Stations on Mini v2
    this->addNodes(&wifiGeneralMenu, "Select Stations", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      int menu_limit = access_points->size();


      for (int i = 0; i < menu_limit; i++) {
        wifiStationMenu.list->clear();
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){

          wifiStationMenu.list->clear();

          wifiStationMenu.parentMenu = &wifiAPMenu;

          // Add back button to the APs
          this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
            this->changeMenu(wifiStationMenu.parentMenu, true);
          });

          this->addNodes(&wifiStationMenu, "Select ALL", TFTGREEN, NULL, 255, [this, i](){

            for (int y = 0; y < access_points->get(i).stations->size(); y++) {
              int cur_ap_sta_inx = access_points->get(i).stations->get(y);
              Station new_sta = stations->get(cur_ap_sta_inx);
              new_sta.selected = !stations->get(cur_ap_sta_inx).selected;

              // Change selection status of menu node
              MenuNode new_node = current_menu->list->get(y + 2);
              new_node.selected = !current_menu->list->get(y + 2).selected;
              current_menu->list->set(y + 2, new_node);

              stations->set(cur_ap_sta_inx, new_sta);
            }

            this->changeMenu(current_menu, true);

          });

          // Add the AP's stations to the specific AP menu
          for (int x = 0; x < access_points->get(i).stations->size(); x++) {
            int cur_ap_sta = access_points->get(i).stations->get(x);

            this->addNodes(&wifiStationMenu, macToString(stations->get(cur_ap_sta)), TFTCYAN, NULL, 255, [this, i, cur_ap_sta, x](){
            Station new_sta = stations->get(cur_ap_sta);
            new_sta.selected = !stations->get(cur_ap_sta).selected;

            // Change selection status of menu node
            MenuNode new_node = current_menu->list->get(x + 2);
            new_node.selected = !current_menu->list->get(x + 2).selected;
            current_menu->list->set(x + 2, new_node);

            stations->set(cur_ap_sta, new_sta);
            }, stations->get(cur_ap_sta).selected);
          }

          // Final change menu to the menu of Stations
          this->changeMenu(&wifiStationMenu, true);
          
        }, false);
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "Join WiFi", TFTWHITE, NULL, KEYBOARD_ICO, [this](){

      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
          // Join WiFi using mini keyboard
          #ifdef HAS_MINI_KB
            this->changeMenu(&miniKbMenu, true);
            String password = this->miniKeyboard(&miniKbMenu, true);
            if (password != "") {
              Serial.println("Using SSID: " + (String)access_points->get(i).essid + " Password: " + (String)password);
              wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
              wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
              wifi_scan_obj.joinWiFi(access_points->get(i).essid, password);
              this->changeMenu(current_menu, true);
            }
          #endif

          // Join WiFi using touch screen keyboard
          #ifdef HAS_TOUCH
            char passwordBuf[64] = {0};  // or prefill with existing SSID
            if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
              wifi_scan_obj.joinWiFi(access_points->get(i).essid, String(passwordBuf), true);
            }

            this->changeMenu(&wifiGeneralMenu, false);
          #endif
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&wifiGeneralMenu, "Join Saved WiFi", TFTWHITE, NULL, KEYBOARD_ICO, [this](){
      String ssid = settings_obj.loadSetting<String>("ClientSSID");
      String pw = settings_obj.loadSetting<String>("ClientPW");

      if ((ssid != "") && (pw != "")) {
        wifi_scan_obj.joinWiFi(ssid, pw, false);
        this->changeMenu(&wifiGeneralMenu, true);
      }
      else {
        wifiAPMenu.parentMenu = &wifiGeneralMenu;

        // Add the back button
        wifiAPMenu.list->clear();
          this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
          this->changeMenu(wifiAPMenu.parentMenu, true);
        });

        // Populate the menu with buttons
        for (int i = 0; i < access_points->size(); i++) {
          // This is the menu node
          this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
            // Join WiFi using mini keyboard
            #ifdef HAS_MINI_KB
              this->changeMenu(&miniKbMenu, true);
              String password = this->miniKeyboard(&miniKbMenu, true);
              if (password != "") {
                Serial.println("Using SSID: " + (String)access_points->get(i).essid + " Password: " + (String)password);
                wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
                wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
                wifi_scan_obj.joinWiFi(access_points->get(i).essid, password);
                this->changeMenu(current_menu, true);
              }
            #endif

            // Join WiFi using touch screen keyboard
            #ifdef HAS_TOUCH
              char passwordBuf[64] = {0};  // or prefill with existing SSID
              if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
                wifi_scan_obj.joinWiFi(access_points->get(i).essid, String(passwordBuf), true);
              }

              this->changeMenu(&wifiGeneralMenu, false);
            #endif
          });
        }
        this->changeMenu(&wifiAPMenu, true);
      }
    });

    this->addNodes(&wifiGeneralMenu, "Start AP", TFTGREEN, NULL, KEYBOARD_ICO, [this](){
      ssidsMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      ssidsMenu.list->clear();
        this->addNodes(&ssidsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(ssidsMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < ssids->size(); i++) {
        // This is the menu node
        this->addNodes(&ssidsMenu, ssids->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
          // Join WiFi using mini keyboard
          #ifdef HAS_MINI_KB
            this->changeMenu(&miniKbMenu, true);
            String password = this->miniKeyboard(&miniKbMenu, true);
            if (password != "") {
              Serial.println("Using SSID: " + (String)ssids->get(i).essid + " Password: " + (String)password);
              wifi_scan_obj.currentScanMode = LV_JOIN_WIFI;
              wifi_scan_obj.StartScan(LV_JOIN_WIFI, TFT_YELLOW); 
              wifi_scan_obj.startWiFi(ssids->get(i).essid, password);
              this->changeMenu(current_menu, true);
            }
          #endif

          // Join WiFi using touch screen keyboard
          #ifdef HAS_TOUCH
            char passwordBuf[64] = {0};  // or prefill with existing SSID
            if (keyboardInput(passwordBuf, sizeof(passwordBuf), "Enter Password")) {
              Serial.println("Using SSID: " + (String)ssids->get(i).essid + " Password: " + String(passwordBuf));
              wifi_scan_obj.startWiFi(ssids->get(i).essid, String(passwordBuf));
            }

            this->changeMenu(&wifiGeneralMenu, false);
          #endif
        });
      }
      this->changeMenu(&ssidsMenu, true);
    });

    wifiStationMenu.parentMenu = &ssidsMenu;
    this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiStationMenu.parentMenu, true);
    });

  this->addNodes(&wifiGeneralMenu, "Set MACs", TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(&setMacMenu, true);
  });

  this->addNodes(&wifiGeneralMenu, "Shutdown WiFi", TFTRED, NULL, 0, [this]() {
    WiFi.disconnect(true);
    delay(100);
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF, TFT_RED);
    this->changeMenu(current_menu, true);
  });


  // Menu for generating and setting MAC addrs for AP and STA
  setMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&setMacMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(setMacMenu.parentMenu, true);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate AP MAC", TFTLIME, NULL, 0, [this]() {
    this->changeMenu(&genAPMacMenu, true);
    wifi_scan_obj.RunGenerateRandomMac(true);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate STA MAC", TFTCYAN, NULL, 0, [this]() {
    this->changeMenu(&genAPMacMenu, true);
    wifi_scan_obj.RunGenerateRandomMac(false);
  });

  // Clone AP MAC to ESP32 for button folks
  //#ifndef HAS_ILI9341
    this->addNodes(&setMacMenu, "Clone AP MAC", TFTRED, NULL, CLEAR_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTLIME, NULL, 255, [this, i](){
          this->changeMenu(&genAPMacMenu, true);
          wifi_scan_obj.RunSetMac(access_points->get(i).bssid, true);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });

    this->addNodes(&setMacMenu, "Clone STA MAC", TFTMAGENTA, NULL, CLEAR_ICO, [this](){
      wifiAPMenu.parentMenu = &wifiGeneralMenu;

      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });

      // Populate the menu with buttons
      for (int i = 0; i < stations->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, macToString(stations->get(i).mac), TFTMAGENTA, NULL, 255, [this, i](){
          this->changeMenu(&genAPMacMenu, true);
          wifi_scan_obj.RunSetMac(stations->get(i).mac, false);
        });
      }
      this->changeMenu(&wifiAPMenu, true);
    });
  //#endif

  // Menu for generating and setting access point MAC (just goes bacK)
  genAPMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&genAPMacMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(genAPMacMenu.parentMenu, true);
  });

  // Build generate ssids menu
  generateSSIDsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&generateSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(generateSSIDsMenu.parentMenu, true);
  });

  // Build clear ssids menu
  
  this->addNodes(&clearSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(clearSSIDsMenu.parentMenu, true);
  });
  clearAPsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&clearAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(clearAPsMenu.parentMenu, true);
  });

  // Build Bluetooth Menu
  bluetoothMenu.parentMenu = &mainMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothMenu, text_table1[31], TFTYELLOW, NULL, SNIFFERS, [this]() {
    this->changeMenu(&bluetoothSnifferMenu, true);
  });
  this->addNodes(&bluetoothMenu, "Bluetooth Attacks", TFTRED, NULL, ATTACKS, [this]() {
    this->changeMenu(&bluetoothAttackMenu, true);
  });

  // Build bluetooth sniffer Menu
  bluetoothSnifferMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothSnifferMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothSnifferMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothSnifferMenu, text_table1[34], TFTGREEN, NULL, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_ALL, TFT_GREEN);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flipper Sniff", TFTORANGE, NULL, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLIPPER, TFT_ORANGE);
  });
  this->addNodes(&bluetoothSnifferMenu, "Airtag Sniff", TFTWHITE, NULL, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_AIRTAG, TFT_WHITE);
  });
  this->addNodes(&bluetoothSnifferMenu, "Airtag Monitor", TFTWHITE, NULL, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_AIRTAG_MON, TFT_WHITE);
  });
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      this->addNodes(&bluetoothSnifferMenu, "BT Wardrive", TFTCYAN, NULL, BLUETOOTH_SNIFF, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(BT_SCAN_WAR_DRIVE, TFT_GREEN);
      });
    }
  #endif
  this->addNodes(&bluetoothSnifferMenu, text_table1[35], TFTMAGENTA, NULL, CC_SKIMMERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS, TFT_MAGENTA);
  });
  this->addNodes(&bluetoothSnifferMenu, "Bluetooth Analyzer", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    this->renderGraphUI(BT_SCAN_ANALYZER);
    wifi_scan_obj.StartScan(BT_SCAN_ANALYZER, TFT_CYAN);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flock Sniff", TFTORANGE, NULL, FLOCK, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLOCK, TFT_ORANGE);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flock Wardrive", TFTCYAN, NULL, FLOCK, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLOCK_WARDRIVE, TFT_CYAN);
  });

  // Bluetooth Attack menu
  bluetoothAttackMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothAttackMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothAttackMenu.parentMenu, true);
  });
  this->addNodes(&bluetoothAttackMenu, "Sour Apple", TFTGREEN, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SOUR_APPLE, TFT_GREEN);
  });
  this->addNodes(&bluetoothAttackMenu, "Swiftpair Spam", TFTCYAN, NULL, KEYBOARD_ICO, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SWIFTPAIR_SPAM, TFT_CYAN);
  });
  this->addNodes(&bluetoothAttackMenu, "Samsung BLE Spam", TFTRED, NULL, GENERAL_APPS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SAMSUNG_SPAM, TFT_RED);
  });
  this->addNodes(&bluetoothAttackMenu, "Google BLE Spam", TFTPURPLE, NULL, LANGUAGE, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_GOOGLE_SPAM, TFT_PURPLE);
  });
  this->addNodes(&bluetoothAttackMenu, "Flipper BLE Spam", TFTORANGE, NULL, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_FLIPPER_SPAM, TFT_ORANGE);
  });
  this->addNodes(&bluetoothAttackMenu, "BLE Spam All", TFTMAGENTA, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SPAM_ALL, TFT_MAGENTA);
  });

  //#ifndef HAS_ILI9341
    #ifdef HAS_BT
    // Select Airtag on Mini
      this->addNodes(&bluetoothAttackMenu, "Spoof Airtag", TFTWHITE, NULL, ATTACKS, [this](){
          wifiAPMenu.parentMenu = &bluetoothAttackMenu;

          // Clear nodes and add back button
          wifiAPMenu.list->clear();
          this->addNodes(&wifiAPMenu, text09, TFT_LIGHTGREY, NULL, 0, [this]() {
          this->changeMenu(wifiAPMenu.parentMenu, true);
        });

        // Add buttons for all airtags
        // Find out how big our menu is going to be
        int menu_limit;
        if (airtags->size() <= BUTTON_ARRAY_LEN)
          menu_limit = airtags->size();
        else
          menu_limit = BUTTON_ARRAY_LEN;

        // Create the menu nodes for all of the list items
        for (int i = 0; i < menu_limit; i++) {
          this->addNodes(&wifiAPMenu, airtags->get(i).mac, TFTWHITE, NULL, BLUETOOTH, [this, i](){
            AirTag new_at = airtags->get(i);
            new_at.selected = true;

            airtags->set(i, new_at);

            // Set all other airtags to "Not Selected"
            for (int x = 0; x < airtags->size(); x++) {
              if (x != i) {
                AirTag new_atx = airtags->get(x);
                new_atx.selected = false;
                airtags->set(x, new_atx);
              }
            }

            // Start the spoof
            display_obj.clearScreen();
            this->drawStatusBar();
            wifi_scan_obj.StartScan(BT_SPOOF_AIRTAG, TFT_WHITE);

          });
        }
        this->changeMenu(&wifiAPMenu, true);
      });

      wifiAPMenu.parentMenu = &bluetoothAttackMenu;
      this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu, true);
      });
    #endif

  //#endif

  // Device menu
  deviceMenu.parentMenu = &mainMenu;
  this->addNodes(&deviceMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(deviceMenu.parentMenu, true);
  });

  #ifdef HAS_SD
    if (sd_obj.supported) {

      sdDeleteMenu.parentMenu = &deviceMenu;

      this->addNodes(&deviceMenu, "Update Firmware", TFTORANGE, NULL, SD_UPDATE, [this]() {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.tft.println("Loading...");

        // Clear menu and lists
        this->buildSDFileMenu(true);

        this->changeMenu(&sdDeleteMenu, true);
      });
    }
  #endif

  this->addNodes(&deviceMenu, "Save/Load Files", TFTCYAN, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveFileMenu, true);
  });

  this->addNodes(&deviceMenu, text_table1[17], TFTWHITE, NULL, DEVICE_INFO, [this]() {
    wifi_scan_obj.currentScanMode = SHOW_INFO;
    this->changeMenu(&infoMenu, true);
    wifi_scan_obj.RunInfo();
  });
  this->addNodes(&deviceMenu, text08, TFTNAVY, NULL, KEYBOARD_ICO, [this]() {
    this->changeMenu(&settingsMenu, true);
  });

  #ifdef HAS_SD
    if (sd_obj.supported) {

      sdDeleteMenu.parentMenu = &deviceMenu;

      this->addNodes(&deviceMenu, "Delete SD Files", TFTCYAN, NULL, SD_UPDATE, [this]() {
        display_obj.clearScreen();
        display_obj.tft.setTextWrap(false);
        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.tft.println("Loading...");

        // Clear menu and lists
        this->buildSDFileMenu();

        this->changeMenu(&sdDeleteMenu, true);
      });
    }
  #endif

  // Save Files Menu
  saveFileMenu.parentMenu = &deviceMenu;
  this->addNodes(&saveFileMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveFileMenu.parentMenu, true);
  });
  this->addNodes(&saveFileMenu, "Save SSIDs", TFTCYAN, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveSSIDsMenu, true);
    wifi_scan_obj.RunSaveSSIDList(true);
  });
  this->addNodes(&saveFileMenu, "Load SSIDs", TFTSKYBLUE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadSSIDsMenu, true);
    wifi_scan_obj.RunLoadSSIDList();
  });
  this->addNodes(&saveFileMenu, "Save APs", TFTNAVY, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu, true);
    wifi_scan_obj.RunSaveAPList();
  });
  this->addNodes(&saveFileMenu, "Load APs", TFTBLUE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu, true);
    wifi_scan_obj.RunLoadAPList();
  });
  this->addNodes(&saveFileMenu, "Save Airtags", TFTWHITE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu, true);
    wifi_scan_obj.RunSaveATList();
  });
  this->addNodes(&saveFileMenu, "Load Airtags", TFTWHITE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu, true);
    wifi_scan_obj.RunLoadATList();
  });

  saveSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveSSIDsMenu.parentMenu, true);
  });

  loadSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadSSIDsMenu.parentMenu, true);
  });

  saveAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveAPsMenu.parentMenu, true);
  });

  loadAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadAPsMenu.parentMenu, true);
  });

  saveATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveATsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveATsMenu.parentMenu, true);
  });

  loadATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadATsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadATsMenu.parentMenu, true);
  });

  // GPS Menu
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      gpsMenu.parentMenu = &mainMenu; // Main Menu is second menu parent

      this->addNodes(&gpsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(gpsMenu.parentMenu, true);
      });

      this->addNodes(&gpsMenu, "GPS Data", TFTRED, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_DATA;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_DATA, TFT_CYAN);
      });

      this->addNodes(&gpsMenu, "NMEA Stream", TFTORANGE, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_NMEA;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_NMEA, TFT_ORANGE);
      });

      this->addNodes(&gpsMenu, "GPS Tracker", TFTGREEN, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = GPS_TRACKER;
        this->changeMenu(&gpsInfoMenu, true);
        wifi_scan_obj.StartScan(GPS_TRACKER, TFT_CYAN);
      });

      this->addNodes(&gpsMenu, "GPS POI", TFTCYAN, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.StartScan(GPS_POI, TFT_CYAN);
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        this->changeMenu(&gpsPOIMenu, true);
      });

      // GPS POI Menu
      gpsPOIMenu.parentMenu = &gpsMenu;
      this->addNodes(&gpsPOIMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        wifi_scan_obj.currentScanMode = GPS_POI;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        this->changeMenu(gpsPOIMenu.parentMenu, true);
      });
      this->addNodes(&gpsPOIMenu, "Mark POI", TFTCYAN, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = GPS_POI;
        display_obj.tft.setCursor(0, TFT_HEIGHT / 2);
        display_obj.clearScreen();
        if (wifi_scan_obj.RunGPSInfo(true, false, true))
          display_obj.showCenterText("POI Logged", TFT_HEIGHT / 2);
        else
          display_obj.showCenterText("POI Log Failed", TFT_HEIGHT / 2);
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        delay(2000);
        this->changeMenu(&gpsPOIMenu, true);
      });

      // GPS Info Menu
      gpsInfoMenu.parentMenu = &gpsMenu;
      this->addNodes(&gpsInfoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        if(wifi_scan_obj.currentScanMode != GPS_TRACKER)
          wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        this->changeMenu(gpsInfoMenu.parentMenu, true);
      }); 
    }
  #endif

  // Settings menu
  // Device menu
  settingsMenu.parentMenu = &deviceMenu;
  this->addNodes(&settingsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    changeMenu(settingsMenu.parentMenu, true);
  });
  for (int i = 0; i < settings_obj.getNumberSettings(); i++) {
    if (this->callSetting(settings_obj.setting_index_to_name(i)) == "bool")
      this->addNodes(&settingsMenu, settings_obj.setting_index_to_name(i), TFTLIGHTGREY, NULL, 0, [this, i]() {
      settings_obj.toggleSetting(settings_obj.setting_index_to_name(i));
      this->changeMenu(&specSettingMenu, true);
      this->displaySetting(settings_obj.setting_index_to_name(i), &settingsMenu, i + 1);
      wifi_scan_obj.force_pmkid = settings_obj.loadSetting<bool>(text_table4[5]);
      wifi_scan_obj.force_probe = settings_obj.loadSetting<bool>(text_table4[6]);
      wifi_scan_obj.save_pcap = settings_obj.loadSetting<bool>(text_table4[7]);
      wifi_scan_obj.ep_deauth = settings_obj.loadSetting<bool>("EPDeauth");
    }, settings_obj.loadSetting<bool>(settings_obj.setting_index_to_name(i)));
  }

  // Specific setting menu
  specSettingMenu.parentMenu = &settingsMenu;
  addNodes(&specSettingMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(specSettingMenu.parentMenu, true);
  });

  // Web Update
  updateMenu.parentMenu = &deviceMenu;

  // Failed update menu
  failedUpdateMenu.parentMenu = &deviceMenu;
  this->addNodes(&failedUpdateMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(failedUpdateMenu.parentMenu, true);
  });

  // Device info menu
  infoMenu.parentMenu = &deviceMenu;
  this->addNodes(&infoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(infoMenu.parentMenu, true);
  });

  // Set the current menu to the mainMenu
  this->changeMenu(&mainMenu, true);

  this->initTime = millis();
}

//#if (!defined(HAS_ILI9341) && defined(HAS_BUTTONS))
#ifdef HAS_MINI_KB
  String MenuFunctions::miniKeyboard(Menu * targetMenu, bool do_pass) {
    // Prepare a char array and reset temp SSID string
    extern LinkedList<ssid>* ssids;

    String ret_val = "";

    bool pressed = true;

    wifi_scan_obj.current_mini_kb_ssid = "";

    #ifdef HAS_MINI_KB
      if (c_btn.isHeld()) {
        while (!c_btn.justReleased())
          delay(1);
      }
    #endif

    int str_len = wifi_scan_obj.alfa.length() + 1; 

    char char_array[str_len];

    wifi_scan_obj.alfa.toCharArray(char_array, str_len);

    #ifdef HAS_TOUCH
      uint16_t t_x = 0, t_y = 0;

    #endif

    // Button loop until hold center button
    #ifdef HAS_BUTTONS
      //#if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1) || defined(MARAUDER_CYD_MICRO))
        while(true) {
          // Keyboard functions for switch hardware
          #ifdef HAS_MINI_KB
            // Cycle char previous
            #ifdef HAS_L
              if ((l_btn.justPressed()) || (l_btn.isHeld())) {
                pressed = true;
                if (this->mini_kb_index > 0)
                  this->mini_kb_index--;
                else
                  this->mini_kb_index = str_len - 2;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                this->buildButtons(targetMenu);

                while (!l_btn.justReleased()) {
                  l_btn.justPressed();
                  if (!l_btn.isHeld())
                    delay(1);
                  else
                    break;
                }
              }
            #endif

            // Cycle char next
            #ifdef HAS_R
              if ((r_btn.justPressed()) || (r_btn.isHeld())) {
                pressed = true;
                if (this->mini_kb_index < str_len - 2)
                  this->mini_kb_index++;
                else
                  this->mini_kb_index = 0;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
                
                while (!r_btn.justReleased()) {
                  r_btn.justPressed();
                  if (!r_btn.isHeld())
                    delay(1);
                  else
                    break;
                }
              }
            #endif

            //// 5-WAY SWITCH STUFF
            // Add character
            #if (defined(HAS_D) && defined(HAS_R))
              if (d_btn.justPressed()) {
                pressed = true;
                wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                while (!d_btn.justReleased())
                  delay(1);
              }
            #endif

            // Remove character
            #if (defined(HAS_U) && defined(HAS_L))
              if (u_btn.justPressed()) {
                pressed = true;
                wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                while (!u_btn.justReleased())
                  delay(1);
              }
            #endif

            //// PARTIAL SWITCH STUFF
            // Advance char or add char
            #if (defined(HAS_D) && !defined(HAS_R))
              if (d_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!d_btn.justReleased()) {
                  d_btn.justPressed();

                  // Add letter to string
                  if (d_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index < str_len - 2)
                    this->mini_kb_index++;
                  else
                    this->mini_kb_index = 0;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                  this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
                }
              }
            #endif

            // Prev char or remove char
            #if (defined(HAS_U) && !defined(HAS_L))
              if (u_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!u_btn.justReleased()) {
                  u_btn.justPressed();

                  // Remove letter from string
                  if (u_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index > 0)
                    this->mini_kb_index--;
                  else
                    this->mini_kb_index = str_len - 2;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                  this->buildButtons(targetMenu);
                }
              }
            #endif

            // Add SSID
            #ifdef HAS_C && !defined(MARAUDER_CARDPUTER)
              if (c_btn.justPressed()) {
                while (!c_btn.justReleased()) {
                  c_btn.justPressed(); // Need to continue updating button hold status. My shitty library.

                  // Exit
                  if (c_btn.isHeld()) {
                    this->changeMenu(targetMenu->parentMenu);
                    return wifi_scan_obj.current_mini_kb_ssid;
                  }
                  delay(1);
                }

                if (!do_pass) {
                // If we have a string, add it to list of SSIDs
                  if (wifi_scan_obj.current_mini_kb_ssid != "") {
                    pressed = true;
                    ssid s = {wifi_scan_obj.current_mini_kb_ssid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
                    ssids->unshift(s);
                    wifi_scan_obj.current_mini_kb_ssid = "";
                  }
                }
              }
            #endif
          #endif

          #ifdef MARAUDER_CARDPUTER
            for (int i = 0; i < 95; i++) {
              if ((M5CardputerKeyboard._ascii_list[i] != '(') &&
                  (M5CardputerKeyboard._ascii_list[i] != '`')) {
                if (this->isKeyPressed(M5CardputerKeyboard._ascii_list[i])) {
                  pressed = true;
                  wifi_scan_obj.current_mini_kb_ssid.concat(M5CardputerKeyboard._ascii_list[i]);
                }
                if (this->isKeyPressed(KEY_BACKSPACE)) {
                  pressed = true;
                  wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                }
              }
            }

            if (!do_pass) {
              if (this->isKeyPressed('`')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return wifi_scan_obj.current_mini_kb_ssid;
              }

              if (this->isKeyPressed('(')) {
                if (!do_pass) {
                  if (wifi_scan_obj.current_mini_kb_ssid != "") {
                    pressed = true;
                    ssid s = {wifi_scan_obj.current_mini_kb_ssid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
                    ssids->unshift(s);
                    wifi_scan_obj.current_mini_kb_ssid = "";
                  }
                }
              }
            }
            else {
              if (this->isKeyPressed('(')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return wifi_scan_obj.current_mini_kb_ssid;
              }

              if (this->isKeyPressed('`')) {
                this->changeMenu(targetMenu->parentMenu, true);
                return "";
              }
            }
            
          #endif

          // Keyboard functions for touch hardware
          #ifdef HAS_TOUCH
            bool touched = display_obj.updateTouch(&t_x, &t_y);

            uint8_t menu_button = display_obj.menuButton(&t_x, &t_y, touched);

            // Cycle char previous
            if (menu_button == UP_BUTTON) {
              pressed = true;
              if (this->mini_kb_index > 0)
                this->mini_kb_index--;
              else
                this->mini_kb_index = str_len - 2;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
              this->buildButtons(targetMenu);
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            // Cycle char next
            if (menu_button == DOWN_BUTTON) {
              pressed = true;
              if (this->mini_kb_index < str_len - 2)
                this->mini_kb_index++;
              else
                this->mini_kb_index = 0;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
              this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            //// 5-WAY SWITCH STUFF
            // Add character when select button is pressed
            if (menu_button == SELECT_BUTTON) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
              while (display_obj.updateTouch(&t_x, &t_y) > 0)
                delay(1);
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
            }

            // Remove character when select button is held
            if ((display_obj.isTouchHeld()) && (display_obj.menuButton(&t_x, &t_y, touched, true) == SELECT_BUTTON)) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
              while (display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y)) < 0)
                delay(1);
            }

            //// PARTIAL SWITCH STUFF
            // Advance char or add char
            #if (defined(HAS_D) && !defined(HAS_R))
              if (d_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!d_btn.justReleased()) {
                  d_btn.justPressed();

                  // Add letter to string
                  if (d_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index < str_len - 2)
                    this->mini_kb_index++;
                  else
                    this->mini_kb_index = 0;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                  this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
                }
              }
            #endif

            // Prev char or remove char
            #if (defined(HAS_U) && !defined(HAS_L))
              if (u_btn.justPressed()) {
                bool was_held = false;
                pressed = true;
                while(!u_btn.justReleased()) {
                  u_btn.justPressed();

                  // Remove letter from string
                  if (u_btn.isHeld()) {
                    wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                    was_held = true;
                    break;
                  }
                }
                if (!was_held) {
                  if (this->mini_kb_index > 0)
                    this->mini_kb_index--;
                  else
                    this->mini_kb_index = str_len - 2;

                  targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                  this->buildButtons(targetMenu);
                }
              }
            #endif

            // Exit if UP button is held
            if ((display_obj.isTouchHeld()) && (display_obj.menuButton(&t_x, &t_y, touched, true) == UP_BUTTON)) {
              display_obj.clearScreen();
              while (display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y)) < 0)
                delay(1);

              // Reset the touch keys so we don't activate the keys when we go back
              display_obj.menuButton(&t_x, &t_y, display_obj.updateTouch(&t_x, &t_y));
              this->changeMenu(targetMenu->parentMenu, true);
              return wifi_scan_obj.current_mini_kb_ssid;
            }

            // If the screen is touched but none of the keys are used, don't refresh display
            if (menu_button < 0)
              pressed = false;

          #endif

          // Display info on screen
          if (pressed) {
            this->displayCurrentMenu();
            display_obj.tft.setTextWrap(false);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3 + TEXT_HEIGHT * 2, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
            display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
            display_obj.tft.println(wifi_scan_obj.current_mini_kb_ssid + "\n");
            display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);

            display_obj.tft.println(ssids->get(0).essid);

            display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
            #ifdef HAS_MINI_KB
              #ifndef MARAUDER_CARDPUTER
              display_obj.tft.println("U/D - Rem/Add Char");
              display_obj.tft.println("L/R - Prev/Nxt Char");
              #endif
              if (!do_pass) {
                #ifdef MARAUDER_CARDPUTER
                  display_obj.tft.println("Enter - Save");
                  display_obj.tft.println("Esc - Exit");
                #else
                  display_obj.tft.println("C - Save");
                  display_obj.tft.println("C(Hold) - Exit");
                #endif
              }
              else {
                #ifdef MARAUDER_CARDPUTER
                  display_obj.tft.println("Enter - Enter");
                #else
                  display_obj.tft.println("C(Hold) - Enter");
                #endif
              }
            #endif

            #ifdef HAS_TOUCH
              display_obj.tft.println("U/D - Prev/Nxt Char");
              display_obj.tft.println("C - Add Char");
              display_obj.tft.println("C(Hold) - Rem Char");
              display_obj.tft.println("U(Hold) - Enter");
            #endif
            pressed = false;
          }
        }
      //#endif
    #endif
  }
#endif

void MenuFunctions::setupSDFileList(bool update) {
  sd_obj.sd_files->clear();

  delete sd_obj.sd_files;

  sd_obj.sd_files = new LinkedList<String>();

  if (!update)
    sd_obj.listDirToLinkedList(sd_obj.sd_files);
  else
    sd_obj.listDirToLinkedList(sd_obj.sd_files, "/", ".bin");
}

void MenuFunctions::buildSDFileMenu(bool update) {
  this->setupSDFileList(update);

  sdDeleteMenu.list->clear();
  delete sdDeleteMenu.list;
  sdDeleteMenu.list = new LinkedList<MenuNode>();

  if (!update)
    sdDeleteMenu.name = "SD Files";
  else
    sdDeleteMenu.name = "Bin Files";

  this->addNodes(&sdDeleteMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(sdDeleteMenu.parentMenu, true);
  });

  if (!update) {
    for (int x = 0; x < sd_obj.sd_files->size(); x++) {
      this->addNodes(&sdDeleteMenu, sd_obj.sd_files->get(x), TFTCYAN, NULL, SD_UPDATE, [this, x]() {
        if (sd_obj.removeFile("/" + sd_obj.sd_files->get(x))) {
          Serial.println("Deleted /" + sd_obj.sd_files->get(x));
          display_obj.clearScreen();
          display_obj.tft.setTextWrap(false);
          display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
          display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
          display_obj.tft.println("Deleting /" + sd_obj.sd_files->get(x) + "...");
          this->buildSDFileMenu();
          this->changeMenu(&sdDeleteMenu, true);
        }
      });
    }
  }
  else {
    for (int x = 0; x < sd_obj.sd_files->size(); x++) {
      this->addNodes(&sdDeleteMenu, sd_obj.sd_files->get(x), TFTCYAN, NULL, SD_UPDATE, [this, x]() {
        wifi_scan_obj.currentScanMode = OTA_UPDATE;
        this->changeMenu(&failedUpdateMenu, true);
        sd_obj.runUpdate("/" + sd_obj.sd_files->get(x));
      });
    }
  }
}

// Function to show all MenuNodes in a Menu
void MenuFunctions::showMenuList(Menu * menu, int layer)
{
  // Iterate through all of the menu nodes in the menu
  for (uint8_t i = 0; i < menu->list->size(); i++)
  {
    // Depending on layer, indent
    for (uint8_t x = 0; x < layer * 4; x++)
      Serial.print(" ");
    Serial.print(F("Node: "));
    Serial.println(menu->list->get(i).name);
  }
  Serial.println();
}


// Function to add MenuNodes to a menu
void MenuFunctions::addNodes(Menu * menu, String name, uint8_t color, Menu * child, int place, std::function<void()> callable, bool selected, String command)
{
  TFT_eSPI_Button new_button;
  menu->list->add(MenuNode{name, false, color, place, &new_button, selected, callable});
}

void MenuFunctions::setGraphScale(float scale) {
  this->_graph_scale = scale;
}

float MenuFunctions::calculateGraphScale(uint8_t value) {
  if ((value * this->_graph_scale < GRAPH_VERT_LIM) && (value * this->_graph_scale > GRAPH_VERT_LIM * 0.75)) {
    return this->_graph_scale;  // No scaling needed if the value is within the limit
  }

  if (value < GRAPH_VERT_LIM)
    return 1.0;

  // Calculate the multiplier proportionally
  return (0.75 * GRAPH_VERT_LIM) / value;
}

float MenuFunctions::calculateGraphScale(int16_t value) {
  if ((value * this->_graph_scale < GRAPH_VERT_LIM) && (value * this->_graph_scale > GRAPH_VERT_LIM * 0.75)) {
    return this->_graph_scale;  // No scaling needed if the value is within the limit
  }

  if (value < GRAPH_VERT_LIM)
    return 1.0;

  // Calculate the multiplier proportionally
  return (0.75 * GRAPH_VERT_LIM) / value;
}

float MenuFunctions::graphScaleCheck(const int16_t array[TFT_WIDTH]) {
  int16_t maxValue = 0;

  // Iterate through the array to find the highest value
  for (int16_t i = 0; i < TFT_WIDTH; i++) {
    if (array[i] > maxValue) {
      maxValue = array[i];
    }
  }

  // If the highest value exceeds GRAPH_VERT_LIM, call calculateMultiplier
  if (maxValue > GRAPH_VERT_LIM) {
    return this->calculateGraphScale(maxValue);
  }

  // If the highest value does not exceed GRAPH_VERT_LIM, return 1.0
  return 1.0;
}

float MenuFunctions::graphScaleCheckSmall(const uint8_t array[CHAN_PER_PAGE]) {
  uint8_t maxValue = 0;

  // Iterate through the array to find the highest value
  for (uint8_t i = 0; i < CHAN_PER_PAGE; i++) {
    if (array[i] > maxValue) {
      maxValue = array[i];
    }
  }

  // If the highest value exceeds GRAPH_VERT_LIM, call calculateMultiplier
  if (maxValue > GRAPH_VERT_LIM) {
    return this->calculateGraphScale(maxValue);
  }

  // If the highest value does not exceed GRAPH_VERT_LIM, return 1.0
  return 1.0;
}

void MenuFunctions::drawMaxLine(int16_t value, uint16_t color) {
  display_obj.tft.drawLine(0, TFT_HEIGHT - (value * this->_graph_scale), TFT_WIDTH, TFT_HEIGHT - (value * this->_graph_scale), color);
  display_obj.tft.setCursor(0, TFT_HEIGHT - (value * this->_graph_scale));
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.println((String)(value / BASE_MULTIPLIER));
}

void MenuFunctions::drawMaxLine(uint8_t value, uint16_t color) {
  //display_obj.tft.drawLine(0, TFT_HEIGHT - (value * this->_graph_scale), TFT_WIDTH, TFT_HEIGHT - (value * this->_graph_scale), color);
  display_obj.tft.setCursor(0, TFT_HEIGHT - (value * this->_graph_scale));
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.println((String)value);
}

void MenuFunctions::drawGraphSmall(uint8_t *values) {
  uint8_t maxValue = 0;
  //(i + (CHAN_PER_PAGE * (this->activity_page - 1)))

  int bar_width = TFT_WIDTH / (CHAN_PER_PAGE * 2);
  //display_obj.tft.fillRect(0, TFT_HEIGHT / 2 + 1, TFT_WIDTH, (TFT_HEIGHT / 2) + 1, TFT_BLACK);

  #ifndef HAS_DUAL_BAND
    for (int i = 1; i < CHAN_PER_PAGE + 1; i++) {
      int targ_val = i + (CHAN_PER_PAGE * (wifi_scan_obj.activity_page - 1)) - 1;
      int x_mult = (i * 2) - 1;
      int x_coord = (TFT_WIDTH / (CHAN_PER_PAGE * 2)) * (x_mult - 1);

      if (values[targ_val] > maxValue) {
        maxValue = values[targ_val];
      }

      if (values[targ_val] * this->_graph_scale <= GRAPH_VERT_LIM) {
        display_obj.tft.fillRect(x_coord, TFT_HEIGHT / 2 + 1, bar_width, TFT_HEIGHT / 2 + 1, TFT_BLACK);
        display_obj.tft.fillRect(x_coord, TFT_HEIGHT - (values[targ_val] * this->_graph_scale), bar_width, values[targ_val] * this->_graph_scale, TFT_CYAN);
      }

      display_obj.tft.drawLine(x_coord - 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), x_coord - 2, TFT_HEIGHT, TFT_WHITE);
    }
  #else
    for (int i = 1; i < CHAN_PER_PAGE + 1; i++) {
      int targ_val = i + (CHAN_PER_PAGE * (wifi_scan_obj.activity_page - 1)) - 1;
      int x_mult = (i * 2) - 1;
      int x_coord = (TFT_WIDTH / (CHAN_PER_PAGE * 2)) * (x_mult - 1);

      if (values[targ_val] > maxValue) {
        maxValue = values[targ_val];
      }

      if (values[targ_val] * this->_graph_scale <= GRAPH_VERT_LIM) {
        display_obj.tft.fillRect(x_coord, TFT_HEIGHT / 2 + 1, bar_width, TFT_HEIGHT / 2 + 1, TFT_BLACK);
        display_obj.tft.fillRect(x_coord, TFT_HEIGHT - (values[targ_val] * this->_graph_scale), bar_width, values[targ_val] * this->_graph_scale, TFT_CYAN);
      }

      display_obj.tft.drawLine(x_coord - 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), x_coord - 2, TFT_HEIGHT, TFT_WHITE);
    }
  #endif

  this->drawMaxLine(maxValue, TFT_GREEN); // Draw max
}

void MenuFunctions::drawGraph(int16_t *values) {
  int16_t maxValue = 0;
  int total = 0;
  for (int i = TFT_WIDTH - 1; i >= 0; i--) {
    if (values[i] >= 0) {
      total = total + values[i];
      if (values[i] > maxValue) {
        maxValue = values[i];
      }
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - (values[i] * this->_graph_scale), TFT_CYAN);
    }
    else {
      int16_t ch_val = values[i] * -1;
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_RED);
      display_obj.tft.setCursor(i, TFT_HEIGHT - GRAPH_VERT_LIM);
      display_obj.tft.setTextColor(TFT_BLACK, TFT_RED);
      display_obj.tft.setTextSize(1);
      display_obj.tft.println((String)ch_val);
    }
  }

  this->drawMaxLine(maxValue, TFT_GREEN); // Draw max
  this->drawMaxLine((int16_t)(total / TFT_WIDTH), TFT_ORANGE); // Draw average
}

void MenuFunctions::renderGraphUI(uint8_t scan_mode) {
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (scan_mode == WIFI_SCAN_CHAN_ANALYZER)
    display_obj.tft.drawCentreString("Frames/" + (String)BANNER_TIME + "ms", TFT_WIDTH / 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  else if (scan_mode == BT_SCAN_ANALYZER)
    display_obj.tft.drawCentreString("BLE Beacons/" + (String)BANNER_TIME + "ms", TFT_WIDTH / 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  display_obj.tft.drawLine(0, TFT_HEIGHT - GRAPH_VERT_LIM - 1, TFT_WIDTH, TFT_HEIGHT - GRAPH_VERT_LIM - 1, TFT_WHITE);
  display_obj.tft.setCursor(0, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 8));
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.println("Max");
  display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  display_obj.tft.println("Average");
  display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
  if (scan_mode != BT_SCAN_ANALYZER)
    display_obj.tft.println("Channel Marker");
}

uint16_t MenuFunctions::getColor(uint16_t color) {
  if (color == TFTWHITE) return TFT_WHITE;
  else if (color == TFTCYAN) return TFT_CYAN;
  else if (color == TFTBLUE) return TFT_BLUE;
  else if (color == TFTRED) return TFT_RED;
  else if (color == TFTGREEN) return TFT_GREEN;
  else if (color == TFTGREY) return TFT_LIGHTGREY;
  else if (color == TFTGRAY) return TFT_LIGHTGREY;
  else if (color == TFTMAGENTA) return TFT_MAGENTA;
  else if (color == TFTVIOLET) return TFT_VIOLET;
  else if (color == TFTORANGE) return TFT_ORANGE;
  else if (color == TFTYELLOW) return TFT_YELLOW;
  else if (color == TFTLIGHTGREY) return TFT_LIGHTGREY;
  else if (color == TFTPURPLE) return TFT_PURPLE;
  else if (color == TFTNAVY) return TFT_NAVY;
  else if (color == TFTSILVER) return TFT_SILVER;
  else if (color == TFTDARKGREY) return TFT_DARKGREY;
  else if (color == TFTSKYBLUE) return TFT_SKYBLUE;
  else if (color == TFTLIME) return 0x97e0;
  else return color;
}

// Function to change menu
void MenuFunctions::changeMenu(Menu* menu, bool simple_change) {
  if (!simple_change) {
    display_obj.initScrollValues();
    display_obj.setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
    display_obj.init();
  }
  current_menu = menu;

  current_menu->selected = 0;

  buildButtons(menu);

  displayCurrentMenu();

  //#ifdef MARAUDER_V8
  //  digitalWrite(TFT_BL, HIGH);
  //#endif
}

void MenuFunctions::buildButtons(Menu *menu, int starting_index, String button_name) {
  if (menu->list == NULL || menu->list->size() == 0)
      return;

  // Ensure starting index is within bounds
  if (starting_index >= menu->list->size())
    starting_index = menu->list->size() - BUTTON_SCREEN_LIMIT;
  if (starting_index < 0)
    starting_index = 0;

  this->menu_start_index = starting_index;

  // Determine the number of buttons to display (limited to screen capacity)
  uint8_t visible_buttons = min(BUTTON_SCREEN_LIMIT, menu->list->size() - starting_index);

  // Loop through and create only the visible buttons
  for (uint8_t i = 0; i < visible_buttons; i++) {
    uint16_t color = this->getColor(menu->list->get(starting_index + i).color);
    
    char buf[menu->list->get(starting_index + i).name.length() + 1] = {};
    if (button_name != "")
      menu->list->get(starting_index + i).name.toCharArray(buf, menu->list->get(starting_index + i).name.length() + 1);
    else
      button_name.toCharArray(buf, button_name.length() + 1);

    if (i >= BUTTON_SCREEN_LIMIT) {
      break;
    }

    display_obj.key[i].initButton(&display_obj.tft,
                                  KEY_X + 0 * (KEY_W + KEY_SPACING_X),
                                  KEY_Y + i * (KEY_H + KEY_SPACING_Y), // Positioning buttons vertically
                                  KEY_W,
                                  KEY_H,
                                  TFT_BLACK, // Outline
                                  TFT_BLACK, // Fill
                                  color, // Text color
                                  buf,
                                  KEY_TEXTSIZE);

    
    display_obj.key[i].setLabelDatum(BUTTON_PADDING - (KEY_W / 2), 2, ML_DATUM);

  }

  for (int i = BUTTON_ARRAY_LEN; i < BUTTON_ARRAY_LEN + 3; i++) {
    uint16_t x = TFT_WIDTH / 2;
    uint16_t y = TFT_HEIGHT / 3 * (i - BUTTON_ARRAY_LEN) + ((TFT_HEIGHT / 3) / 2);
    uint16_t w = TFT_WIDTH;
    uint16_t h = TFT_HEIGHT / 3 - 1;

    display_obj.key[i].initButton(&display_obj.tft,
                                  x,
                                  y, // Positioning buttons vertically
                                  w,
                                  h,
                                  TFT_LIGHTGREY, // Outline
                                  TFT_BLACK, // Fill
                                  TFT_BLACK, // Text color
                                  "Chicken",
                                  1);
  }
}

void MenuFunctions::displayCurrentMenu(int start_index)
{
  //Serial.println(F("Displaying current menu..."));
  display_obj.clearScreen();
  display_obj.updateBanner(current_menu->name);
  display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
  this->drawStatusBar();

  if (current_menu->list != NULL)
  {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.setFreeFont(MENU_FONT);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
    #endif

    for (uint16_t i = start_index; i < min(start_index + BUTTON_SCREEN_LIMIT, current_menu->list->size()); i++)
    {
      if (!current_menu || !current_menu->list || i >= current_menu->list->size())
        continue;
      uint16_t color = this->getColor(current_menu->list->get(i).color);
      #ifdef HAS_FULL_SCREEN
        if ((current_menu->list->get(i).selected) || (current_menu->selected == i)) {
          display_obj.key[i - start_index].drawButton(true, current_menu->list->get(i).name);
        }
        else {
          display_obj.key[i - start_index].drawButton(false, current_menu->list->get(i).name);          
        }
        
        if ((current_menu->list->get(i).name != text09) && (current_menu->list->get(i).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(i).icon],
                                      ICON_W,
                                      ICON_H,
                                      TFT_BLACK,
                                      color);

      #endif

      #ifdef HAS_MINI_SCREEN
        if ((current_menu->selected == i) || (current_menu->list->get(i).selected))
          display_obj.key[i - start_index].drawButton(true, current_menu->list->get(i).name);
        else 
          display_obj.key[i - start_index].drawButton(false, current_menu->list->get(i).name);
      #endif
    }
    display_obj.tft.setFreeFont(NULL);
  }

  this->displayMenuButtons();
}

#endif

