#include "CommandLine.h"

CommandLine::CommandLine() {
}

void CommandLine::RunSetup() {
  
}

String CommandLine::getSerialInput() {
  String input = "";

  if (Serial.available() > 0)
    input = Serial.readStringUntil('\n');

  return input;
}

void CommandLine::main(uint32_t currentTime) {
   this->parseCommand(this->getSerialInput());
}

void CommandLine::parseCommand(String input) {
  if (input != "")
    Serial.println("#" + input);

  if (input == STOPSCAN_CMD) {
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

    // If we don't do this, the text and button coordinates will be off
    #ifdef HAS_SCREEN
      display_obj.tft.init();
      menu_function_obj.changeMenu(menu_function_obj.current_menu);
    #endif
  }
    
  else if (input == SCANAP_CMD) {
    #ifdef HAS_SCREEN
      display_obj.clearScreen();
      menu_function_obj.drawStatusBar();
    #endif
    wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP, TFT_MAGENTA);
  }

  else if (input == CLEARAP_CMD) {
    wifi_scan_obj.RunClearAPs();
  }

  else if (input == SNIFF_BEACON_CMD) {
    #ifdef HAS_SCREEN
      display_obj.clearScreen();
      menu_function_obj.drawStatusBar();
    #endif
    wifi_scan_obj.StartScan(WIFI_SCAN_AP, TFT_MAGENTA);
  }

  else if (input == SNIFF_DEAUTH_CMD) {
    #ifdef HAS_SCREEN
      display_obj.clearScreen();
      menu_function_obj.drawStatusBar();
    #endif
    wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH, TFT_RED);
  }
}
