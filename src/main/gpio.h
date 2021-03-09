#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdbool.h>


#define GPIO_FORCE_PROV 5


void gpioInit();
bool isGpioForceProv();

#endif // _GPIO_H_
