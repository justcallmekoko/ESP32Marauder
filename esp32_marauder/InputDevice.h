#pragma once

#ifndef InputDevice_h
#define InputDevice_h

#include "configs.h"

#ifdef HAS_OLED

// Abstract base — OledMenu depends only on this interface
class InputDevice {
public:
  virtual bool up()   = 0;  // rising edge: move selection up
  virtual bool down() = 0;  // rising edge: move selection down
  virtual bool back() = 0;  // rising edge: go back / stop scan
  virtual bool sel()  = 0;  // rising edge: confirm / enter submenu

  // Block until back() is no longer held (used after info screens)
  virtual void waitBackRelease() = 0;

  virtual ~InputDevice() {}
};

// ----------------------------------------------------------------
// Joystick implementation
// ----------------------------------------------------------------
#ifdef HAS_JOYSTICK

class JoystickInput : public InputDevice {
public:
  JoystickInput()
    : _up_last(false), _down_last(false),
      _back_last(false), _sw_last(false) {}

  void begin() {
    pinMode(JOY_SW_PIN, INPUT_PULLUP);
  }

  bool up() override {
    bool cur = analogRead(JOY_Y_PIN) < (2048 - JOY_THRESHOLD);
    bool fired = cur && !_up_last;
    _up_last = cur;
    return fired;
  }

  bool down() override {
    bool cur = analogRead(JOY_Y_PIN) > (2048 + JOY_THRESHOLD);
    bool fired = cur && !_down_last;
    _down_last = cur;
    return fired;
  }

  bool back() override {
    bool cur = analogRead(JOY_X_PIN) < (2048 - JOY_THRESHOLD);
    bool fired = cur && !_back_last;
    _back_last = cur;
    return fired;
  }

  bool sel() override {
    bool cur = (digitalRead(JOY_SW_PIN) == LOW);
    bool fired = cur && !_sw_last;
    _sw_last = cur;
    return fired;
  }

  void waitBackRelease() override {
    while (analogRead(JOY_X_PIN) < (2048 - JOY_THRESHOLD)) delay(10);
  }

private:
  bool _up_last, _down_last, _back_last, _sw_last;
};

#endif // HAS_JOYSTICK

// ----------------------------------------------------------------
// Button implementation
// ----------------------------------------------------------------
#ifdef HAS_BUTTONS

#include "Switches.h"
extern Switches u_btn;
extern Switches d_btn;
extern Switches c_btn;
extern Switches l_btn;

class ButtonInput : public InputDevice {
public:
  bool up()   override { return u_btn.justPressed(); }
  bool down() override { return d_btn.justPressed(); }
  bool back() override { return l_btn.justPressed(); }
  bool sel()  override { return c_btn.justPressed(); }

  void waitBackRelease() override {
    while (!l_btn.justPressed()) delay(10);
  }
};

#endif // HAS_BUTTONS

#endif // HAS_OLED
#endif // InputDevice_h
