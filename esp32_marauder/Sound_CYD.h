
#ifndef Sound_CYD_H
#define Sound_CYD_H

#include "configs.h"
#include "settings.h"

#include <Arduino.h>
#include "driver/ledc.h"

#ifndef SOUND_PIN
    #define SOUND_PIN 26
#endif

extern Settings settings_obj;

class Sound_CYD {

    public:
	void doit(int f, int = 500);
	void RunSetup();
	// void alert();
	// void tic();
	// void tok();
	// void beep();
	void click();
	void geigerClick(uint8_t i = 0);

    // void s_power_on();
    // void s_beep();
    // void s_ready();
    // void s_ready_2();
    // void s_error();
    // void s_error_2();

    void gen_tone(uint32_t f, uint32_t t);
    void stop_tone();

	uint32_t duty_cycle;
};

#endif  /* Sound_CYD_H */
