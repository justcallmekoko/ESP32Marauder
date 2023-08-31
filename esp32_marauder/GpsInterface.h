#ifndef GpsInterface_h
#define GpsInterface_h

#include <MicroNMEA.h>

#include "configs.h"

class GpsInterface {
  public:
    void begin();
    void main();

    String getNumSatsString();
    bool getFixStatus();
    bool getGpsModuleStatus();

  private:
    // GPS Info
    String lat = "";
    String lon = "";
    float altf = 0.0;
    uint8_t gps_year = 0;
    uint8_t gps_month = 0;
    uint8_t gps_day = 0;
    uint8_t gps_hour = 0;
    uint8_t gps_minute = 0;
    uint8_t gps_second = 0;
    uint8_t gps_hundredths = 0;
    String datetime = "";
    
    bool gps_enabled = false;
    bool good_fix = false;
    uint8_t num_sats = 0;
    //MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

    String dt_string_from_gps();
    void showGPSInfo();
    void setGPSInfo();
};

#endif