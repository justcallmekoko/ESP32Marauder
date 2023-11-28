#pragma once

#ifndef GpsInterface_h
#define GpsInterface_h

#include <MicroNMEA.h>
#include <SoftwareSerial.h>
#include <LinkedList.h>

#include "configs.h"

void gps_nmea_notimp(MicroNMEA& nmea);

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
    String getNmea();
    String getNmeaNotimp();

    void setType(String t);

    void enqueue(MicroNMEA& nmea);
    LinkedList<String>* get_queue();
    void flush_queue();
    void new_queue();
    void enable_queue();
    void disable_queue();
    bool queue_enabled();

    void sendSentence(const char* sentence);
    void sendSentence(Stream &s, const char* sentence);

    String generateGXgga();
    String generateGXrmc();

    enum type_t {
      GPSTYPE_ALL,
      GPSTYPE_GPS,
      GPSTYPE_GLONASS,
      GPSTYPE_GALILEO
    };

  private:
    // GPS Info
    String nmea_sentence = "";
    String notimp_nmea_sentence = "";
    String lat = "";
    String lon = "";
    float altf = 0.0;
    float accuracy = 0.0;
    String datetime = "";
    
    bool gps_enabled = false;
    bool good_fix = false;
    uint8_t num_sats = 0;

    type_t type_flag = GPSTYPE_ALL;

    bool queue_enabled_flag=0;
    LinkedList<String> *queue=NULL;

    String dt_string_from_gps();
    void setGPSInfo();
};

#endif