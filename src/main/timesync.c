#include "timesync.h"
#include <stdbool.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "ts";

static bool synchronized = false;

void timesyncCallback(struct timeval *tv) {
    ESP_LOGI(TAG, "time is synchronized now");
    synchronized = true;
}

void timesyncInit() {
    ESP_LOGI(TAG, "Initializiing SNTP");
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
