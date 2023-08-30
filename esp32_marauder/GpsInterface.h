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
    bool gps_enabled = false;
    bool good_fix = false;
    uint8_t num_sats = 0;
    //MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

    void showGPSInfo();
};

#endif