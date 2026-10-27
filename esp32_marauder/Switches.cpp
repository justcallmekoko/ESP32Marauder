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
	
  if (pullup)
  	pinMode(this->pin, INPUT_PULLUP);
  else
    pinMode(this->pin, INPUT_PULLDOWN);
	
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

#ifdef XIAO_ESP32_S3_TOUCH
	bool raw;
	if ((this->pullup) && (buttonState == LOW))
		raw = true;
	else if ((!this->pullup) && (buttonState == HIGH))
		raw = true;
	else
		raw = false;

	// Time-based debounce: only accept a new logical state after it has been
	// stable for DEBOUNCE_MS. Filters mechanical contact bounce so one physical
	// press = one press event (and hold-to-repeat still works while held).
	const uint32_t DEBOUNCE_MS = 30;
	uint32_t now = millis();
	if (raw != this->db_last_raw) {
		this->db_last_raw = raw;
		this->db_last_change = now;
	}
	if ((uint32_t)(now - this->db_last_change) >= DEBOUNCE_MS) {
		this->db_state = raw;
	}
	return this->db_state;
#else
	if ((this->pullup) && (buttonState == LOW))
		return true;
	else if ((!this->pullup) && (buttonState == HIGH))
		return true;
	else
		return false;
#endif
}

bool Switches::isPressedNow() {
	return this->getButtonState();
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
