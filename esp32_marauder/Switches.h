#ifndef Switches_h
#define Switches_h

#include <Arduino.h>

class Switches {
	private:
		int pin;
		uint32_t hold_lim;
		uint32_t cur_hold;
		
		uint32_t hold_init = millis();
		
		bool isheld;
		
		bool pullup;
		
		bool pressed;

#ifdef XIAO_ESP32_S3_TOUCH
		// Debounce state: raw reads are filtered so contact bounce does not
		// register as many rapid presses (which made menu scrolling a "hair trigger").
		bool db_state = false;       // debounced logical state (true = pressed)
		bool db_last_raw = false;    // last raw sample
		uint32_t db_last_change = 0; // millis of last raw change
#endif

		bool getButtonState();
		
	public:
		Switches();
		
		Switches(int pin, uint32_t hold_lim, bool pullup);
		
		int getPin();
		bool getPullup();
		
		bool justPressed();
		bool justReleased();
		bool isHeld();
		bool isPressedNow();
};

#endif
