#pragma once

#ifndef CliBusy_h
#define CliBusy_h

namespace marauder {

// Why a scan/attack CLI command cannot start right now.
enum class CliBusyReason {
  WifiConnected,  // joined to an AP (scan mode WIFI_CONNECTED)
  ShowInfo,       // "info" left the scan mode at SHOW_INFO
  ScanRunning,    // any other active scan mode, including the NMEA stream
};

// True for commands that only run while no scan is active (the
// `if (!wifi_scan_obj.scanning())` block in CommandLine::runCommand()).
bool cliIsScanGatedCommand(const char* command);

// One-line reply for a gated command that was refused.
const char* cliBusyMessage(CliBusyReason reason);

}  // namespace marauder

#endif
