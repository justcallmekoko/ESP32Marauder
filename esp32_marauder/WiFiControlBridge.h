#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "CommandLine.h"

extern CommandLine cli_obj;

class WiFiControlBridge {
  public:
    void begin(const char* ap_ssid, const char* ap_pass) {
      WiFi.softAP(ap_ssid, ap_pass);
      Serial.print("Control AP IP: ");
      Serial.println(WiFi.softAPIP());

      server = new AsyncWebServer(8081);

      server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html",
          "<html><body style='font-family:monospace;background:#111;color:#0f0'>"
          "<h3>Marauder Remote CLI</h3>"
          "<form action='/cmd' method='POST'>"
          "<input name='c' style='width:80%' autofocus placeholder='e.g. scanall'>"
          "<input type='submit' value='Send'>"
          "</form></body></html>");
      });

      server->on("/cmd", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("c", true)) {
          String cmd = request->getParam("c", true)->value();
          cli_obj.runCommand(cmd);
        }
        request->redirect("/");
      });

      server->begin();
    }

  private:
    AsyncWebServer* server;
};

extern WiFiControlBridge wifi_control;