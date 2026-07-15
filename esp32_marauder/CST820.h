
#ifndef _CST820_H
#define _CST820_H

#include "configs.h"

#ifdef HAS_CST820

  #include <Arduino.h>
  #include <Wire.h>

  #ifndef I2C_ADDR_CST820
  #define I2C_ADDR_CST820 0x15
  #endif

  enum GESTURE {
    None = 0x00,
    SlideDown = 0x01,
    SlideUp = 0x02,
    SlideLeft = 0x03,
    SlideRight = 0x04,
    SingleTap = 0x05,
    DoubleTap = 0x0B,
    LongPress = 0x0C
  };

  char *EVENT_TYPES[] = {"Down", "Up", "Contact"};

  //  from CST820
  struct data_struct {
    byte gestureID; // Gesture ID
    byte points;    // Number of touch points
    byte event;     // Event (0 = Down, 1 = Up, 2 = Contact)
    short x;
    short y;
    uint8_t version;
    uint8_t versionInfo[3]; // Chip, proj, firmware ver
  };

  /*!
      @brief  CST820 I2C CTP controller driver
  */
  class CST820 {
  public:
    CST820();


    void begin(int8_t _sda = -1, int8_t _scl = -1, int8_t _rst = -1,
               int8_t _int = -1, uint32_t freq = 0);
    uint8_t getTouch(uint16_t *x, uint16_t *y, uint16_t ignore = 0);
    String gesture();
    String event_type();
    int8_t set_auto_sleep_time(int seconds = 1);
    int8_t enable_double_click(byte enable = 0x01);
    int8_t disable_auto_sleep();
    int8_t enable_auto_sleep();
    bool available();
    data_struct data;
    void read_touch();

  private:

    TwoWire *_wire;
    int8_t sda, scl;
    uint8_t i2c_read(uint8_t addr);
    int8_t i2c_read_continuous(uint8_t addr, uint8_t *data, uint32_t length);
    int8_t i2c_write(uint8_t addr, uint8_t data);
    int8_t i2c_write_continuous(uint8_t addr, const uint8_t *data,
                                uint32_t length);
  };

#endif // HAS_CST820
#endif // _CST820_H
