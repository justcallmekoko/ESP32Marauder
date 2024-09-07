#pragma once

#ifndef CalebGPSInterface_h
#define CalebGPSInterface_h

#include <TinyGPS++.h>
#include <HardwareSerial.h>

class CalebGPSInterface {
  public:
    CalebGPSInterface();  // Constructor
    void begin();         // Initialize GPS
    void updateGPS();     // Read GPS data
    String getLatitude(); // Get Latitude as String
    String getLongitude();// Get Longitude as String
    float getAltitude();  // Get Altitude in meters
    bool isGPSFixValid(); // Check if GPS fix is valid
    int getNumSatellites();// Get number of satellites

  private:
    TinyGPSPlus gps;      // TinyGPS++ object
    HardwareSerial Serial2;  // HardwareSerial for GPS communication
};

#endif