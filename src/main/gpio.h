#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>


#define GPIO_FORCE_PROV 5
#define GPIO_ZERO_CROSSING 26

void gpioInit();
bool isGpioForceProv();

#endif // _GPIO_H_
