#include "CliBusy.h"

#include <string.h>

namespace marauder {

namespace {

// Keep in sync with the scan-gated block in CommandLine::runCommand().
// tools/test_cli_busy_commands.py checks this list against that block.
const char* const kScanGatedCommands[] = {
  "attack",      "brightness",  "findmy",      "sniffskim",
  "sniffbt",     "blespam",     "spoofat",     "csimotion",
  "evilportal",  "geofence",    "karma",       "randapmac",
  "randstamac",  "cloneapmac",  "clonestamac", "mactrack",
  "packetcount", "remoteid",    "scanall",     "foxhunt",
  "sniffbeacon", "sniffdeauth", "sniffmultissid", "sniffpinescan",
  "sniffpmkid",  "sniffprobe",  "sniffpwn",    "sniffraw",
  "sniffsae",    "update",      "wardrivepoi", "wardrive",
};

}  // namespace

bool cliIsScanGatedCommand(const char* command) {
  if (command == nullptr) return false;
  for (const char* gated : kScanGatedCommands) {
    if (strcmp(command, gated) == 0) return true;
  }
  return false;
}

const char* cliBusyMessage(CliBusyReason reason) {
  switch (reason) {
    case CliBusyReason::WifiConnected:
      return "WiFi is connected. Run stopscan -f first.";
    case CliBusyReason::ShowInfo:
      return "Run stopscan before starting a scan.";
    case CliBusyReason::ScanRunning:
    default:
      return "A scan is already running. Run stopscan first.";
  }
}

}  // namespace marauder
