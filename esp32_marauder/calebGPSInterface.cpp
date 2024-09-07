#include "calebGPSInterface.h"

// Constructor
CalebGPSInterface::CalebGPSInterface() : Serial2(1) {  // Use Serial2 as default
}

// Initialize the GPS module
void CalebGPSInterface::begin() {
    Serial2.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);  // Start communication with GPS module
    Serial.println("GPS Module Initialized");
}

// Update GPS data by reading from the serial buffer
void CalebGPSInterface::updateGPS() {
    while (Serial2.available()) {
        char c = Serial2.read();
        gps.encode(c);  // Process GPS data with TinyGPS++
    }
}

// Get Latitude as a string
String CalebGPSInterface::getLatitude() {
    if (gps.location.isValid()) {
        return String(gps.location.lat(), 6);  // Return latitude
    } else {
        return "Invalid";
    }
}

// Get Longitude as a string
String CalebGPSInterface::getLongitude() {
    if (gps.location.isValid()) {
        return String(gps.location.lng(), 6);  // Return longitude
    } else {
        return "Invalid";
    }
}

// Get Altitude in meters
float CalebGPSInterface::getAltitude() {
    if (gps.altitude.isValid()) {
        return gps.altitude.meters();  // Return altitude in meters
    } else {
        return 0.0;
    }
}

// Check if GPS fix is valid
bool CalebGPSInterface::isGPSFixValid() {
    return gps.location.isValid();  // Return true if GPS fix is valid
}

// Get the number of satellites
int CalebGPSInterface::getNumSatellites() {
    return gps.satellites.value();  // Return the number of satellites
}