// #include <bitset>
/*
MIT License

Copyright (c) 2026 Peter Shipley

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction,
*/


/*
    some parts barrowed from fbiego's CST816S driver
    fbiego/CST816S
    https://pan.jczn1688.com/pd/1/HMI%20display/JC2432W328.zip
*/


/*

    if the upper nubble of reg 0xFE are set gestures are "event types" are disabled/
    I am yet to Figure out how to enable gestures.

*/

#include "CST820.h"

#ifdef HAS_CST820


// https://pebblebay.com/i2c-lock-up-prevention-and-recovery/
#define I2C_RECOVER_NUM_CLOCKS      10U     /* # clock cycles for recovery  */
#define I2C_RECOVER_CLOCK_FREQ      50000U  /* clock frequency for recovery */

#define I2C_RECOVER_CLOCK_DELAY_US  (1000000U / (2U * I2C_RECOVER_CLOCK_FREQ))

CST820::CST820() {
}

void CST820::begin(int8_t _sda, int8_t _scl, int8_t _rst, int8_t _int, uint32_t freq) {
    int8_t x;
    int i;


      log_d("CST820::begin");
    #ifdef I2C_SDA
      if (_sda != -1 && _sda != I2C_SDA) {
        _wire = &Wire1;
        log_d("CST820::begin using Wire1");
      } else {
        _wire = &Wire;
      }
    #else
      _wire = &Wire;
    #endif

      pinMode(_scl, OUTPUT);
    // Initialize I2C
    // https://github.com/esp8266/Arduino/issues/1025
    // https://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
    if (_sda != -1 && _scl != -1) {
        /*
        for (x= 0; x < I2C_RECOVER_NUM_CLOCKS; ++x) {
            digitalWrite(_scl, LOW);
            delayMicroseconds(5);
            digitalWrite(_scl, HIGH);
            delayMicroseconds(5);
        }
        */

        log_d("CST820::RunSetup SDA=%d SCL=%d, freq=%d", _sda, _scl, freq);
        _wire->begin(_sda, _scl, freq);
    } else {
        _wire->begin();
        if (freq)
          _wire->setClock(freq);
          log_d("CST820::begin setClock %d", freq);
    }

    _wire = &Wire;

    // Int Pin Configuration
    if (_int != -1) {
        // pinMode(_int, INPUT);
        pinMode(_int, INPUT_PULLUP);
    }

    // Reset Pin Configuration
    if (_rst != -1) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(10);
        digitalWrite(_rst, HIGH);
        delay(300);
    }

    memset(&data, 0, sizeof(data));

    data.version = i2c_read(0x15);
    delay(5);
    x = i2c_read_continuous(0xA7, data.versionInfo, 3);
    if (x != 0) {
        Serial.print("Err reading data.versionInfo: ");
        Serial.println(x);
    }

    disable_auto_sleep();

    // Enable continuous gesture actions and double-click
    enable_double_click(0x07);


    /*  Untested
    i2c_write(0xEF, 30); // MotionSlAngle
    i2c_write(0xFA, 0X79); // IrqCtl
    i2c_write(0xFB, 5); // AutoReset
    i2c_write(0xFC, 10); // LongPressTime
    */
}

/*!
    @brief  update data obj
*/
void CST820::read_touch() {
    byte data_raw[8];
    int8_t x;
    do {
        x = i2c_read_continuous(0x01, data_raw, 6);   // -1 = err, 0 = good
    } while (x);

    data.gestureID = data_raw[0] & 0x0F;  // Gesture
    data.points = data_raw[1];     // number of touch points
    data.event = data_raw[2] >> 6;  // Event (0 = Down, 1 = Up, 2 = Contact)
    data.x = ((data_raw[2] & 0xF) << 8) + data_raw[3];
    data.y = ((data_raw[4] & 0xF) << 8) + data_raw[5];
}

/*
    Legacy call interface
*/
uint8_t CST820::getTouch(uint16_t *x, uint16_t *y, uint16_t ignore) {
    read_touch();

    //  if (gesture)
    //   *gesture = data.gestureID;
    //  if (!(*gesture == SlideUp || *gesture == SlideDown))
    //  {
    //     *gesture = None;
    //  }

    *x = data.x;
    *y = data.y;

    return data.points;
}

  /*
      test for input
  */
  bool CST820::available() {
      read_touch();
      return static_cast<bool>(data.points);
  }


  uint8_t CST820::i2c_read(uint8_t addr) {
      uint8_t rdData = 0;
      uint8_t rdDataCount;
      do {
          _wire->beginTransmission(I2C_ADDR_CST820);
          _wire->write(addr);
          _wire->endTransmission(false);  // Restart
          rdDataCount = _wire->requestFrom(I2C_ADDR_CST820, 1);
      } while (rdDataCount == 0);
      while (_wire->available()) {
          rdData = _wire->read();
      }
      return rdData;
  }

  int8_t CST820::i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length) {
    _wire->beginTransmission(I2C_ADDR_CST820);
    _wire->write(addr);
    if ( _wire->endTransmission(true))
        return -1;
    _wire->requestFrom(I2C_ADDR_CST820, length);
    for (int i = 0; i < length; i++) {
      *data++ = _wire->read();
    }
    return 0;
  }

  int8_t CST820::i2c_write(uint8_t addr, uint8_t data) {
    _wire->beginTransmission(I2C_ADDR_CST820);
    _wire->write(addr);
    _wire->write(data);
    _wire->endTransmission();
    return _wire->endTransmission(true);
  }

  int8_t CST820::i2c_write_continuous(uint8_t addr, const uint8_t *data, uint32_t length) {
    _wire->beginTransmission(I2C_ADDR_CST820);
    _wire->write(addr);
    for (int i = 0; i < length; i++) {
      _wire->write(*data++);
    }
    return _wire->endTransmission(true);
  }

  /*
     - Bit 0: EnDClick (enable double-click)
     - Bit 1: EnConUD (enable continuous up/down swipe)
     - Bit 2: EnConLR (enable continuous left/right swipe)
  */
  int8_t CST820::enable_double_click(byte enable) {    // default 0x01
    // byte enableDoubleTap = 0x01;  // Set EnDClick (bit 0) to enable double-tap
    return i2c_write(0xEC, enable & 0x07);
  }

  /*!
      @brief  Disable auto sleep mode
  */
  int8_t CST820::disable_auto_sleep(void) {
    byte disableAutoSleep = 0x01;  // 0x01 value disables auto sleep
    return i2c_write(0xFE, disableAutoSleep);
  }

  /*!
      @brief  Enable auto sleep mode
  */
  int8_t CST820::enable_auto_sleep(void) {
    byte enableAutoSleep = 0x00;  // 0 value enables auto sleep
    return i2c_write(0xFE, enableAutoSleep);
  }

  /*!
      @brief  Set the auto sleep time
      @param  seconds Time in seconds (1-255) before entering standby mode after inactivity
  */
  int8_t CST820::set_auto_sleep_time(int seconds) {
    if (seconds < 1) {
      seconds = 1;  // Enforce minimum value of 1 second
    } else if (seconds > 255) {
      seconds = 255;  // Enforce maximum value of 255 seconds
    }

    byte sleepTime = static_cast<byte>(seconds);  // Convert int to byte
    return i2c_write(0xF9, sleepTime);
  }

  /*!
      @brief  get the gesture event name
  */
  String CST820::gesture() {
    switch (data.gestureID) {
      case None:
        return F("NONE");
        break;
      case SlideDown:
        return F("SWIPE DOWN");
        break;
      case SlideUp:
        return F("SWIPE UP");
        break;
      case SlideLeft:
        return F("SWIPE LEFT");
        break;
      case SlideRight:
        return F("SWIPE RIGHT");
        break;
      case SingleTap:
        return F("SINGLE CLICK");
        break;
      case DoubleTap:
        return F("DOUBLE CLICK");
        break;
      case LongPress:
        return F("LONG PRESS");
        break;
      default:
        return (String)data.gestureID;
        break;
    }
  }

  String CST820::event_type() {
      return EVENT_TYPES[data.event];
  }

#endif HAS_CST820
