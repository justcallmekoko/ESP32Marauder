/**
 *  @file Adafruit_TCA8418.cpp
 *
 *  @mainpage I2C Driver for the Adafruit TCA8418 Keypad Matrix / GPIO Expander
 * Breakout
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for the Adafruit TCA8418 Keypad Matrix / GPIO Expander
 * Breakout
 *
 * 	This is a library for the Adafruit TCA8418 breakout:
 * 	https://www.adafruit.com/product/XXXX
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *  @section dependencies Dependencies
 *  This library depends on the Adafruit BusIO library
 *
 *  @section author Author
 *
 *  Limor Fried (Adafruit Industries)
 *
 * 	@section license License
 *
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Arduino.h"

#include "Adafruit_TCA8418.h"

/**
 *    @brief  Instantiates a new TCA8418 class
 */
Adafruit_TCA8418::Adafruit_TCA8418(void) {}

/**
 *    @brief  destructor
 */
Adafruit_TCA8418::~Adafruit_TCA8418(void) {}

/**
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  address  The I2C address for the expander
 *    @param  wire     The TwoWire object to be used for I2C connections.
 *    @return True if initialization was successful, otherwise false.
 */
bool Adafruit_TCA8418::begin(uint8_t address, TwoWire *wire) {
  if (i2c_dev) {
    delete i2c_dev; // remove old interface
  }

  i2c_dev = new Adafruit_I2CDevice(address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  //  GPIO
  //  set default all GIO pins to INPUT
  writeRegister(TCA8418_REG_GPIO_DIR_1, 0x00);
  writeRegister(TCA8418_REG_GPIO_DIR_2, 0x00);
  writeRegister(TCA8418_REG_GPIO_DIR_3, 0x00);

  //  add all pins to key events
  writeRegister(TCA8418_REG_GPI_EM_1, 0xFF);
  writeRegister(TCA8418_REG_GPI_EM_2, 0xFF);
  writeRegister(TCA8418_REG_GPI_EM_3, 0xFF);

  //  set all pins to FALLING interrupts
  writeRegister(TCA8418_REG_GPIO_INT_LVL_1, 0x00);
  writeRegister(TCA8418_REG_GPIO_INT_LVL_2, 0x00);
  writeRegister(TCA8418_REG_GPIO_INT_LVL_3, 0x00);

  //  add all pins to interrupts
  writeRegister(TCA8418_REG_GPIO_INT_EN_1, 0xFF);
  writeRegister(TCA8418_REG_GPIO_INT_EN_2, 0xFF);
  writeRegister(TCA8418_REG_GPIO_INT_EN_3, 0xFF);

  return true;
}

/**
 * @brief configures the size of the keypad matrix.
 *
 * @param [in] rows    number of rows, should be <= 8
 * @param [in] columns number of columns, should be <= 10
 * @return true is rows and columns have valid values.
 *
 * @details will always use the lowest pins for rows and columns.
 *          0..rows-1  and  0..columns-1
 */
bool Adafruit_TCA8418::matrix(uint8_t rows, uint8_t columns) {
  if ((rows > 8) || (columns > 10))
    return false;

  //  MATRIX
  //  skip zero size matrix
  if ((rows != 0) && (columns != 0)) {
    // setup the keypad matrix.
    uint8_t mask = 0x00;
    for (int r = 0; r < rows; r++) {
      mask <<= 1;
      mask |= 1;
    }
    writeRegister(TCA8418_REG_KP_GPIO_1, mask);

    mask = 0x00;
    for (int c = 0; c < columns && c < 8; c++) {
      mask <<= 1;
      mask |= 1;
    }
    writeRegister(TCA8418_REG_KP_GPIO_2, mask);

    if (columns > 8) {
      if (columns == 9)
        mask = 0x01;
      else
        mask = 0x03;
      writeRegister(TCA8418_REG_KP_GPIO_3, mask);
    }
  }

  return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  KEY EVENTS
//

/**
 * @brief checks if key events are available in the internal buffer
 *
 * @return number of key events in the buffer
 */
uint8_t Adafruit_TCA8418::available() {
  uint8_t eventCount = readRegister(TCA8418_REG_KEY_LCK_EC);
  eventCount &= 0x0F; //  lower 4 bits only
  return eventCount;
}

/**
 * @brief gets first event from the internal buffer
 *
 * @return key event or 0 if none available
 *
 * @details
 *     key event 0x00        no event
 *               0x01..0x50  key  press
 *               0x81..0xD0  key  release
 *               0x5B..0x72  GPIO press
 *               0xDB..0xF2  GPIO release
 */
uint8_t Adafruit_TCA8418::getEvent() {
  uint8_t event = readRegister(TCA8418_REG_KEY_EVENT_A);
  return event;
}

/**
 * @brief flushes the internal buffer of key events
 *        and cleans the GPIO status registers.
 *
 * @return number of keys flushed.
 */
uint8_t Adafruit_TCA8418::flush() {
  //  flush key events
  uint8_t count = 0;
  while (getEvent() != 0)
    count++;
  //  flush gpio events
  readRegister(TCA8418_REG_GPIO_INT_STAT_1);
  readRegister(TCA8418_REG_GPIO_INT_STAT_2);
  readRegister(TCA8418_REG_GPIO_INT_STAT_3);
  //  clear INT_STAT register
  writeRegister(TCA8418_REG_INT_STAT, 3);
  return count;
}

/////////////////////////////////////////////////////////////////////////////
//
//  GPIO
//

/**
 * @brief read GPIO
 *
 * @param [in] pinnum Pin name between TCA8418_ROW0 and TCA8418_COL9  0..17
 * @return 0 = LOW, 1 = HIGH, 0xFF = pinnum out of range
 */
uint8_t Adafruit_TCA8418::digitalRead(uint8_t pinnum) {
  if (pinnum > TCA8418_COL9)
    return 0xFF;

  uint8_t reg = TCA8418_REG_GPIO_DAT_STAT_1 + pinnum / 8;
  uint8_t mask = (1 << (pinnum % 8));

  // LEVEL  0 = LOW  other = HIGH
  uint8_t value = readRegister(reg);
  if (value & mask)
    return HIGH;
  return LOW;
}

/**
 * @brief set GPIO pin to LOW or HIGH
 *
 * @param [in] pinnum  Pin name between TCA8418_ROW0 and TCA8418_COL9  0..17
 * @param [in] level   0 = LOW, all other are HIGH
 * @return true if successful
 */
bool Adafruit_TCA8418::digitalWrite(uint8_t pinnum, uint8_t level) {
  if (pinnum > TCA8418_COL9)
    return false;

  uint8_t reg = TCA8418_REG_GPIO_DAT_OUT_1 + pinnum / 8;
  uint8_t mask = (1 << (pinnum % 8));

  // LEVEL  0 = LOW  other = HIGH
  uint8_t value = readRegister(reg);
  if (level == LOW)
    value &= ~mask;
  else
    value |= mask;
  writeRegister(reg, value);
  return true;
}

/**
 * @brief set mode of GPIO pin to INPUT, INPUT_PULLUP or OUTPUT
 *
 * @param [in] pinnum Pin name between TCA8418_ROW0 and TCA8418_COL9  0..17
 * @param [in] mode   INPUT, INPUT_PULLUP or OUTPUT
 * @return  false if failed.
 */
bool Adafruit_TCA8418::pinMode(uint8_t pinnum, uint8_t mode) {
  if (pinnum > TCA8418_COL9)
    return false;
  // if (mode > INPUT_PULLUP) return false; ?s

  uint8_t idx = pinnum / 8;
  uint8_t reg = TCA8418_REG_GPIO_DIR_1 + idx;
  uint8_t mask = (1 << (pinnum % 8));

  // MODE  0 = INPUT   1 = OUTPUT
  uint8_t value = readRegister(reg);
  if (mode == OUTPUT)
    value |= mask;
  else
    value &= ~mask;
  writeRegister(reg, value);

  // PULLUP  0 = enabled   1 = disabled
  reg = TCA8418_REG_GPIO_PULL_1 + idx;
  value = readRegister(reg);
  if (mode == INPUT_PULLUP)
    value &= ~mask;
  else
    value |= mask;
  writeRegister(reg, value);

  return true;
}

/**
 * @brief set IRQ mode of GPIO pin to FALLING RISING
 *
 * @param [in] pinnum  Pin name between TCA8418_ROW0 and TCA8418_COL9  0..17
 * @param [in] mode    IRQ mode   FALLING RISING
 * @return  false if failed.
 */
bool Adafruit_TCA8418::pinIRQMode(uint8_t pinnum, uint8_t mode) {
  if (pinnum > TCA8418_COL9)
    return false;
  if ((mode != RISING) && (mode != FALLING))
    return false;

  //  MODE  0 = FALLING   1 = RISING
  uint8_t idx = pinnum / 8;
  uint8_t reg = TCA8418_REG_GPIO_INT_LVL_1 + idx;
  uint8_t mask = (1 << (pinnum % 8));

  uint8_t value = readRegister(reg);
  if (mode == RISING)
    value |= mask;
  else
    value &= ~mask;
  writeRegister(reg, value);

  // ENABLE INTERRUPT
  reg = TCA8418_REG_GPIO_INT_EN_1 + idx;
  value = readRegister(reg);
  value |= mask;
  writeRegister(reg, value);

  return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURATION
//

/**
 * @brief enables key event + GPIO interrupts.
 */
void Adafruit_TCA8418::enableInterrupts() {
  uint8_t value = readRegister(TCA8418_REG_CFG);
  value |= (TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
  writeRegister(TCA8418_REG_CFG, value);
};

/**
 * @brief disables key events + GPIO interrupts.
 */
void Adafruit_TCA8418::disableInterrupts() {
  uint8_t value = readRegister(TCA8418_REG_CFG);
  value &= ~(TCA8418_REG_CFG_GPI_IEN | TCA8418_REG_CFG_KE_IEN);
  writeRegister(TCA8418_REG_CFG, value);
};

/**
 * @brief enables matrix overflow interrupt.
 */
void Adafruit_TCA8418::enableMatrixOverflow() {
  uint8_t value = readRegister(TCA8418_REG_CFG);
  value |= TCA8418_REG_CFG_OVR_FLOW_M;
  writeRegister(TCA8418_REG_CFG, value);
};

/**
 * @brief disables matrix overflow interrupt.
 */
void Adafruit_TCA8418::disableMatrixOverflow() {
  uint8_t value = readRegister(TCA8418_REG_CFG);
  value &= ~TCA8418_REG_CFG_OVR_FLOW_M;
  writeRegister(TCA8418_REG_CFG, value);
};

/**
 * @brief enables key debounce.
 */
void Adafruit_TCA8418::enableDebounce() {
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_1, 0x00);
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_2, 0x00);
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_3, 0x00);
}

/**
 * @brief disables key debounce.
 */
void Adafruit_TCA8418::disableDebounce() {
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_1, 0xFF);
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_2, 0xFF);
  writeRegister(TCA8418_REG_DEBOUNCE_DIS_3, 0xFF);
}

/////////////////////////////////////////////////////////////////////////////
//
//  LOW LEVEL
//

/**
 * @brief reads byte value from register
 *
 * @param [in] reg register address
 * @return value from register
 */
uint8_t Adafruit_TCA8418::readRegister(uint8_t reg) {
  Adafruit_I2CRegister i2cReg = Adafruit_I2CRegister(i2c_dev, reg);
  uint8_t buffer[1] = {0};
  i2cReg.read(buffer, 1);
  return buffer[0];
}

/**
 * @brief write byte value to register
 *
 * @param [in] reg register address
 * @param [in] value
 */
void Adafruit_TCA8418::writeRegister(uint8_t reg, uint8_t value) {
  Adafruit_I2CRegister i2cReg = Adafruit_I2CRegister(i2c_dev, reg);
  i2cReg.write(value);
}
