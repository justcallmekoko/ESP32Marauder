#include "Switches.h"

Switches::Switches() {
	this->pin = 0;
	this->pin = false;
	this->pressed = false;
	this->hold_lim = 2000;
	this->cur_hold = 0;
	this->isheld = false;
	
	pinMode(this->pin, INPUT);
	
	return;
}

Switches::Switches(int pin, uint32_t hold_lim, bool pullup) {
	this->pin = pin;
	this->pullup = pullup;
	this->pressed = false;
	this->hold_lim = hold_lim;
	this->cur_hold = 0;
	this->isheld = false;
	
  // ESP32 GPIO 34-39 are INPUT-ONLY pads — they do NOT have internal
  // pull-up or pull-down resistors. Attempting to enable PU/PD on them
  // triggers "gpio_pullup_en: GPIO number error (input-only pad...)"
#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C5) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32S3)
  // Classic ESP32: GPIO 34, 35, 36, 37, 38, 39 are input-only
  if (this->pin >= 34 && this->pin <= 39) {
    pinMode(this->pin, INPUT);   // No internal PU/PD available — use plain INPUT
  } else
#endif
  {
    if (pullup)
      pinMode(this->pin, INPUT_PULLUP);
    else
      pinMode(this->pin, INPUT_PULLDOWN);
  }
	
	return;
}

int Switches::getPin() {
	return this->pin;
}

bool Switches::getPullup() {
	return this->pullup;
}

bool Switches::isHeld() {
	return this->isheld;
}

bool Switches::getButtonState() {
	int buttonState = digitalRead(this->pin);
	
	if ((this->pullup) && (buttonState == LOW))
		return true;
	else if ((!this->pullup) && (buttonState == HIGH))
		return true;
	else
		return false;
}

bool Switches::justPressed() {
	bool btn_state = this->getButtonState();
	
	// Button was JUST pressed
	if (btn_state && !this->pressed) {
		this->hold_init = millis();
		this->pressed = btn_state;
		return true;
	}
	else if (btn_state) { // Button is STILL pressed
		// Check if button is held
		//Serial.println("cur_hold: " + (String)this->cur_hold);
		if ((millis() - this->hold_init) < this->hold_lim) {
			this->isheld = false;
		}
		else {
			this->isheld = true;
		}
		
		this->pressed = btn_state;
		return false;
	}
	else { // Button is not pressed
		this->pressed = btn_state;
		this->isheld = false;
		return false;
	}
}

bool Switches::justReleased() {
	bool btn_state = this->getButtonState();
	
	// Button was JUST released
	if (!btn_state && this->pressed) {
		this->isheld = false;
		this->pressed = btn_state;
		return true;
	}
	else { // Button is STILL released
		this->pressed = btn_state;
		return false;
	}
	
}