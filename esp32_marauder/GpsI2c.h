#pragma once

#ifndef GpsI2c_h
#define GpsI2c_h

#include "configs.h"

#ifdef HAS_GPSI2C

#include <Wire.h>
#include "DFRobot_GNSS.h"
// #define GPS_I2C_ADDR 0x75
#include <time.h>

#if !defined(GPS_SDA) && defined(I2C_SDA)
  #define GPS_SDA I2C_SDA
  #define GPS_SCL I2C_SCL
#endif



/*
  I2C Errors
    0: success.
    1: data too long to fit in transmit buffer.
    2: received NACK on transmit of address.
    3: received NACK on transmit of data.
    4: other error.
    5: timeout
*/

class GpsI2c {
 public:
    TwoWire *_wire;
    DFRobot_GNSS_I2C gnss;

    void begin(int sdaPin, int sclPin, uint32_t frequency = 400000);

    void begin(TwoWire *wireInstance = &Wire);

    // void begin();
    void main(uint32_t current = 0, uint16_t scanMode = 1);

    int getNumSats() { return num_sats; }

    String getNumSatsString() { return (String)num_sats; }
    bool getFixStatus() { return this->good_fix; }
    String getFixStatusAsString() { return this->good_fix ? "Yes" : "No"; }
    bool getGpsModuleStatus() { return this->gps_enabled; }
    String getLat() { return this->lat; }
    String getLon() { return this->lon; }
    int32_t getLatInt() { return this->lat_int; }
    int32_t getLonInt() { return this->lon_int; }
    float getAccuracy() { return (float) 2.5; }
    float getAlt() { return this->altf; }
    String getDatetime() { return this->dt_string_from_gps(); }
    String getText() { return this->gps_text; }
    // int getTextQueueSize() { return -2; }
    // String getTextQueue(bool flush=1);
    // String getNmea();
    // String getNmeaNotimp();
    // String getNmeaNotparsed();

    void setType(String t);
    void setType(uint8_t);
    String generateType();

    // void enqueue(MicroNMEA& nmea);
    // LinkedList<nmea_sentence_t>* get_queue();
    void flush_queue() { log_d("flush_queue"); }
    void flush_text() { log_d("flush_text"); }
    void new_queue() { log_d("new_queue"); }
    void enable_queue() { log_d("enable_queue"); }
    void disable_queue() { log_d("disable_queue"); }
    bool queue_enabled() {  log_d("queue_enabled"); return false;}

    void sendSentence(const char* sentence) {}
    void sendSentence(Stream &s, const char* sentence) {}

    String generateGXgga() { return "generateGXgga"; }
    String generateGXrmc() { return "";}

    void setGPSInfo();
    void SHowGPSInfo();

    struct tm GetTimeInfo();

    //    ======  Legacy compatibility ======
    bool queue_enabled_flag = 0;
    // LinkedList<nmea_sentence_t> *queue=NULL;

    String GetGNSSType();

    unsigned int text_cycles = 0;
    // LinkedList<String> *text_in=NULL;
    // LinkedList<String> *text=NULL;

    void flush_queue_text();
    void flush_queue_textin();
    void flush_queue_nmea();
    String dt_string_from_gps();

    String nmea_sentence = "";
    String notimp_nmea_sentence = "";
    String notparsed_nmea_sentence = "";

    bool probeBaud(uint32_t baud) { return true; }
    void setGpsTo115200From9600() {}
    uint32_t initGpsBaudAndForce115200() {return 115200;}

    bool gps_enabled = false;

 private:
    eGnssMode_t type_flag = eGPS;

    // GPS Info
    String gps_text = "";

    String lat = "";
    String lon = "";
    int32_t lat_int = 0;
    int32_t lon_int = 0;
    sLonLat_t lat_dat = {0};
    sLonLat_t lon_dat = {0};
    float altf = 0.0;
    // float accuracy = 2.5;
    String datetime = "";

    bool good_fix = false;
    // char nav_system = '\0';
    String nav_system = "";
    uint8_t num_sats = 0;
};

#endif    //  HAS_GPSI2C
#endif    // GPSI2C_h
