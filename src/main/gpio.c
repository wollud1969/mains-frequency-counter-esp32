#include <stdbool.h>

#include "gpio.h"
#include "counter.h"

#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "gpio";


void gpioInit() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << GPIO_FORCE_PROV);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << GPIO_ZERO_CROSSING);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_ZERO_CROSSING, counterZeroCrossingISR, NULL);

    ESP_LOGI(TAG, "gpios configured");
}

bool isGpioForceProv() {
    int r = gpio_get_level(GPIO_FORCE_PROV);
    ESP_LOGI(TAG, "forceProv pin is %d", r);
    return (r != 0);
}