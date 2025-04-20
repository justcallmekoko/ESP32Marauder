#include <Arduino.h>
#include <DNSServer.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "StreamConcat.h"
#include "StreamString.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

DNSServer dnsServer;
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  LittleFS.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
  dnsServer.start(53, "*", WiFi.softAPIP());

  File file1 = LittleFS.open("/header.html", "w");
  file1.print("<html><head><title>ESP Captive Portal</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>");
  file1.close();

  File file2 = LittleFS.open("/body.html", "w");
  file2.print("<h1>Welcome to ESP Captive Portal</h1>");
  file2.close();

  File file3 = LittleFS.open("/footer.html", "w");
  file3.print("</body></html>");
  file3.close();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    File header = LittleFS.open("/header.html", "r");
    File body = LittleFS.open("/body.html", "r");
    StreamConcat stream1(&header, &body);

    StreamString content;
    content.printf("FreeHeap: %" PRIu32, ESP.getFreeHeap());
    StreamConcat stream2 = StreamConcat(&stream1, &content);

    File footer = LittleFS.open("/footer.html", "r");
    StreamConcat stream3 = StreamConcat(&stream2, &footer);

    request->send(stream3, "text/html", stream3.available());
    header.close();
    body.close();
    footer.close();
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
}

uint32_t last = 0;

void loop() {
  // dnsServer.processNextRequest();

  if (millis() - last > 2000) {
    Serial.printf("FreeHeap: %" PRIu32, ESP.getFreeHeap());
    last = millis();
  }
}