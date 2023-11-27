#include "GpsInterface.h"

#ifdef HAS_GPS

extern GpsInterface gps_obj;

char nmeaBuffer[100];

MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

#ifndef GPS_SOFTWARE_SERIAL
  HardwareSerial Serial2(GPS_SERIAL_INDEX);
#else
  EspSoftwareSerial::UART Serial2;
#endif

void GpsInterface::begin() {

  #ifndef GPS_SOFTWARE_SERIAL
    Serial2.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
  #else
    Serial2.begin(9600, SWSERIAL_8N1, GPS_TX, GPS_RX);
  #endif

  MicroNMEA::sendSentence(Serial2, "$PSTMSETPAR,1201,0x00000042");
  MicroNMEA::sendSentence(Serial2, "$PSTMSAVEPAR");

  MicroNMEA::sendSentence(Serial2, "$PSTMSRR");

  delay(4000);

  if (Serial2.available()) {
    Serial.println("GPS Attached Successfully");
    this->gps_enabled = true;
    while (Serial2.available())
      Serial2.read();
  }

  this->queue_enabled_flag=0;
  this->queue=NULL;
  this->new_queue();

  nmea.setUnknownSentenceHandler(gps_nmea_notimp);
}

//passthrough for other objects
void gps_nmea_notimp(MicroNMEA& nmea){
  gps_obj.enqueue(nmea);
}

void GpsInterface::enqueue(MicroNMEA& nmea){
  String nmea_sentence = String(nmea.getSentence());

  if(nmea_sentence != ""){
    this->notimp_nmea_sentence = nmea_sentence;

    if(this->queue_enabled_flag){
      if(!this->queue) this->new_queue();
      this->queue->add(String(nmea_sentence));
    }
  }
}

void GpsInterface::enable_queue(){
  this->flush_queue();
  this->queue_enabled_flag=1;
}

void GpsInterface::disable_queue(){
  this->queue_enabled_flag=0;
  this->flush_queue();
}

bool GpsInterface::queue_enabled(){
  return this->queue_enabled_flag;
}

LinkedList<String>* GpsInterface::get_queue(){
  return this->queue;
}

void GpsInterface::new_queue(){
  this->queue=new LinkedList<String>;
}

void GpsInterface::flush_queue(){
  if(this->queue) delete this->queue;
  this->new_queue();
}

void GpsInterface::sendSentence(const char* sentence){
  MicroNMEA::sendSentence(Serial2, sentence);
}

void GpsInterface::sendSentence(Stream &s, const char* sentence){
  MicroNMEA::sendSentence(s, sentence);
}

void GpsInterface::setType(String t){
  if(t == "gps")
    this->type_flag=GPSTYPE_GPS;
  else if(t == "glonass")
    this->type_flag=GPSTYPE_GLONASS;
  else if(t == "galileo")
    this->type_flag=GPSTYPE_GALILEO;
  else
    this->type_flag=GPSTYPE_ALL;
}

String GpsInterface::generateGXgga(){
  String msg_type="$G";
  if(this->type_flag == GPSTYPE_GPS)
    msg_type+='P';
  else if(this->type_flag == GPSTYPE_GLONASS)
    msg_type+='L';
  else if(this->type_flag == GPSTYPE_GALILEO)
    msg_type+='A';
  else
    msg_type+='N';
  msg_type+="GGA,";

  char timeStr[8];
  snprintf(timeStr, 8, "%02d%02d%02d,", (int)(nmea.getHour()), (int)(nmea.getMinute()), (int)(nmea.getSecond()));

  long lat = nmea.getLatitude();
  char latDir = lat < 0 ? 'S' : 'N';
  lat = abs(lat);
  char latStr[12];
  snprintf(latStr, 12, "%02ld%08.5f,", lat / 1000000, ((lat % 1000000)*60) / 1000000.0);

  long lon = nmea.getLongitude();
  char lonDir = lon < 0 ? 'W' : 'E';
  lon = abs(lon);
  char lonStr[13];
  snprintf(lonStr, 13, "%03ld%08.5f,", lon / 1000000, ((lon % 1000000)*60) / 1000000.0);

  int fixQuality = nmea.isValid() ? 1 : 0;
  char fixStr[3];
  snprintf(fixStr, 3, "%01d,", fixQuality);

  int numSatellites = nmea.getNumSatellites();
  char satStr[4];
  snprintf(satStr, 4, "%02d,", numSatellites);

  unsigned long hdop = nmea.getHDOP();
  char hdopStr[13];
  snprintf(hdopStr, 13, "%01.2f,", 2.5 * (((float)(hdop))/10));

  long altitude;
  if(!nmea.getAltitude(altitude)) altitude=0;
  char altStr[9];
  snprintf(altStr, 9, "%01.1f,", altitude/1000.0);

  String message = msg_type + timeStr + latStr + latDir + ',' + lonStr + lonDir +
                    ',' + fixStr + satStr + hdopStr + altStr + "M,,M,,";

  return message;
}

String GpsInterface::generateGXrmc(){
  String msg_type="$G";
  if(this->type_flag == GPSTYPE_GPS)
    msg_type+='P';
  else if(this->type_flag == GPSTYPE_GLONASS)
    msg_type+='L';
  else if(this->type_flag == GPSTYPE_GALILEO)
    msg_type+='A';
  else
    msg_type+='N';
  msg_type+="RMC,";

  char timeStr[8];
  snprintf(timeStr, 8, "%02d%02d%02d,", (int)(nmea.getHour()), (int)(nmea.getMinute()), (int)(nmea.getSecond()));

  char dateStr[8];
  snprintf(dateStr, 8, "%02d%02d%02d,", (int)(nmea.getDay()), (int)(nmea.getMonth()), (int)(nmea.getYear()%100));

  char status = nmea.isValid() ? 'A' : 'V';
  char mode = nmea.isValid() ? 'A' : 'N';

  long lat = nmea.getLatitude();
  char latDir = lat < 0 ? 'S' : 'N';
  lat = abs(lat);
  char latStr[12];
  snprintf(latStr, 12, "%02ld%08.5f,", lat / 1000000, ((lat % 1000000)*60) / 1000000.0);

  long lon = nmea.getLongitude();
  char lonDir = lon < 0 ? 'W' : 'E';
  lon = abs(lon);
  char lonStr[13];
  snprintf(lonStr, 13, "%03ld%08.5f,", lon / 1000000, ((lon % 1000000)*60) / 1000000.0);

  char speedStr[8];
  snprintf(speedStr, 8, "%01.1f,", nmea.getSpeed() / 1000.0);

  char courseStr[7];
  snprintf(courseStr, 7, "%01.1f,", nmea.getCourse() / 1000.0);

  String message = msg_type + timeStr + status + ',' + latStr + latDir + ',' +
                    lonStr + lonDir + ',' + speedStr + courseStr + dateStr + ',' + ',' + mode;
  return message;
}

// Thanks JosephHewitt
String GpsInterface::dt_string_from_gps(){
  //Return a datetime String using GPS data only.
  String datetime = "";
  if (nmea.isValid() && nmea.getYear() > 0){
    datetime += nmea.getYear();
    datetime += "-";
    datetime += nmea.getMonth();
    datetime += "-";
    datetime += nmea.getDay();
    datetime += " ";
    datetime += nmea.getHour();
    datetime += ":";
    datetime += nmea.getMinute();
    datetime += ":";
    datetime += nmea.getSecond();
  }
  return datetime;
}

void GpsInterface::setGPSInfo() {
  String nmea_sentence = String(nmea.getSentence());
  if(nmea_sentence != "") this->nmea_sentence = nmea_sentence;

  this->good_fix = nmea.isValid();
  this->num_sats = nmea.getNumSatellites();

  this->datetime = this->dt_string_from_gps();

  this->lat = String((float)nmea.getLatitude()/1000000, 7);
  this->lon = String((float)nmea.getLongitude()/1000000, 7);
  long alt = 0;
  if (!nmea.getAltitude(alt)){
    alt = 0;
  }
  this->altf = (float)alt / 1000;

  this->accuracy = 2.5 * ((float)nmea.getHDOP()/10);

  //nmea.clear();
}

float GpsInterface::getAccuracy() {
  return this->accuracy;
}

String GpsInterface::getLat() {
  return this->lat;
}

String GpsInterface::getLon() {
  return this->lon;
}

float GpsInterface::getAlt() {
  return this->altf;
}

String GpsInterface::getDatetime() {
  return this->datetime;
}

String GpsInterface::getNumSatsString() {
  return (String)num_sats;
}

bool GpsInterface::getFixStatus() {
  return this->good_fix;
}

String GpsInterface::getFixStatusAsString() {
  if (this->getFixStatus())
    return "Yes";
  else
    return "No";
}

bool GpsInterface::getGpsModuleStatus() {
  return this->gps_enabled;
}

String GpsInterface::getNmea() {
  return this->nmea_sentence;
}

String GpsInterface::getNmeaNotimp() {
  return this->notimp_nmea_sentence;
}

void GpsInterface::main() {
  while (Serial2.available()) {
    //Fetch the character one by one
    char c = Serial2.read();
    //Serial.print(c);
    //Pass the character to the library
    nmea.process(c);
  }

  uint8_t num_sat = nmea.getNumSatellites();

  if ((nmea.isValid()) && (num_sat > 0))
    this->setGPSInfo();

  else if ((!nmea.isValid()) && (num_sat <= 0)) {
    this->setGPSInfo();
  }
}
#endif