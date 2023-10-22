#ifndef GpsInterface_h
#define GpsInterface_h

#include <MicroNMEA.h>
#include "configs.h"

//SoftwareSerial causes compliation issues with ESP32-S2FN4R2 mini dev boards, but ESP doesn't have a custom IDF target for them.
// using extra preprocessor directives here to prevent meddling with other ESP32-S2's like the flipper wifi devboard. 
#ifndef ESP32_S2_MINI_MARAUDER_FLIPPER
  #include <SoftwareSerial.h>
#endif

class GpsInterface {
  public:
    void begin();
    void main();

    String getNumSatsString();
    bool getFixStatus();
    String getFixStatusAsString();
    bool getGpsModuleStatus();
    String getLat();
    String getLon();
    float getAlt();
    float getAccuracy();
    String getDatetime();

  private:
    // GPS Info
    String lat = "";
    String lon = "";
    float altf = 0.0;
    float accuracy = 0.0;
    String datetime = "";
    
    bool gps_enabled = false;
    bool good_fix = false;
    uint8_t num_sats = 0;

    String dt_string_from_gps();
    void setGPSInfo();
};

#endif