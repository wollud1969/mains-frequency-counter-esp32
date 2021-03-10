#include <stdint.h>

#include "counter.h"
#include "timesync.h"

#include <driver/timer.h>
#include <esp_log.h>
#include <esp_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "cnt";


static xQueueHandle zeroCrossingQueue = NULL;
static const uint64_t QUEUE_MARKER = UINT64_MAX;


static void counterSecondTask(void *arg) {
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        xQueueSend(zeroCrossingQueue, &QUEUE_MARKER, portMAX_DELAY);
    }
}

static void counterZeroCrossingAveragerTask(void *arg) {
    uint64_t counterCnt = 0;
    uint64_t counterSum = 0;
    uint64_t savedCounterValue = 0;
    while (1) {
        uint64_t currentCounterValue;
        xQueueReceive(zeroCrossingQueue, &currentCounterValue, portMAX_DELAY);
        if (currentCounterValue == QUEUE_MARKER) {
            if (counterCnt > 0) {
                uint32_t counterSecondAverage = ((uint32_t)(counterSum)) / ((uint32_t)(counterCnt));
                int ts = timesyncReady();
                ESP_LOGI(TAG, "%d %u %u %u", ts, (uint32_t)counterCnt, (uint32_t)counterSum, counterSecondAverage);
            } else {
                ESP_LOGW(TAG, "counterCnt is zero");
            }
            counterCnt = 0;
            counterSum = 0;
        } else {
            uint64_t period = (savedCounterValue < currentCounterValue) ?
                              (currentCounterValue - savedCounterValue) : 
                              ((UINT64_MAX - savedCounterValue) + currentCounterValue);
            savedCounterValue = currentCounterValue;
            counterCnt += 1;
            counterSum += period;
        }
    }
}

void IRAM_ATTR counterZeroCrossingISR(void *arg) {
    uint64_t counterValue = 0;
    
    timer_spinlock_take(TIMER_GROUP_0);
    counterValue = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, 0);
    timer_spinlock_give(TIMER_GROUP_0);

    if (zeroCrossingQueue) {
        xQueueSendFromISR(zeroCrossingQueue, &counterValue, NULL);
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
