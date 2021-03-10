#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#define GPIO_BUILTIN_LED 2
#define GPIO_FORCE_PROV 5
#define GPIO_ZERO_CROSSING 26

void gpioInit();
bool isGpioForceProv();
void gpioLedOn();
void gpioLedOff();
void gpioLedBlink(uint8_t f);
     
#endif // _GPIO_H_
