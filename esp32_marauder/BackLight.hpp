#include <Preferences.h>

#include "configs.h"

#ifndef BACKlIGHT_HPP
#define BACKlIGHT_HPP

//  #include "Display.h"
/*
  Four  #ifdef blocks defining brightnessInit() brightnessOn brightnessOff... etc...

    ifdef HAS_CH32V003     :  IO Port Expander used by MARAUDER_WS_C5_28
    elif HAS_AW9364        :  4-Channel 1-wire Dimming LED Driver used by MARAUDER_CYD_HMI / Lilygo-T-HMI
    elif ledcAttach        :  used by CYD and most others
    elif just digitalWrite
    else dummy/noop versions
    endif

    // Every block provides the following
    uint8_t BL_NUM_LEVELS
    uint8_t BL_LEVELS
    void brightnessInit()
    void brightnessCycle()
    void backlightOn()
    void backlightOff()
    uint8_t getBrightnessLevel()
    void brightnessSet(uint8_t level)
    void brightnessSave(uint8_t level)

*/

#if defined(HAS_SCREEN) && ( !defined(TFT_BL) || !defined(HAS_CH32V003) )
  #warning "HAS_SCREEN is defined and TFT_BL is undefined"
#endif

#if defined(HAS_SCREEN) && ( defined(TFT_BL) || defined(HAS_CH32V003) )

  static Preferences bl_prefs;
  static uint8_t bl_level_idx = 9; // default brightness


// HAS_CH32V003 used by MARAUDER_WS_C5_28
// CH32V003: IO Port Expander replaces TFT_BL Pin
#ifdef HAS_CH32V003
    #include <CH32V003_IOExpander.hpp>
    extern CH32V003_IOExpander CH32V003_obj;

    static const uint8_t BL_LEVELS[] = {0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255};

    static uint8_t BL_NUM_LEVELS = 16;
    // Dummy Functions, should never be called but are here just in case

    static void brightnessInit() { }   // brightnessInit Fallthrough

    static void brightnessCycle() { }

    static uint8_t getBrightnessLevel() {
      log_d("HAS_CH32V003 getBrightnessLevel level =  %d ", bl_level_idx);
      return bl_level_idx;
    }

    static void brightnessSet(uint8_t level) {
      if (level >= BL_NUM_LEVELS) {
        level = BL_NUM_LEVELS - 1;
        bl_level_idx = level;
      }

      CH32V003_obj.setPWM(BL_LEVELS[bl_level_idx]);
    }

    static void brightnessSave(uint8_t level) {
      log_i("HAS_CH32V003 brightnessSave level = %d ", level);
      bl_level_idx = level;
      brightnessSet(level);
      bl_prefs.putUChar("level", bl_level_idx);
    }

    static void backlightOn() {
      if (bl_level_idx < 3) {
        bl_level_idx = 3;
      }
      brightnessSet(bl_level_idx);
    }

    static void backlightOff() {
    log_d("backlightOff CH32V003");
      CH32V003_obj.setPWM(0);
    }

  // HAS_AW9364 used by MARAUDER_CYD_HMI / Lilygo-T-HMI
  // AW9364: 4-Channel 1-wire Dimming LED Driver
  #elif defined(HAS_AW9364)

    uint8_t BL_NUM_LEVELS = 16;

    static void _setBrightness(uint8_t value) {
        static uint8_t _brightness = 0;

        if (_brightness == value) {
            return;
        }

        if (value > 16) {
            value = 16;
        }
        if (value == 0) {
            digitalWrite(TFT_BL, 0);
            delay(3);
            _brightness = 0;
            return;
        }
        if (_brightness == 0) {
            digitalWrite(TFT_BL, 1);
            _brightness = BL_NUM_LEVELS;
            delayMicroseconds(30);
        }
        int from = BL_NUM_LEVELS - _brightness;
        int to = BL_NUM_LEVELS - value;
        int num = (BL_NUM_LEVELS + to - from) % BL_NUM_LEVELS;
        for (int i = 0; i < num; i++) {
            digitalWrite(TFT_BL, 0);
            digitalWrite(TFT_BL, 1);
        }
        _brightness = value;
    }

    static void brightnessInit() {
      log_i("HAS_AW9364 brightnessInit TFT_BL = %d", TFT_BL);
      pinMode(TFT_BL, OUTPUT);
      digitalWrite(TFT_BL, 1);
      // ledDriver.begin(TFT_BL);
      bl_prefs.begin("backlight", false);
      bl_level_idx = bl_prefs.getUChar("level", BL_NUM_LEVELS);
      if (bl_level_idx > BL_NUM_LEVELS) bl_level_idx = BL_NUM_LEVELS;
      log_d("HAS_AW9364 brightnessInit level = %d ", bl_level_idx);
      // ledDriver.setBrightness(bl_level_idx);
      // bl_level_idx = BL_NUM_LEVELS;
    }

    static void brightnessCycle() {
        bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
        _setBrightness(bl_level_idx);
        bl_prefs.putUChar("level", bl_level_idx);
        log_d("[Brightness] Level %d / %.0f %%", (bl_level_idx + 1), (static_cast<float>(bl_level_idx / BL_NUM_LEVELS) * 100));
    }

    uint8_t getBrightnessLevel() {
      log_d("HAS_AW9364 getBrightnessLevel level =  %d ", bl_level_idx);
      return bl_level_idx;
    }

    static void brightnessSet(uint8_t level) {
        log_i("HAS_AW9364 brightnessSet level = %d ", level);
        if (level > BL_NUM_LEVELS) level = BL_NUM_LEVELS;
        bl_level_idx = level;
        _setBrightness(bl_level_idx);
        // bl_prefs.putUChar("level", bl_level_idx);
    }

    static void brightnessSave(uint8_t level) {
        log_i("HAS_AW9364 brightnessSave level = %d ", level);
        bl_level_idx = level;
        brightnessSet(level);
        bl_prefs.putUChar("level", bl_level_idx);
    }

    static void backlightOn() {
      log_d("HAS_AW9364 backlightOn: %d", bl_level_idx);
      digitalWrite(TFT_BL, 1);
      if (bl_level_idx < 3) bl_level_idx = 3;
      _setBrightness(bl_level_idx);
    }

    static void backlightOff() {
      log_d("HAS_AW9364 backlightOff: %d", bl_level_idx);
      digitalWrite(TFT_BL, 0);
      // _setBrightness(0);
    }


  // Init PWM brightness AFTER display init (so ledcAttach overrides TFT_eSPI's pinMode)
  // #ifndef HAS_MINI_SCREEN


  #elif !defined(HAS_MINI_SCREEN)

  // PWM Brightness Control
  const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255, 255};
  uint8_t BL_NUM_LEVELS = 11;

  #define BL_CHANNEL 0
  #define BL_FREQ 6000
  #define BL_RESOLUTION 8

    // Helper macros for LEDC API compatibility (2.x vs 3.x board package)
    #if !defined(HAS_MINI_SCREEN) && !defined(HAS_AW9364)
      #if ESP_ARDUINO_VERSION_MAJOR >= 3
        // #define BL_SETUP()       ledcAttach(TFT_BL, BL_FREQ, BL_RESOLUTION)
        #define BL_SETUP()       ledcAttachChannel(TFT_BL, BL_FREQ, BL_RESOLUTION, BL_CHANNEL)
        //#define BL_SET(duty)     ledcWrite(TFT_BL, (duty))
        #define BL_SET(duty)     ledcWriteChannel(BL_CHANNEL, (duty))
        #define TLED          TFT_BL
      #else
        #define BL_SETUP()       do { ledcSetup(BL_CHANNEL, BL_FREQ, BL_RESOLUTION); ledcAttachPin(TFT_BL, BL_CHANNEL); } while(0)
        #define BL_SET(duty)     ledcWrite(BL_CHANNEL, (duty))
        #define TLED          BL_CHANNEL
      #endif
    #endif

    static void brightnessInit() {
      log_d("flipperLED::RunSetup: TFT_BL=%d", TFT_BL);
      pinMode(TFT_BL, OUTPUT);
      BL_SETUP();
      bl_prefs.begin("backlight", false);
      bl_level_idx = bl_prefs.getUChar("level", 9);
      if (bl_level_idx >= BL_NUM_LEVELS) bl_level_idx = 9;
      BL_SET(BL_LEVELS[bl_level_idx]);
      delay(5);
      uint32_t currentDuty = ledcRead(TLED);

      log_d("brightnessInit: level = %d currentDuty=%d", bl_level_idx, currentDuty);
      log_d("    BL_CHANNEL = %d", BL_CHANNEL);
      log_d("    TFT_BL = %d", TFT_BL);
      log_d("    BL TLED = %d", TLED);
    }

    static void brightnessCycle() {
      bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
      Serial.print(F("[Brightness] Level "));
      Serial.print(bl_level_idx + 1);
      Serial.print(F("/"));
      Serial.print(BL_NUM_LEVELS);
      Serial.print(F(" ("));
      Serial.print(BL_LEVELS[bl_level_idx] * 100 / 255);
      Serial.println(F("%)"));
    }

    static uint8_t getBrightnessLevel() {
      uint32_t currentDuty = ledcRead(TLED);
      // uint32_t currFreq = ledcReadFreq(TLED);
      log_d("level = %d currentDuty=%d, Freq=%d", bl_level_idx, currentDuty, ledcReadFreq(TLED));
      return bl_level_idx;
    }

    static void brightnessSet(uint8_t level) {
      if (level >= BL_NUM_LEVELS) {
        level = BL_NUM_LEVELS - 1;
        bl_level_idx = level;
      }

      uint32_t currentDuty; //  = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);

      BL_SET(BL_LEVELS[bl_level_idx]);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);

      currentDuty = ledcRead(TLED);
      // Serial.print("TFT_BL currentDuty = "); Serial.println(currentDuty);
      log_i("level = %d currentDuty=%d, Freq=%d", bl_level_idx, currentDuty, ledcReadFreq(TLED));
    }

    static void brightnessSave(uint8_t level) {
      if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);
      uint32_t currentDuty = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);
      log_i("level = %d currentDuty=%d, Freq=%d", bl_level_idx, currentDuty, ledcReadFreq(TLED));
    }

    static void backlightOn() {
      if (bl_level_idx < 3)
        bl_level_idx = 3;
      // Serial.println("BL brightnessOn");
      BL_SET(BL_LEVELS[bl_level_idx]);
      // Serial.print("bl_level_idx = "); Serial.println(BL_LEVELS[bl_level_idx]);
      uint32_t currentDuty = ledcRead(TLED);
      // Serial.print("currentDuty = "); Serial.println(currentDuty);
      log_d("level = %d currentDuty=%d, Freq=%d", bl_level_idx, currentDuty, ledcReadFreq(TLED));
    }

    static void backlightOff() {
      BL_SET(0);
      // log_d("backlightOff");
    log_d("backlightOff ledc");
      uint32_t currentDuty = ledcRead(TLED);
      log_d("level = %d currentDuty=%d", bl_level_idx, currentDuty);
    }

  #else   // HAS_MINI_SCREEN

  static uint8_t BL_NUM_LEVELS = 1;
  // dummyFunctions
  static static void brightnessInit() {
      log_d("brightnessInit(noDim): TFT_BL = %d", TFT_BL);
      Serial.print(F("[brightnessInit] HAS_MINI_SCREEN "));
      pinMode(TFT_BL, OUTPUT);
  }

  // dummy
  static void brightnessCycle() { }
  static uint8_t getBrightnessLevel() { return 9; }
  static void brightnessSet(uint8_t level) { (void) level; }
  static void brightnessSave(uint8_t level) { (void) level; }

  static void backlightOn() {
    log_d("backlightOn");
    #ifdef TFT_BL
      // ???
      #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, LOW);
      #endif

      #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, HIGH);
      #endif
    #else
      // Nothing
    #endif
  }

  static void backlightOff() {
    log_d("backlightOff pin");
    #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
      digitalWrite(TFT_BL, HIGH);
    #endif

    #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
      digitalWrite(TFT_BL, LOW);
    #endif
  }

#endif //  HAS_AW9364  / !HAS_MINI_SCREEN / ..

#else // HAS_SCREEN

  static uint8_t BL_NUM_LEVELS = 1;
  // Dummy Functions, should never be called but are here just in case
  static void brightnessInit() { log_d("-- brightnessInit Fallthrough"); }
  static void brightnessCycle() { }
  static uint8_t getBrightnessLevel() { return 9; }
  static void brightnessSet(uint8_t level) { }
  static void brightnessSave(uint8_t level) { }
  static void backlightOn() { }
  static void backlightOff() { }

#endif // HAS_SCREEN

#endif   //  BACKlIGHT_HPP
