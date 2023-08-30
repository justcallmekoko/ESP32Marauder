#ifndef GpsInterface_h
#define GpsInterface_h

#include <MicroNMEA.h>

class GpsInterface {
  public:
    void begin();
    void main();

    String getNumSatsString();
    bool getFixStatus();

  private:
    bool good_fix = false;
    uint8_t num_sats = 0;
    //MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

    void showGPSInfo();
};

#endif