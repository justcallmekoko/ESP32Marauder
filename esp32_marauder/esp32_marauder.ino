/* FLASH SETTINGS
Board: LOLIN D32
 Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include "configs.h"

#ifndef HAS_SCREEN
  #define MenuFunctions_h
  #define Display_h
#endif


// #include "ESP32_PinDebug.h"

#include <stdio.h>

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#include "Assets.h"
#include "WiFiScan.h"
#ifdef HAS_SD
  #include "SDInterface.h"
#endif
#include "Buffer.h"

#ifdef HAS_FLIPPER_LED
  #include "flipperLED.h"
#elif defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
#elif defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  #include "stickcLED.h"
#elif defined(HAS_NEOPIXEL_LED) || defined(HAS_T_DONGLE_LED)
  #include "LedInterface.h"
#endif

#include "settings.h"
#include "CommandLine.h"
#include "ReconMission.h"
#include "lang_var.h"

#ifdef HAS_T_DONGLE_DISPLAY
  #include "TDongleDisplay.h"
#endif

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif

#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef HAS_CH32V003
    #include <CH32V003_IOExpander.hpp>
    CH32V003_IOExpander CH32V003_obj;
#endif

// Yet another Cap Touch
#ifdef HAS_CST3530
    #include <CST3530.hpp>
    CST3530 CST3530_obj;
#endif

#if defined(HAS_SHTC3) && defined(HAS_TEMP_SENSOR)
    #include <SHTC3.hpp>
    SHTC3 SHTC3_obj;
#endif


#ifdef HAS_BUTTONS
  #include "Switches.h"
  
  #if (U_BTN >= 0 && U_BTN != -1)
    Switches u_btn = Switches(U_BTN, 1000, U_PULL);
    // perimanSetPinBusExtraType(U_BTN, "U_BTN");
  #endif
  #if (D_BTN >= 0 && D_BTN != -1)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
    // perimanSetPinBusExtraType(D_BTN, "D_BTN");
  #endif
  #if (L_BTN >= 0 && L_BTN != -1)
    Switches l_btn = Switches(L_BTN, 1000, L_PULL);
    // perimanSetPinBusExtraType(L_BTN, "L_BTN");
  #endif
  #if (R_BTN >= 0 && R_BTN != -1)
    Switches r_btn = Switches(R_BTN, 1000, R_PULL);
    // perimanSetPinBusExtraType(R_BTN, "R_BTN");
  #endif
  #if (C_BTN >= 0 && C_BTN != -1)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
    // perimanSetPinBusExtraType(C_BTN, "C_BTN");
  #endif

#endif


WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;
ReconMission recon_obj;

// Brightness functions defined in BackLight.cpp
#ifdef HAS_SCREEN
  void brightnessInit();
  extern void backlightOff();
  extern void backlightOn();
#endif

#ifdef HAS_T_DONGLE_DISPLAY
  TDongleDisplay t_dongle_display;
#endif

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_RTC
  #include "RTC.h"
  RTC rtc_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#if defined(HAS_SD)
  #if defined(HAS_C5_SD)
    SDInterface sd_obj = SDInterface(nullptr);
  #else
    SDInterface sd_obj;
  #endif
#endif

#ifdef HAS_FLIPPER_LED
  flipperLED flipper_led;
#elif defined(XIAO_ESP32_S3)
  xiaoLED xiao_led;
#elif defined(MARAUDER_M5STICKC) || defined(MARAUDER_M5STICKCP2)
  stickcLED stickc_led;
#elif defined(HAS_NEOPIXEL_LED) || defined(HAS_T_DONGLE_LED)
  LedInterface led_obj;
#endif

const String PROGMEM version_number = MARAUDER_VERSION;

#ifdef HAS_NEOPIXEL_LED
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800);
#endif

uint32_t currentTime  = 0;


/*  Fixed Below
#ifdef HAS_C5_SD
  SPIClass sharedSPI(SPI);
  SDInterface sd_obj = SDInterface(&sharedSPI, SD_CS);
#endif
*/

//   Converts reason type to a C string.
//  Type is located in /tools/sdk/esp32/include/esp_system/include/esp_system.h
const char *resetReasonName() {
  esp_reset_reason_t r = esp_reset_reason();
  switch (r) {
    case ESP_RST_UNKNOWN:   return "Unknown";
    case ESP_RST_POWERON:   return "PowerOn";    //Power on or RST pin toggled
    case ESP_RST_EXT:       return "ExtPin";     //External pin - not applicable for ESP32
    case ESP_RST_SW:        return "Reboot";     //esp_restart()
    case ESP_RST_PANIC:     return "Crash";      //Exception/panic
    case ESP_RST_INT_WDT:   return "WDT_Int";    //Interrupt watchdog (software or hardware)
    case ESP_RST_TASK_WDT:  return "WDT_Task";   //Task watchdog
    case ESP_RST_WDT:       return "WDT_Other";  //Other watchdog
    case ESP_RST_DEEPSLEEP: return "Sleep";      //Reset after exiting deep sleep mode
    case ESP_RST_BROWNOUT:  return "BrownOut";   //Brownout reset (software or hardware)
    case ESP_RST_SDIO:      return "SDIO";       //Reset over SDIO
    case ESP_RST_USB:       return "USB";        // Reset by USB peripheral
    case ESP_RST_JTAG:      return "JTAG";       // Reset by JTAG
    case ESP_RST_EFUSE:     return "EFUSE";      // Reset due to efuse error
    case ESP_RST_PWR_GLITCH: return "PWR_GLITCH";       // Reset due to power glitch detected
    case ESP_RST_CPU_LOCKUP: return "CPU_LOCKUP";       // Reset due to CPU lock up (double exception)
    default:                return "?";
  }
}


void print_reset_reason() {
  Serial.print(F("Last reset reason: "));
  Serial.println(resetReasonName());
}


void setup()
{

  log_d("Main setup");

  // https://github.com/Xinyuan-LilyGO/T-HMI/issues/34
  // LILYGO T-HMI : latch power on if on battery
  // Prevent StickCP2 from turning off when disconnect USB cable
  #ifdef POWER_HOLD_PIN  
    log_d("Enable POWER_HOLD_PIN");
    pinMode(POWER_HOLD_PIN, OUTPUT);
    digitalWrite(POWER_HOLD_PIN, HIGH);
    // perimanSetPinBusExtraType(POWER_HOLD_PIN, "POWER_HOLD_PIN");
  #endif

  // needed for MARAUDER_CYD_HMI "LILYGO T-HMI ESP32-S3
  // Enable power to screen & peripherals
  #ifdef PWR_EN_PIN  // Enable power to peripherals
    log_d("Enable power to peripherals");
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    // perimanSetPinBusExtraType(PWR_EN_PIN, "PWR_EN_PIN");
  #endif

  randomSeed(esp_random());
  
  #ifndef DEVELOPER
    esp_log_level_set("*", ESP_LOG_NONE);
  #endif

  #ifndef HAS_IDF_3
    esp_spiram_init();
  #endif

  Serial.begin(115200);

  #ifdef I2C_SDA
    log_d("I2C Begin: Wire: I2C_SDA=%d  I2C_SCL=%d", TP_SDA, TP_SCL);
    Wire.begin(TP_SDA, TP_SCL);
  #endif

  #ifdef HAS_CH32V003
    if (!CH32V003_obj.begin()) {
      Serial.println("CH32V003 not found - check wiring and I2C address");
    }
  #endif

  #ifdef HAS_ACT_LED
    pinMode(ACT_LED_PIN, OUTPUT);
    delay(100);
    digitalWrite(ACT_LED_PIN, LOW);
    // perimanSetPinBusExtraType(ACT_LED_PIN, "ACT_LED_PIN");
  #endif

  #if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT == 1
    while(!Serial && millis() < 2000) {
      delay(500);
    }
  #else
    while(!Serial)
	delay(10);
  #endif

  #if ESP_ARDUINO_VERSION_MAJOR >= 3
  // &&  defined(ARDUINO_USB_CDC_ON_BOOT)
    log_d("setting setTxTimeoutMs()");
    Serial.setTxTimeoutMs(0);
  #endif

  // #ifdef HAS_C5_SD
  //   sharedSPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  //   delay(100);
  // #endif

  // TFT_BL >= 0 does not if TFT_BL is -1
  // due to cpp's "unsigned promotion rules" where -1 == maxint
  #ifdef HAS_SCREEN && defined(TFT_BL) && TFT_BL != -1
    log_d("pinMode %d OUTPUT", TFT_BL);
    pinMode(TFT_BL, OUTPUT);
    // perimanSetPinBusExtraType(TFT_BL, "TFT_BL");
  #endif
  
  #ifdef HAS_SCREEN
    backlightOff();
  #endif

  #if BATTERY_ANALOG_ON == 1
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(CHARGING_PIN, INPUT);
    // perimanSetPinBusExtraType(BATTERY_PIN, "BATTERY_PIN");
    // perimanSetPinBusExtraType(CHARGING_PIN, "CHARGING_PIN");
  #endif
  
  #if defined(TFT_CS) && TFT_CS != -1
    log_d("TFT_CS=%d", TFT_CS);
    pinMode(TFT_CS, OUTPUT);
    // perimanSetPinBusExtraType(TFT_CS, "TFT_CS");
  #endif

  #ifdef MARAUDER_WS_C5_28

    // Must happen before display init CH32V003 controls LCD_RST and backlight
    log_d("Wire: I2C_SDA=%d  I2C_SCL=%d", TP_SDA, TP_SCL);

    log_d("CH32V003_obj.lcdReset");
    CH32V003_obj.lcdReset();      // pulses LCD_RST via EXIO1

    log_d("CH32V003_obj.setPWM");
    CH32V003_obj.setPWM(80); // 80% brightness

    log_d("CH32V003_obj.touchReset");
    CH32V003_obj.touchReset();    // pulses Touch_RST via EXIO0

    #ifdef HAS_CST3530
      CST3530_obj.begin(Wire);
      // #if defined(TP_INT) && TP_INT >= 0
      //   CST3530_obj.enableInterrupt(TP_INT);
      // #endif
      // CST3530_obj.begin(&Wire, TP_RST, TP_INT, TP_FREQ);
      log_d("CST3530_obj.begin done");
    #else
      log_d("HAS_CST3530 False");
    #endif

    #if defined(HAS_SHTC3) && defined(HAS_TEMP_SENSOR)
      SHTC3_obj.begin(&Wire);
      log_d("SHTC3_obj.begin done");
    #endif

  #endif  // MARAUDER_WS_C5_28

  // Preset SPI CS pins to avoid bus conflicts
  // Beware of "unsigned promotion rules" where -1 == maxint
  #if defined(HAS_SCREEN) && defined(TFT_CS) && TFT_CS != -1
    digitalWrite(TFT_CS, HIGH);
  #endif

  #if defined(HAS_SD) && defined(SD_CS) && !defined(HAS_C5_SD)
    log_d("SD_CS=%d", SD_CS);
    pinMode(SD_CS, OUTPUT);
    delay(10);
  
    digitalWrite(SD_CS, HIGH);
    delay(10);
  #endif

  //Serial.begin(115200);

  //while(!Serial)
  //  delay(10);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("RTC::setup: ");
    Serial.println(&timeinfo, "%F %T");
  } else {
    log_w("getLocalTime Fail");
  }

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));
  #ifdef ESP_ARDUINO_VERSION_STR
    Serial.print("Arduino ESP32 Core Version: ");
    Serial.println(ESP_ARDUINO_VERSION_STR);
  #elif defined(ESP_ARDUINO_VERSION)
    Serial.printf("Arduino Core Major: %d, Minor: %d, Patch: %d\n", 
	    ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  #endif

  #ifdef HAS_PSRAM
    log_d("psramInit");
    if (!psramInit()) {
      Serial.println(F("PSRAM not available"));
      log_d("PSRAM not available");
    }
  #endif

  #ifdef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));
    #endif
  #endif

  #ifdef HAS_SCREEN
    log_d("display_obj.RunSetup");
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  // this removes the need for "sharedSPI"
  // the TFT init fucks up the SPI bus by closing SD_MISO
  // so reinit it and assert it to for the sd_obj.
  #ifdef HAS_C5_SD
    SPIClass& spi = display_obj.tft.getSPIinstance();
    spi.end();                                          // release TFT's MISO-less bus config
    spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);         // re-init with MISO included
    sd_obj.setSPI(&spi);
  #endif

  // Init PWM brightness AFTER display init (so ledcAttach overrides TFT_eSPI's pinMode)
  #if defined(HAS_SCREEN) && !defined(HAS_MINI_SCREEN)
    brightnessInit();
    backlightOff();
  #endif

  #ifdef HAS_SCREEN
    display_obj.drawBootSplash();
  #endif

  #ifdef HAS_SCREEN
    backlightOn(); // Need this
  #endif

  #ifdef HAS_SCREEN
    // Do some stealth mode stuff
    #ifdef HAS_BUTTONS
      if (c_btn.justPressed()) {
        display_obj.headless_mode = true;

        backlightOff();
      }
    #endif
  #endif

  settings_obj.begin();

  const char* type = settings_obj.getSettingType("wu");

  if (type == nullptr || type[0] == '\0') {
    Serial.println(F("Current settings format not supported. Installing new default settings..."));
    settings_obj.createDefaultSettings(SPIFFS);
  }

  buffer_obj = Buffer();

  #ifndef HAS_SIMPLEX_DISPLAY
    #if defined(HAS_SD)
      // Do some SD stuff
      if(!sd_obj.initSD())
        Serial.println(F("SD Card NOT Supported"));
    #endif
  #endif

  log_d("wifi_scan_obj.RunSetup");
  wifi_scan_obj.RunSetup();

  #ifdef HAS_RTC
    rtc_obj.RunSetup();
  #else
    log_d("RTC NOT Installed");
  #endif

  #ifdef HAS_T_DONGLE_DISPLAY
    t_dongle_display.begin();
  #endif

  evil_portal_obj.setup();

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif

  // Do some LED stuff
  #ifdef HAS_FLIPPER_LED
    flipper_led.RunSetup();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.RunSetup();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.RunSetup();
  #elif defined(HAS_NEOPIXEL_LED) || defined(HAS_T_DONGLE_LED)
    led_obj.RunSetup();
  #endif

  #ifdef HAS_GPS
    if (settings_obj.loadSetting<bool>("Probe GPS at Boot")) {    // faster Boot
      gps_obj.begin();
    }
  #endif

  #ifdef HAS_SCREEN  
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  #ifdef HAS_SCREEN
    #if defined(MARAUDER_CARDPUTER) || defined(MARAUDER_CARDPUTER_ADV)
      display_obj.clearScreen();
    #endif
    menu_function_obj.RunSetup();
  #endif

  /*char ssidBuf[64] = {0};  // or prefill with existing SSID
  if (keyboardInput(ssidBuf, sizeof(ssidBuf), "Enter SSID")) {
    // user pressed OK
    Serial.println(ssidBuf);
  } else {
    Serial.println(F("User exited keyboard"));
  }

  menu_function_obj.changeMenu(menu_function_obj.current_menu);*/

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);

  cli_obj.RunSetup();
}


void loop()
{
  currentTime = millis();
  bool mini = false;

  #ifdef SCREEN_BUFFER
    #ifndef HAS_ILI9341
      mini = true;
    #endif
  #endif

  #if (defined(HAS_ILI9341) && !defined(MARAUDER_CYD_2USB))
    #ifdef HAS_BUTTONS
      if (c_btn.isHeld()) {
        if (menu_function_obj.disable_touch)
          menu_function_obj.disable_touch = false;
        else
          menu_function_obj.disable_touch = true;

        menu_function_obj.updateStatusBar();

        while (!c_btn.justReleased())
          delay(1);
      }
    #endif
  #endif

  // Update all of our objects
  cli_obj.main(currentTime);
  wifi_scan_obj.main(currentTime);
  recon_obj.main(currentTime);

  #ifdef HAS_T_DONGLE_DISPLAY
    t_dongle_display.update(currentTime, wifi_scan_obj);
  #endif

  #ifdef HAS_GPS
    gps_obj.main();
  #endif

  // Save buffer to SD and/or serial
  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
  #endif
  // menu_function_obj.updateStatusBar();
  if ((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) ||
      (mini)) {
    #ifdef HAS_SCREEN
      menu_function_obj.main(currentTime);
    #endif
  }
  #ifdef HAS_FLIPPER_LED
    flipper_led.main();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.main();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.main();
  #elif defined(HAS_T_DONGLE_LED)
    // The LED shares GPIO2/GPIO7 with the display/SD bus. Always make it the
    // final writer so later SPI activity cannot leave it latched white.
    led_obj.refresh();
  #elif defined(HAS_NEOPIXEL_LED)
    led_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
}
