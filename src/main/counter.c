#include <stdint.h>

#include "timer.h"

#include <driver/timer.h>
#include <esp_log.h>
#include <esp_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "cnt";


static xQueueHandle zeroCrossingQueue = NULL;
static const uint64_t QUEUE_MARKER = UINT64_MAX;


static void counterTask(void *arg) {
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xQueueSend(zeroCrossingQueue, QUEUE_MARKER, portMAX_DELAY);
    }
}

static void counterZeroCrossingAveragerTask(void *arg) {
    uint64_t counterCnt = 0;
    uint64_t counterSum = 0;
    while (1) {
        uint64_t counterCurrentValue;
        xQueueReceive(zeroCrossingQueue, &counterCurrentValue, portMAX_DELAY);
        if (counterCurrentValue == QUEUE_MARKER) {
            uint32_t counterSecondAverage = ((uint32_t)(counterSum)) / ((uint32_t)(counterCnt));
            ESP_LOGI(TAG, "second average is %lu", counterSecondAverage);
        } else {
            counterSum += 1;
            counterCnt += counterValue;
        }
    }
}

void IRAM_ATTR counterZeroCrossingISR(void *arg) {
    uint64_t counterValue = 0;
    
    timer_spinlock_take(TIMER_GROUP_0);
    counterValue = time_group_get_counter_value_in_isr(TIMER_GROUP_0, 0);
    timer_spinlock_give(TIMER_GROUP_0);

    if (zeroCrossingQueue) {
        xQueueSendFromISR(zeroCrossingQueue, counterValue, NULL);
    }
}

void counterInit() {
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

    zeroCrossingQueue = xQueueCreate(20, sizeof(uint64_t));

    xTaskCreate(counterSecondTask, "counter_second_task", 2048, NULL, 5, NULL);
    xTaskCreate(counterZeroCrossingAveragerTask, "counter_averager_task", 2048, NULL, 5, NULL);
}
