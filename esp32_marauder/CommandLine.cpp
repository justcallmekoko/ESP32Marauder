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
  String input = this->getSerialInput();

  this->parseCommand(input);
  this->runCommand(input);
}

void CommandLine::parseCommand(String input) {
  if (input != "") {
    char delim[] = " ";

    char fancy[input.length() + 1] = {};
    input.toCharArray(fancy, input.length() + 1);
        
    char* ptr = strtok(fancy, delim);
  
    while (ptr != NULL) {
      Serial.println(ptr);
  
      ptr = strtok(NULL, delim);
    }

    memset(fancy, 0, sizeof(fancy));
  }

  return;
}

void CommandLine::runCommand(String input) {
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

  if (input == SET_CH_CMD) {
    
  }

  if (!wifi_scan_obj.scanning()) {
    if (input == SCANAP_CMD) {
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
  
    else if (input == SNIFF_PMKID_CMD) {
      wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
    }
  }
}
