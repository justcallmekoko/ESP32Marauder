
#include "GpsI2c.h"
// #include <format>
// #include "ESP32_PinDebug.h"

//   https://wiki.dfrobot.com/tel0157/#tech_specs
//   https://github.com/DFRobot/DFRobot_GNSS

// gps_obj.generateGXgga();
//     String gxrmc = gps_obj.generateGXrmc();

#ifdef HAS_GPSI2C

#include <Arduino.h>

#ifndef HAS_GPSI2C_ADDR
  #define HAS_GPSI2C_ADDR 0x20
#endif

extern GpsI2c  gps_obj;
extern bool system_time_set;
extern bool set_system_time(struct tm, bool setrtc = false);

// char nmeaBuffer[100];


// 72H ID
// 1BH Bat Volt
// 16H temperature

uint32_t lastq;

void GpsI2c::begin(int sdaPin, int sclPin, uint32_t frequency) {
  // Serial.print("GpsI2c::begin  ADDR="); Serial.print(GNSS_DEVICE_ADDR);
  log_d("GpsI2c");

  Wire.begin(sdaPin, sclPin);
  // Wire.begin(I2C_SDA, I2C_SCL);

  begin(&Wire);
}

void GpsI2c::begin(TwoWire *wireInstance) {
  _wire = wireInstance;

  delay(500);
  // describeAllPins();
  // Serial.print("GNSS_DEVICE_ADDR=0x"); Serial.println(GNSS_DEVICE_ADDR, HEX);
  // Serial.print("HAS_GPSI2C_ADDR=0x"); Serial.println(HAS_GPSI2C_ADDR, HEX);

  // (&Wire, HAS_GPSI2C_ADDR);
  DFRobot_GNSS_I2C gnssu = DFRobot_GNSS_I2C(_wire, GNSS_DEVICE_ADDR);

  lastq = millis();

  // retry GPS connection but give up after 5+ sec...
  while (!(this->gps_enabled = gnss.begin())) {
     log_d("GpsI2c::begin fail");
     Serial.println("GpsI2c::begin Fail");
     // describeAllPins();
     if ( (millis() - lastq) > 4400)
       return;
     delay(1050);
  }

  this->gps_enabled = true;
  log_d("GpsI2c::begin gps_enabled=%d", this->gps_enabled);

  // Serial.print("GpsI2c::begin gps_enabled="); Serial.println(this->gps_enabled);

  gnss.enablePower();      // Enable gnss power

  // Set GNSS to be used
  //   eGPS              use gps
  //   eBeiDou           use beidou
  //   eGPS_BeiDou       use gps + beidou
  //   eGLONASS          use glonass
  //   eGPS_GLONASS      use gps + glonass
  //   eBeiDou_GLONASS   use beidou +glonass
  //   eGPS_BeiDou_GLONASS use gps + beidou + glonass
  //
  // gnss.setGnss(eGPS_BeiDou_GLONASS);
  gnss.getAllGnss();

  gnss.setRgbOn();

  lastq = millis();
}



/*
*   eGPS              use gps
*   eBeiDou           use beidou
*   eGPS_BeiDou       use gps + beidou
*   eGLONASS          use glonass
*   eGPS_GLONASS      use gps + glonass
*   eBeiDou_GLONASS   use beidou +glonass
*   eGPS_BeiDou_GLONASS use gps + beidou + glonass
*/

void GpsI2c::setType(uint8_t t) {
log_d("setType(i) called");
  if (t > 0 && t < 8)
      gnss.setGnss((eGnssMode_t) t);
}

void GpsI2c::setType(String t) {
log_d("setType(S) called");

    if (t ==  "gps") gnss.setGnss(eGPS);
    else if (t ==  "beidou") gnss.setGnss(eBeiDou);
    else if (t ==  "gps + beidou") gnss.setGnss(eGPS_BeiDou);
    else if (t ==  "glonass") gnss.setGnss(eGLONASS);
    else if (t ==  "gps + glonass") gnss.setGnss(eGPS_GLONASS);
    else if (t ==  "beidou + glonass") gnss.setGnss(eBeiDou_GLONASS);
    else if (t ==  "gps + beidou + glonass") gnss.setGnss(eGPS_BeiDou_GLONASS);
}


// GpsInterface Compatibility
// Honestly, I'm guessing at this part
String GpsI2c::generateType() {
  log_d("generateType called");

  /*
  switch (this->nav_system) {
    case eGPS: return "P";
    case eBeiDou: return "B";
    case eGPS_BeiDou: return "BD";
    case eGLONASS: return "L";
    case eGPS_GLONASS: return "LD";
    case eBeiDou_GLONASS: return "BL";   // ????
    case eGPS_BeiDou_GLONASS: return "N";
    default: return "N";
  };
  */
  return "N";
}


String GpsI2c::GetGNSSType() {
  log_d("GetGNSSType called");
  /*
  switch (this->nav_system) {
    case eGPS: return "gps";
    case eBeiDou: return "beidou";
    case eGPS_BeiDou: return "gps + beidou";
    case eGLONASS: return "glonass";
    case eGPS_GLONASS: return "gps + glonass";
    case eBeiDou_GLONASS: return "beidou + glonass";
    case eGPS_BeiDou_GLONASS: return "gps + beidou + glonass";
    default: return "??";
  };
  */
  return "gps + beidou + glonass";
}

struct tm GpsI2c::GetTimeInfo() {
  struct tm timeInfo = {0};
  sTim_t utc = gnss.getUTC();
  sTim_t date = gnss.getDate();

  if (date.year > 0) {
    timeInfo.tm_year = date.year - 1900;
    timeInfo.tm_mon = date.month -1;
    timeInfo.tm_mday = date.date;
    timeInfo.tm_hour = utc.hour;
    timeInfo.tm_min = utc.minute;
    timeInfo.tm_sec = utc.second;
    // timeInfo.tm_gmtoff = 0;   //  "UTC"
  }

  return timeInfo;
}

String GpsI2c::dt_string_from_gps() {
  //Return a datetime String using GPS data only.
  String datetime = "";

  sTim_t utc = gnss.getUTC();
  sTim_t date = gnss.getDate();

  datetime += date.year;
  datetime += "-";
  datetime += date.month;
  datetime += "-";
  datetime += date.date;
  datetime += " ";
  datetime += utc.hour;
  datetime += ":";
  datetime += utc.minute;
  datetime += ":";
  datetime += utc.second;

  return datetime;
}


void GpsI2c::setGPSInfo() {
  String nmea_sentence = String("");

  this->num_sats = gnss.getNumSatUsed();

  if (num_sats <= 2) {
    log_d("num_sats = %d",  num_sats);
    return;
  }

  if (!good_fix && num_sats >= 3)
    log_d("GPS fix extablished");

  this->good_fix = (num_sats >= 3) ? true : false;
  this->nav_system = gnss.getGnssMode();

  lat_dat = gnss.getLat();
  lon_dat = gnss.getLon();

  this->datetime = this->dt_string_from_gps();

  this->lat_int = static_cast<int32_t>(lat_dat.latitudeDegree * 1000000);
  this->lon_int = static_cast<int32_t>(lon_dat.lonitudeDegree * 1000000);

  this->lat = String(lat_dat.latitudeDegree, 6);
  this->lon = String(lat_dat.latitudeDegree, 6);

  this->altf = static_cast<float>(gnss.getAlt());

  // if GPS has a good_fix and system_time_set has not been set
  if (!system_time_set) {
    // Serial.print("datetime");  Serial.println(this->datetime);
    log_d("set_system_time(GetTimeInfo())");
    struct tm timeInfo = GetTimeInfo();
    Serial.println(&timeInfo, "%F %T");
    set_system_time(timeInfo);
  }

  // this->accuracy = 2.5;  // ????
}

void GpsI2c::SHowGPSInfo() {
  Serial.print("NumSatUsed    = "); Serial.println(this->num_sats);
  Serial.print("good_fix      = "); Serial.println(this->good_fix);
  Serial.print("nav_system    = "); Serial.println(this->nav_system);

  Serial.print("datetime      = "); Serial.println(this->datetime);

  Serial.print("lat_int       = "); Serial.println(this->lat_int);
  Serial.print("lon_int       = "); Serial.println(this->lon_int);

  Serial.print("lat           = "); Serial.println(this->lat);
  Serial.print("lon           = "); Serial.println(this->lon);

  Serial.print("altf          = "); Serial.println(this->altf);
}



// int GpsI2c::getNumSats() { return num_sats; }


// String GpsI2c::getFixStatusAsString() { return this->good_fix ? "Yes" : "No"; }

// bool GpsI2c::getGpsModuleStatus() { return this->gps_enabled; }


// int GpsI2c::getTextQueueSize() { return -2; }


// String GpsI2c::getNmea() {
//   return this->nmea_sentence;
// }

// String GpsI2c::getNmeaNotimp() {
//   return this->notimp_nmea_sentence;
// }

// String GpsI2c::getNmeaNotparsed() {
//   return this->notparsed_nmea_sentence;
// }


void GpsI2c::main(uint32_t current, uint16_t scanMode) {
  //  if ( (current - lastq) < (400 * (wifi_scan_obj.currentScanMode ? 1 : 20 )))
  //  if ( (current - lastq) < (400 * (scanMode ? 1 : 20 )))
  if ((current - lastq) < 400) {
    return;
  }

  if (!this->gps_enabled) {
    return;
  }

  this->setGPSInfo();
  lastq = millis();
}


#endif   // HAS_GPSI2C
