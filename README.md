# ESP32 Marauder — Headless Serial CLI Edition

Customized and optimized version of **ESP32 Marauder**, created by **Sanak3**, focused exclusively on direct **USB / Serial connection to a PC** (Headless mode / no screen).

All legacy code for displays, physical buttons, battery management, and external hardware modules has been removed, resulting in a lightweight, fast firmware with **optimized BLE advertising performance** and **high-speed Serial PCAP streaming (921600 baud)**.

---

## Key Improvements in This Edition

1. **High-Speed Serial Streaming (921600 Baud)**:
   - Eliminates buffer bottlenecks during raw packet sniffing (`sniffraw`, `sniffbeacon`).
   - Enables real-time PCAP streaming directly into Wireshark on your PC.
2. **Optimized BLE Spam (High Speed & No Crashes)**:
   - Eliminated continuous NimBLE stack teardown (`init`/`deinit`) per packet.
   - Advertising interval configured to the minimum specification standard (20ms).
   - Prevents memory leaks / heap exhaustion and sudden reboots.
3. **Global Attack/Scan Duration Support (`-d <seconds>`)**:
   - Universal parameter available across all Wi-Fi scans, sniffing modes, attacks, and BLE spam.
   - Run for a specified number of seconds or indefinitely until `stopscan` is sent.
4. **Structured JSON Output Mode & System Telemetry**:
   - `sysinfo [-j]`: Real-time telemetry (free RAM heap, CPU frequency, uptime, flash size).
   - `json [on/off]`: Toggle machine-readable JSON output for easy integration with Python, Node.js, and PC automation scripts.
   - `list -a -j` / `list -b -j`: Output discovered Access Points and BLE devices in JSON.
5. **Clean & 100% Headless Codebase**:
   - Free from display library dependencies (TFT_eSPI, touch drivers, etc.).
   - Directly boots into high-speed Serial CLI mode.

---

## How to Build and Flash

### Arduino IDE Configuration

1. Install the official **esp32** board package by Espressif in the *Boards Manager*.
2. Install the following libraries via the *Library Manager*:
   - `NimBLE-Arduino`
   - `ArduinoJson` (v6.x)
   - `ESP32Ping`
   - `AsyncTCP`
   - `ESPAsyncWebServer`
3. Open the `esp32_marauder/` folder and launch `esp32_marauder.ino`.
4. Configure the settings under **Tools**:
   - **Board**: `ESP32 Dev Module` (or your specific ESP32 board)
   - **Upload Speed**: `921600`
   - **CPU Frequency**: `240MHz`
   - **Flash Frequency**: `80MHz`
   - **Partition Scheme**: `Minimal SPIFFS (1.9MB APP / 190KB SPIFFS)` or `Default 4MB with SPIFFS`
   - **Port**: Select your ESP32 serial COM/TTY port
5. Click **Upload**.

---

## Serial Connection and Usage

Open your preferred **Serial Monitor** (Arduino IDE, PuTTY, minicom, screen, or any serial terminal):

* **Baud Rate**: `921600`
* **Line Ending**: `Newline (\n)` or `Both NL & CR (\r\n)`

Upon boot, the banner will be displayed:

```text
  __  __                               _           
 |  \/  | __ _ _ __ __ _ _   _  __| | ___ _ __     
 | |\/| |/ _` | '__/ _` | | | |/ _` |/ _ \ '__|    
 | |  | | (_| | | | (_| | |_| | (_| |  __/ |       
 |_|  |_|\__,_|_|  \__,_|\__,_|\__,_|\___|_|       
                                                   
     MARAUDER CUSTOM HEADLESS EDITION BY SANAK3    

=========================================================
       MARAUDER CUSTOM HEADLESS EDITION BY SANAK3        
            Version: v1.15.0 | Baud: 921600
            ESP-IDF: v4.4.7
=========================================================

Type 'help' for available commands, or 'sysinfo' for stats.
```

---

## CLI Command Reference

### Bluetooth Low Energy (BLE)

* **BLE Spam with Custom Duration**:
  ```bash
  blespam -t <type> [-d <seconds>]
  ```
  **Available Types**:
  - `applejuice`: Advertisements for iOS / Apple devices
  - `sourapple`: Alternative Apple action packets
  - `google`: Google Fast Pair
  - `samsung`: Samsung Galaxy Watch / Buds
  - `windows`: Microsoft SwiftPair
  - `flipper`: Flipper Zero
  - `all`: Continuous cycle across all payload types

  **Examples**:
  ```bash
  # Continuous transmission (indefinite) until 'stopscan' is entered
  blespam -t all

  # Transmit Samsung payloads for 30 seconds with auto-stop
  blespam -t samsung -d 30

  # Transmit Google Fast Pair for 60 seconds
  blespam -t google -d 60
  ```

* **Bluetooth Sniffing & Detection**:
  ```bash
  sniffbt -t <airtag/flipper/flock/meta> [-d <seconds>]
  sniffskim [-d <seconds>]
  ```

---

### Wi-Fi (Scanners & Sniffers)

* **Network and Station Discovery**:
  ```bash
  scanap [-d <seconds>]      # Scan for Access Points (e.g. scanap -d 15)
  scansta [-d <seconds>]     # Scan for connected Stations
  list -a [-j]               # List discovered APs (use -j for JSON output)
  list -c [-j]               # List discovered Clients
  list -b [-j]               # List discovered BLE devices
  select -a <id>             # Select target AP by index
  ```

* **Packet Capturing (Sniffing & PCAP Stream)**:
  ```bash
  sniffraw [-d <seconds>]    # Raw PCAP capture stream over Serial (921600 baud)
  sniffbeacon [-d <seconds>] # Sniff Beacon frames
  sniffprobe [-d <seconds>]  # Sniff Probe Requests
  sniffdeauth [-d <seconds>] # Sniff Deauthentication frames
  sniffpwn [-d <seconds>]    # Detect Pwnagotchi beacons
  ```

* **Wi-Fi Attacks and Packet Generation**:
  ```bash
  attack -t beacon -l [-d <seconds>]    # Beacon spam using SSID list
  attack -t beacon -r [-d <seconds>]    # Random SSID beacon spam
  attack -t deauth [-d <seconds>]       # Deauthentication frames against selected targets
  attack -t probe [-d <seconds>]        # Probe request spam
  attack -t rickroll [-d <seconds>]     # Rick Roll SSID beacon spam
  ```

---

### System Diagnostics & Automation

* `sysinfo [-j]`: Display CPU frequency, free RAM heap, uptime, and firmware telemetry (or in JSON format with `-j`).
* `json [on/off]`: Toggle persistent structured JSON output for automated scripting.
* `help`: Display complete command list and syntax.
* `channel <1-14>`: Set Wi-Fi channel manually.
* `settings`: View and modify persistent configuration in SPIFFS.
* `clearlist <ap/sta/ssids/ble/all>`: Clear target lists from memory.
* `stopscan`: Abort any ongoing scan or attack immediately.
* `reboot`: Restart the ESP32.

---

## License and Disclaimer

This project is intended strictly for **educational purposes, authorized security assessments, and compliance testing on your own equipment**. Unauthorized transmission or disruption of third-party networks and devices is illegal and the sole responsibility of the user.
