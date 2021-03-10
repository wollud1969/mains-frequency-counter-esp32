#include "timesync.h"
#include "gpio.h"
#include <stdbool.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "ts";

static bool synchronized = false;

void timesyncCallback(struct timeval *tv) {
    struct timespec timestamp;
    clock_gettime(CLOCK_REALTIME, &timestamp);
    uint32_t current_seconds = timestamp.tv_sec;
    uint32_t current_milliseconds = timestamp.tv_nsec / 1e6;

    ESP_LOGI(TAG, "time is synchronized now: %u s %u ms", current_seconds, current_milliseconds);

    if (!synchronized) {
        gpioLedBlink(10);
        synchronized = true;
    }
}

void timesyncInit() {
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, SNTP_SERVER);
    sntp_set_time_sync_notification_cb(timesyncCallback);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_sync_interval(60*1000); // 1 minute
    sntp_init();
}

bool timesyncReady() {
    return synchronized;
}

uint32_t timesyncGetCurrentSeconds() {
    struct timespec timestamp;
    clock_gettime(CLOCK_REALTIME, &timestamp);
    return timestamp.tv_sec;
}