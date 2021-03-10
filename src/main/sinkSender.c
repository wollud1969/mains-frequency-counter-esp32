#include "sinksender.h"
#include "sinkStruct.h"

#include <stdint.h>
#include <stdbool.h>


#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "ss";


extern xQueueHandle minuteBufferQueue;

static void sinksenderExecTask(void *arg) {
    while (1) {
        if (minuteBufferQueue) {
            static t_minuteBuffer minuteBuffer;
            if (xQueueReceive(minuteBufferQueue, &minuteBuffer, portMAX_DELAY) == pdPASS) {
                ESP_LOGI(TAG, "Got a buffer from queue");
            }
        }
    }
}

void sinksenderInit() {
    ESP_LOGI(TAG, "Initializing sink sender");

    xTaskCreate(sinksenderExec, "sinksender_exec_task", 4096, NULL, 5, NULL);
}
