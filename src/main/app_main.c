
#include <stdio.h>
#include <string.h>

#include "network_mngr.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "gpio.h"
#include "counter.h"
#include "timesync.h"
#include "sinkSender.h"

const char VERSION[] = "deadbeef;


// static const char *TAG = "app";

void deviceInit() {
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}

void app_main(void)
{
    deviceInit();

    // it is important to initialize the counter before the gpios
    counterInit();

    gpioInit();
    networkInit(isGpioForceProv());
    timesyncInit();

    sinksenderInit();

    /* Start main application now */
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
