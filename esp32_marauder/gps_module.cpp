#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include "gps_module.h"

// Initialize TinyGPS++ library and HardwareSerial 
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // UART1 for GPS (change pins if necessary)

//Initialize GPS

/*#ifdef MARAUDER_MINI
  pinMode(26, OUTPUT);

  delay(1);

  analogWrite(26, 243);
  delay(1);

  Serial.println("Activated GPS");
  delay(100);
#endif*/

void setupGPS () {
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // Adjust pins if needed
    Serial.println("GPS Module initialized");
}

// Function to read GPS data
void readGPS() {
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
        if (gps.location.isUpdated()) {
            Serial.print("Latitude: ");
            Serial.println(gps.location.lat(), 6);
            Serial.print("Longitude: ");
            Serial.println(gps.location.lng(), 6);
        }
    }
}