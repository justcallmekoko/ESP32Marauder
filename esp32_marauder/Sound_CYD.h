
#ifndef Sound_CYD_H
#define Sound_CYD_H

#include "configs.h"
#include "settings.h"

#include <Arduino.h>

#ifndef SOUND_PIN
    #define SOUND_PIN 26
#endif

extern Settings settings_obj;

class Sound_CYD {

    public:
	void doit(double s, int = 500);
	void RunSetup();
	void alert();
	void tic();
	void tok();
	void beep();
	void click();
	void geigerClick(uint8_t i = 0);

	uint32_t duty_cycle;
};

#endif  /* Sound_CYD_H */
