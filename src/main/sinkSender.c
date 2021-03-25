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

static const char SINK_SERVER[] = "sink.hottis.de";
static const uint16_t SINK_PORT = 20169;

static const char DEFAULT_DEVICE_ID[] = "MainsCnt0x";
static char deviceId[16];
static const char DEFAULT_SHAREDSECRET[] = "1234567890123456789012345678901";
static char sharedSecret[32];

extern const uint32_t APP_VERSION;


extern xQueueHandle minuteBufferQueue;


static void sinksenderSend(t_minuteBuffer *minuteBuffer) {
    int64_t uptime = esp_timer_get_time() / 1e6 / 3600;
    minuteBuffer->s.totalRunningHours = (uint32_t) uptime;
    minuteBuffer->s.totalPowercycles = 0;
    minuteBuffer->s.totalWatchdogResets = 0;
    minuteBuffer->s.version = APP_VERSION;

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

    uint8_t macAddress[6];
    if (ESP_OK == esp_efuse_mac_get_default(macAddress)) {
        sprintf(deviceId, "%02x%02x%02x%02x%02x%02x", 
                macAddress[0], macAddress[1], macAddress[2],
                macAddress[3], macAddress[4], macAddress[5]);
    } else {
        ESP_LOGE(TAG, "can not read mac address, use default device id");
        strcpy(deviceId, DEFAULT_DEVICE_ID);
    }

    nvs_handle_t nvsHandle;
    if (ESP_OK != nvs_open("sink", NVS_READWRITE, &nvsHandle)) {
        ESP_LOGE(TAG, "Unable to open nvs namespace sink, use default values");
        strcpy(sharedSecret, DEFAULT_SHAREDSECRET);
    } else {
        size_t s;
        esp_err_t err;

        err = nvs_get_str(nvsHandle, "sharedSecret", sharedSecret, &s);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "sharedSecret not configured, create one");
            memset(sharedSecret, 0, sizeof(sharedSecret));
            esp_fill_random(sharedSecret, sizeof(sharedSecret)-1);
            for (uint8_t i = 0; i < sizeof(sharedSecret)-1; i++) {
                sharedSecret[i] = (sharedSecret[i] % 90) + 33; // 0 .. 255 -> 33 .. 122
                if (sharedSecret[i] == 34) { // "
                    sharedSecret[i] = 123;   // {
                } 
                if (sharedSecret[i] == 39) { // '
                    sharedSecret[i] = 124;   // |
                } 
                if (sharedSecret[i] == 92) { // }
                    sharedSecret[i] = 125;   // 
                } 
                if (sharedSecret[i] == 96) { // `
                    sharedSecret[i] = 126;   // ~
                } 
            }
            ESP_LOGI(TAG, "generated sharedSecret is %s", sharedSecret);
            if (ESP_OK == nvs_set_str(nvsHandle, "sharedSecret", sharedSecret)) {
                ESP_LOGI(TAG, "new sharedSecret stored");
            } else {
                ESP_LOGE(TAG, "unable to store sharedSecret");
            }
        }
    }

    ESP_LOGI(TAG, "finally deviceId: %s", deviceId);
    ESP_LOGI(TAG, "finally sharedSecret: [masked]");

    xTaskCreate(sinksenderExecTask, "sinksender_exec_task", 4096, NULL, 5, NULL);
}
