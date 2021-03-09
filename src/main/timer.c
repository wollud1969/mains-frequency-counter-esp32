#include <stdint.h>

#include "timer.h"

#include <driver/timer.h>
#include <esp_log.h>
#include <esp_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "tm";



static void timer_task(void *arg) {
    while (1) {
        uint64_t counterValue;
        timer_get_counter_value(TIMER_GROUP_0, 0, &counterValue);
        ESP_LOGI(TAG, "timer value: %08x %08x", (uint32_t)(counterValue >> 32), (uint32_t)(counterValue));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void timerInit() {
    timer_config_t config = {
        .divider = 80,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = TIMER_AUTORELOAD_EN
    };
    timer_init(TIMER_GROUP_0, 0, &config);
    timer_set_counter_value(TIMER_GROUP_0, 0, 0);
    timer_start(TIMER_GROUP_0, 0);

    xTaskCreate(timer_task, "timer_task", 2048, NULL, 5, NULL);
}
