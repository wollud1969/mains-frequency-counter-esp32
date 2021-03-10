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
#include <sys/sysinfo.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static const char *TAG = "ss";
extern char VERSION[];


extern xQueueHandle minuteBufferQueue;


static void sinksenderSend(t_minuteBuffer *minuteBuffer) {
    struct sysinfo info;
    sysinfo(&info);

    minuteBuffer.s.totalRunningHours = info.uptime / 3600;
    minuteBuffer.s.totalPowercycles = 0;
    minuteBuffer.s.totalWatchdogResets = 0;
    minuteBuffer.s.version = strtoll(VERSION, NULL, 16);

    memset(minuteBuffer.s.deviceId, 0, sizeof(minuteBuffer.s.deviceId));
    strcpy(minuteBuffer.s.deviceId, deviceId);

    memcpy(minuteBuffer.s.hash, sharedSecret, SHA256_BLOCK_SIZE);
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, minuteBuffer.b, sizeof(minuteBuffer.b));
    sha256_final(&ctx, minuteBuffer.s.hash);

    struct hostent *hptr = gethostbyname(sinkServer);
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
            servaddr.sin_port = htons(sinkPort);
            memcpy(&servaddr.sin_addr.s_addr, sinkAddr, 4);

            ssize_t res = sendto(sockfd, minuteBuffer.b, sizeof(minuteBuffer.b), 
                                0, (struct sockaddr*)&servaddr, 
                                sizeof(servaddr));
            ESP_LOGI(TAG, "%d octets sent", res);
        } else {
            ESP_LOGE(TAG, "unable to get socket: %s", strerror(errno));
        }
        } else {
        ESP_LOGE(TAG, "unknown address type: %d", hptr->h_addrtype);
        }
    } else {
        ESP_LOGE(TAG, "sinkserver %s couldn't be resolved: %s", sinkServer, hstrerror(h_errno));
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

    xTaskCreate(sinksenderExecTask, "sinksender_exec_task", 4096, NULL, 5, NULL);
}
