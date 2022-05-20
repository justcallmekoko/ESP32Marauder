#ifndef CommandLine_h
#define CommandLine_h

#include "configs.h"

#ifdef HAS_SCREEN
  #include "MenuFunctions.h"
  #include "Display.h"
#endif 

#include "WiFiScan.h"

#ifdef HAS_SCREEN
  extern MenuFunctions menu_function_obj;
  extern Display display_obj;
#endif

extern WiFiScan wifi_scan_obj;

// Commands
const char PROGMEM SCANAP_CMD[] = "scanap";
const char PROGMEM SNIFF_BEACON_CMD[] = "sniffbeacon";
const char PROGMEM SNIFF_DEAUTH_CMD[] = "sniffdeauth";
const char PROGMEM STOPSCAN_CMD[] = "stopscan";
const char PROGMEM CLEARAP_CMD[] = "clearap";

class CommandLine {
  private:
    String getSerialInput();
    void parseCommand(String input);
        
  public:
    CommandLine();

    void RunSetup();
    void main(uint32_t currentTime);
};

#endif
