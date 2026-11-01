#pragma once
#include <stdint.h>
#ifdef MARAUDER_M5PAPER
namespace m5paper {
void beginButtons();
void servicePowerButton();
bool takePowerSelect();
bool navigationDown(int pin);
void powerOff(uint32_t wakeAfterMs = 0);
}
#endif
