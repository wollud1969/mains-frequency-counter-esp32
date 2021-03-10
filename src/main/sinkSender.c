#include "sinkSender.h"
#include "sinkStruct.h"
#include "sha256.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <nvs.h>


static const char *TAG = "ss";
extern char VERSION[];

const uint32_t CONFIG_MAGIC = 0xaffe0001;

static const char DEFAULT_SINKSERVER[] = "sink.hottis.de";
static const uint16_t DEFAULT_SINKPORT = 20169;
static const char DEFAULT_DEVICE_ID[] = "MainsCnt0x";
static const char DEFAULT_SHAREDSECRET[] = "1234567890123456789012345678901";

typedef struct __attribute__((__packed__)) {
    uint32_t magic;
    char sinkServer[48];
    uint16_t sinkPort;
    char deviceId[16];
    char sharedSecret[32];
} sinksenderConfig_t;

sinksenderConfig_t config;


extern xQueueHandle minuteBufferQueue;


static void sinksenderSend(t_minuteBuffer *minuteBuffer) {
    minuteBuffer->s.totalRunningHours = 0;
    minuteBuffer->s.totalPowercycles = 0;
    minuteBuffer->s.totalWatchdogResets = 0;
    minuteBuffer->s.version = strtoll(VERSION, NULL, 16);

    memset(minuteBuffer->s.deviceId, 0, sizeof(minuteBuffer->s.deviceId));
    strcpy(minuteBuffer->s.deviceId, config.deviceId);

    memcpy(minuteBuffer->s.hash, config.sharedSecret, SHA256_BLOCK_SIZE);
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, minuteBuffer->b, sizeof(minuteBuffer->b));
    sha256_final(&ctx, minuteBuffer->s.hash);

    struct hostent *hptr = gethostbyname(config.sinkServer);
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
            servaddr.sin_port = htons(config.sinkPort);
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
        ESP_LOGE(TAG, "sinkserver %s couldn't be resolved: %d", config.sinkServer, h_errno);
    }
}

static void sinksenderExecTask(void *arg) {
    while (1) {
        if (minuteBufferQueue) {
            static t_minuteBuffer minuteBuffer;
            if (xQueueReceive(minuteBufferQueue, &minuteBuffer, portMAX_DELAY) == pdPASS) {
                ESP_LOGI(TAG, "Got a buffer from queue");
                sinksenderSend(&minuteBuffer);
            }
        }
    }
}

void sinksenderInit() {
    ESP_LOGI(TAG, "Initializing sink sender");

    ESP_LOGI(TAG, "About to load sink sender configuration");
    nvs_handle_t nvsHandle;
    if (ESP_OK != nvs_open("config", NVS_READWRITE, &nvsHandle)) {
        ESP_LOGE(TAG, "Unable to open nvs namespace config, use default values");
        strcpy(config.sinkServer, DEFAULT_SINKSERVER);
        config.sinkPort = DEFAULT_SINKPORT;
        strcpy(config.deviceId, DEFAULT_DEVICE_ID);
        strcpy(config.sharedSecret, DEFAULT_SHAREDSECRET);
    } else {
        size_t s;
        esp_err_t err = nvs_get_blob(nvsHandle, "sinkSender", (void*)&config, &s);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "sinkSender configuration loaded");
        } else {
            ESP_LOGE(TAG, "Get result is %d", err);
        }
        if (config.magic != CONFIG_MAGIC) {
            ESP_LOGW(TAG, "No configuration found, write it");
            config.magic = CONFIG_MAGIC;
            strcpy(config.sinkServer, DEFAULT_SINKSERVER);
            config.sinkPort = DEFAULT_SINKPORT;
            strcpy(config.deviceId, DEFAULT_DEVICE_ID);
            strcpy(config.sharedSecret, DEFAULT_SHAREDSECRET);
            err = nvs_set_blob(nvsHandle, "sinkSender", (void*)&config, sizeof(config));
            ESP_LOGW(TAG, "Set result is %d", err);
            err = nvs_commit(nvsHandle);
            ESP_LOGW(TAG, "Commit result is %d", err);
        }
    }

    xTaskCreate(sinksenderExecTask, "sinksender_exec_task", 4096, NULL, 5, NULL);
}
