// Reproduced issue https://github.com/mathieucarbou/ESPAsyncWebServer/issues/26

#include <DNSServer.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"

DNSServer dnsServer;
AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
  public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(__unused AsyncWebServerRequest* request) {
      // request->addInterestingHeader("ANY");
      return true;
    }

    void handleRequest(AsyncWebServerRequest* request) {
      AsyncResponseStream* response = request->beginResponseStream("text/html");
      response->print("<!DOCTYPE html><html><head><title>Captive Portal</title></head><body>");
      response->print("<p>This is out captive portal front page.</p>");
      response->printf("<p>You were trying to reach: http://%s%s</p>", request->host().c_str(), request->url().c_str());
      response->printf("<p>Try opening <a href='http://%s'>this link</a> instead</p>", WiFi.softAPIP().toString().c_str());
      response->print("</body></html>");
      request->send(response);
    }
};

bool hit1 = false;
bool hit2 = false;

void setup() {
  Serial.begin(115200);

  server
    .on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      Serial.println("Captive portal request...");
      Serial.println("WiFi.localIP(): " + WiFi.localIP().toString());
      Serial.println("request->client()->localIP(): " + request->client()->localIP().toString());
#if ESP_IDF_VERSION_MAJOR >= 5
      Serial.println("WiFi.type(): " + String((int)WiFi.localIP().type()));
      Serial.println("request->client()->type(): " + String((int)request->client()->localIP().type()));
#endif
      Serial.println(WiFi.localIP() == request->client()->localIP() ? "should be: ON_STA_FILTER" : "should be: ON_AP_FILTER");
      Serial.println(WiFi.localIP() == request->client()->localIP());
      Serial.println(WiFi.localIP().toString() == request->client()->localIP().toString());
      request->send(200, "text/plain", "This is the captive portal");
      hit1 = true;
    })
    .setFilter(ON_AP_FILTER);

  server
    .on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      Serial.println("Website request...");
      Serial.println("WiFi.localIP(): " + WiFi.localIP().toString());
      Serial.println("request->client()->localIP(): " + request->client()->localIP().toString());
#if ESP_IDF_VERSION_MAJOR >= 5
      Serial.println("WiFi.type(): " + String((int)WiFi.localIP().type()));
      Serial.println("request->client()->type(): " + String((int)request->client()->localIP().type()));
#endif
      Serial.println(WiFi.localIP() == request->client()->localIP() ? "should be: ON_STA_FILTER" : "should be: ON_AP_FILTER");
      Serial.println(WiFi.localIP() == request->client()->localIP());
      Serial.println(WiFi.localIP().toString() == request->client()->localIP().toString());
      request->send(200, "text/plain", "This is the website");
      hit2 = true;
    })
    .setFilter(ON_STA_FILTER);

  // assert(WiFi.softAP("esp-captive-portal"));
  // dnsServer.start(53, "*", WiFi.softAPIP());
  // server.begin();
  // Serial.println("Captive portal started!");

  // while (!hit1) {
  //   dnsServer.processNextRequest();
  //   yield();
  // }
  // delay(1000); // Wait for the client to process the response

  // Serial.println("Captive portal opened, stopping it and connecting to WiFi...");
  // dnsServer.stop();
  // WiFi.softAPdisconnect();

  WiFi.persistent(false);
  WiFi.begin("IoT");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi with IP address: " + WiFi.localIP().toString());
  server.begin();

  // while (!hit2) {
  //   delay(10);
  // }
  // delay(1000); // Wait for the client to process the response
  // ESP.restart();
}

void loop() {
}
