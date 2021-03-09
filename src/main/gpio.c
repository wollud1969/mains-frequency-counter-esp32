#include <stdbool.h>

#include "gpio.h"

#include <driver/gpio.h>



void gpioInit() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << GPIO_FORCE_PROV);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

bool isGpioForceProv() {
    return 0 == gpio_get_level(GPIO_FORCE_PROV);
}