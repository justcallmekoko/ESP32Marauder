#pragma once
#include <stdint.h>
#ifdef MARAUDER_RETERMINAL_STICKY
namespace sticky {
void beginButtons();
void servicePowerButton();
bool takePowerSelect();
bool navigationDown(int pin);
void powerOff(uint32_t wakeAfterMs = 0);
}
#endif
