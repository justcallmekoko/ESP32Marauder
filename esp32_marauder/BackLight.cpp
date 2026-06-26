#include <Preferences.h>

#include "configs.h"

/*
  Three #ifdef blocks defining brightnessInit() brightnessOn brightnessOff... etc...
    using HAS_AW9364
    using ledcAttach
    using just digitalWrite
    and dummy/noop versions

    void brightnessInit()
    void brightnessCycle()
    void backlightOn()
    void backlightOff()
    uint8_t getBrightnessLevel()
    void brightnessSet(uint8_t level)
    void brightnessSave(uint8_t level)

*/
#if defined(HAS_SCREEN) && !defined(TFT_BL)
  #warning "HAS_SCREEN is defined and TFT_BL is undefined"
#endif

extern const uint8_t BL_NUM_LEVELS;

#if defined(HAS_SCREEN) && defined(TFT_BL)


Preferences bl_prefs;
uint8_t bl_level_idx = 9; // default brightness

// HAS_AW9364
// AW9364: 4-Channel 1-wire Dimming LED Driver
#if defined(HAS_AW9364)

  const uint8_t BL_NUM_LEVELS = 16;

  void _setBrightness(uint8_t value)
  {
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

  void brightnessInit() {
    log_d("HAS_AW9364 brightnessInit TFT_BL = %d", TFT_BL);
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

  void brightnessCycle() {
      bl_level_idx = (bl_level_idx + 1) % BL_NUM_LEVELS;
      _setBrightness(bl_level_idx);
      bl_prefs.putUChar("level", bl_level_idx);
      log_d("[Brightness] Level %d / %.0f %%", (bl_level_idx + 1), (static_cast<float>(bl_level_idx / BL_NUM_LEVELS) * 100));
  }

  uint8_t getBrightnessLevel() {
    log_d("HAS_AW9364 getBrightnessLevel level =  %d ", bl_level_idx);
    return bl_level_idx;
  }

  void brightnessSet(uint8_t level) {
      log_d("HAS_AW9364 brightnessSet level = %d ", level);
      if (level > BL_NUM_LEVELS) level = BL_NUM_LEVELS;
      bl_level_idx = level;
      _setBrightness(bl_level_idx);
      // bl_prefs.putUChar("level", bl_level_idx);
  }

  void brightnessSave(uint8_t level) {
      log_d("HAS_AW9364 brightnessSave level = %d ", level);
      bl_level_idx = level;
      brightnessSet(level);
      bl_prefs.putUChar("level", bl_level_idx);
  }

  void backlightOn() {
    log_d("HAS_AW9364 backlightOn: %d", bl_level_idx);
    digitalWrite(TFT_BL, 1);
    if (bl_level_idx < 3) bl_level_idx = 3;
    _setBrightness(bl_level_idx);
  }

  void backlightOff() {
    log_d("HAS_AW9364 backlightOff: %d", bl_level_idx);
    digitalWrite(TFT_BL, 0);
    // _setBrightness(0);
  }


  // Init PWM brightness AFTER display init (so ledcAttach overrides TFT_eSPI's pinMode)
  // #ifndef HAS_MINI_SCREEN


#elif defined(HAS_SCREEN) &&  !defined(HAS_MINI_SCREEN)

  // PWM Brightness Control
  const uint8_t BL_LEVELS[] = {26, 51, 77, 102, 128, 153, 179, 204, 230, 255};
  const uint8_t BL_NUM_LEVELS = 10;

  #define BL_CHANNEL 0
  #define BL_FREQ 5000
  #define BL_RESOLUTION 8

  // Helper macros for LEDC API compatibility (2.x vs 3.x board package)
  #if !defined(HAS_MINI_SCREEN) && !defined(HAS_AW9364)
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
      #define BL_SETUP()       ledcAttach(TFT_BL, BL_FREQ, BL_RESOLUTION)
      #define BL_SET(duty)     ledcWrite(TFT_BL, (duty))
    #else
      #define BL_SETUP()       do { ledcSetup(BL_CHANNEL, BL_FREQ, BL_RESOLUTION); ledcAttachPin(TFT_BL, BL_CHANNEL); } while(0)
      #define BL_SET(duty)     ledcWrite(BL_CHANNEL, (duty))
    #endif
  #endif

  void brightnessInit() {
      // Serial.print(F("[brightnessInit] ledcWrite "));
      pinMode(TFT_BL, OUTPUT);
      BL_SETUP();
      bl_prefs.begin("backlight", false);
      bl_level_idx = bl_prefs.getUChar("level", 9);
      if (bl_level_idx >= BL_NUM_LEVELS) bl_level_idx = 9;
      BL_SET(BL_LEVELS[bl_level_idx]);
  }

  void brightnessCycle() {
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

  uint8_t getBrightnessLevel() {
      return bl_level_idx;
  }

  void brightnessSet(uint8_t level) {
    if (level >= BL_NUM_LEVELS)
      level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;
      BL_SET(BL_LEVELS[bl_level_idx]);
  }

  void brightnessSave(uint8_t level) {
      if (level >= BL_NUM_LEVELS) level = BL_NUM_LEVELS - 1;
      bl_level_idx = level;
      BL_SET(BL_LEVELS[bl_level_idx]);
      bl_prefs.putUChar("level", bl_level_idx);
  }

  void backlightOn() {
    // Serial.println("BL brightnessOn");
    BL_SET(BL_LEVELS[bl_level_idx]);
  }

  void backlightOff() {
    // Serial.println("BL brightnessOff");
      BL_SET(0);
  }
#else   // HAS_MINI_SCREEN

  const uint8_t BL_NUM_LEVELS = 1;
  // dummyFunctions
  void brightnessInit() {
      // Serial.print(F("[brightnessInit] HAS_MINI_SCREEN "));
      pinMode(TFT_BL, OUTPUT);
  }
  void brightnessCycle() { }

  uint8_t getBrightnessLevel() { return 9; }

  void brightnessSet(uint8_t level) { (void) level; }
  void brightnessSave(uint8_t level) { (void) level; }

  void backlightOn() {
    // Serial.println("-- brightnessOn");
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

  void backlightOff() {
    // Serial.println("-- brightnessOff");
    #ifdef TFT_BL
      #if defined(MARAUDER_MINI) || defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, HIGH);
      #endif

      #if !defined(MARAUDER_MINI) && !defined(MARAUDER_MINI_V3)
        digitalWrite(TFT_BL, LOW);
      #endif
    #else
      // Nothing
    #endif
  }

#endif  //  HAS_AW9364  / !HAS_MINI_SCREEN / ..

#else  // HAS_SCREEN

  const uint8_t BL_NUM_LEVELS = 1;
  // Dummy Functions, should never be called but are here just in case
  void brightnessInit() {
      Serial.print(F("[brightnessInit] NULL "));
  }
  void brightnessCycle() { }
  uint8_t getBrightnessLevel() { return 9; }
  void brightnessSet(uint8_t level) { }
  void brightnessSave(uint8_t level) { }
  void backlightOn() { Serial.print(F("[backlightOn] NULL "));  }
  void backlightOff() {Serial.print(F("[backlightOff] NULL ")); }

#endif // HAS_SCREEN
