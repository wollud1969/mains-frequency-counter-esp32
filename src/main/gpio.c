#include <stdbool.h>
#include <stdint.h>

#include "gpio.h"
#include "counter.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


static const char *TAG = "gpio";


static uint8_t ledBlinkLevel = 0;
static uint8_t ledLastState = 0;

static void gpioLedTask(void *arg) {
    static uint8_t cnt = 0;
    while (1) {
        if (ledBlinkLevel != 0) {
            if (cnt == ledBlinkLevel) {
                cnt = 0;
                ledLastState ^= 0x01;
                gpio_set_level(GPIO_BUILTIN_LED, ledLastState);
            } else {
                cnt += 1;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void gpioInit() {
    ESP_LOGI(TAG, "Initializing gpios");

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

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << GPIO_BUILTIN_LED);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_ZERO_CROSSING, counterZeroCrossingISR, NULL);

    xTaskCreate(gpioLedTask, "gpio_led_task", 1024, NULL, 5, NULL);
}

bool isGpioForceProv() {
    int r = gpio_get_level(GPIO_FORCE_PROV);
    ESP_LOGI(TAG, "forceProv pin is %d", r);
    return (r != 0);
}

void gpioLedOn() {
    ledBlinkLevel = 0;
    ledLastState = 1;
    gpio_set_level(GPIO_BUILTIN_LED, ledLastState);
}

void gpioLedOff() {
    ledBlinkLevel = 0;
    ledLastState = 0;
    gpio_set_level(GPIO_BUILTIN_LED, ledLastState);
}

void gpioLedBlink(uint8_t f) {
    ledBlinkLevel = f;
}
