#include "sinkSender.h"
#include "sinkStruct.h"
#include "sha256.h"
#include "gpio.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <nvs.h>


static const char *TAG = "ss";
extern char VERSION[];


static const char SINK_SERVER[] = "sink.hottis.de";
static const uint16_t SINK_PORT = 20169;

static const char DEFAULT_DEVICE_ID[] = "MainsCnt0x";
static char deviceId[16];
static const char DEFAULT_SHAREDSECRET[] = "1234567890123456789012345678901";
static char sharedSecret[32];



extern xQueueHandle minuteBufferQueue;


static void sinksenderSend(t_minuteBuffer *minuteBuffer) {
    int64_t uptime = esp_timer_get_time() / 1e6 / 3600;
    minuteBuffer->s.totalRunningHours = (uint32_t) uptime;
    minuteBuffer->s.totalPowercycles = 0;
    minuteBuffer->s.totalWatchdogResets = 0;
    minuteBuffer->s.version = strtoll(VERSION, NULL, 16);

    memset(minuteBuffer->s.deviceId, 0, sizeof(minuteBuffer->s.deviceId));
    strcpy(minuteBuffer->s.deviceId, deviceId);

    memcpy(minuteBuffer->s.hash, sharedSecret, SHA256_BLOCK_SIZE);
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, minuteBuffer->b, sizeof(minuteBuffer->b));
    sha256_final(&ctx, minuteBuffer->s.hash);

    struct hostent *hptr = gethostbyname(SINK_SERVER);
    if (hptr) {
        if (hptr->h_addrtype == AF_INET) {
        char *sinkAddr = hptr->h_addr_list[0];
        ESP_LOGI(TAG, "sink addr: %d.%d.%d.%d", 
                sinkAddr[0], sinkAddr[1], sinkAddr[2], sinkAddr[3]);

        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd != -1) {
            struct sockaddr_in servaddr;
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(SINK_PORT);
            memcpy(&servaddr.sin_addr.s_addr, sinkAddr, 4);

            ssize_t res = sendto(sockfd, minuteBuffer->b, sizeof(minuteBuffer->b), 
                                0, (struct sockaddr*)&servaddr, 
                                sizeof(servaddr));
            ESP_LOGI(TAG, "%d octets sent", res);
            int rc = close(sockfd);
            if (rc == -1) {
                ESP_LOGW(TAG, "close on socket returns %s", strerror(errno));
            }
        } else {
            ESP_LOGE(TAG, "unable to get socket: %s", strerror(errno));
        }
        } else {
        ESP_LOGE(TAG, "unknown address type: %d", hptr->h_addrtype);
        }
    } else {
        ESP_LOGE(TAG, "sinkserver %s couldn't be resolved: %d", SINK_SERVER, h_errno);
    }
}

static void sinksenderExecTask(void *arg) {
    while (1) {
        if (minuteBufferQueue) {
            static t_minuteBuffer minuteBuffer;
            if (xQueueReceive(minuteBufferQueue, &minuteBuffer, portMAX_DELAY) == pdPASS) {
                gpioLedOff();
                ESP_LOGI(TAG, "Got a buffer from queue");
                sinksenderSend(&minuteBuffer);
                gpioLedOn();
            }
        }
    }
}

void sinksenderInit() {
    ESP_LOGI(TAG, "Initializing sink sender");

    ESP_LOGI(TAG, "About to load sink sender configuration");
    nvs_handle_t nvsHandle;
    if (ESP_OK != nvs_open("sink", NVS_READWRITE, &nvsHandle)) {
        ESP_LOGE(TAG, "Unable to open nvs namespace sink, use default values");
        strcpy(deviceId, DEFAULT_DEVICE_ID);
        strcpy(sharedSecret, DEFAULT_SHAREDSECRET);
    } else {
        size_t s;
        esp_err_t err;

        err = nvs_get_str(nvsHandle, "deviceId", NULL, &s);
        ESP_LOGI(TAG, "1. err: %d, len: %d", err, s);
        err = nvs_get_str(nvsHandle, "deviceId", deviceId, &s);
        ESP_LOGI(TAG, "2. err: %d, len: %d", err, s);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "deviceId: %s", deviceId);
        } else {
            strcpy(deviceId, DEFAULT_DEVICE_ID);
            ESP_LOGI(TAG, "deviceId not configured, use default");            
        }
        err = nvs_get_str(nvsHandle, "sharedSecret", NULL, &s);
        ESP_LOGI(TAG, "1. err: %d, len: %d", err, s);
        err = nvs_get_str(nvsHandle, "sharedSecret", sharedSecret, &s);
        ESP_LOGI(TAG, "2. err: %d, len: %d", err, s);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "sharedSecret: %s", sharedSecret);
        } else {
            strcpy(sharedSecret, DEFAULT_SHAREDSECRET);
            ESP_LOGI(TAG, "sharedSecret not configured, use default");            
        }
    }

    ESP_LOGI(TAG, "finally deviceId: %s", deviceId);
    ESP_LOGI(TAG, "finally sharedSecret: %s", sharedSecret);

    xTaskCreate(sinksenderExecTask, "sinksender_exec_task", 4096, NULL, 5, NULL);
}
