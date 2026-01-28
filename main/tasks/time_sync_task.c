#include "time_sync_task.h"

#include <sys/time.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_timer.h"

#define TAG "time_sync_task"
static volatile bool s_time_synced = false;
static int64_t s_time_base_us = 0;
static int64_t s_time_base_uptime_us = 0;
static uint32_t s_sync_interval_ms = 3600000;

static void time_sync_cb(struct timeval *tv)
{
    if (tv)
    {
        s_time_base_us = (int64_t)tv->tv_sec * 1000000LL + (int64_t)tv->tv_usec;
        s_time_base_uptime_us = esp_timer_get_time();
        s_time_synced = true;
        ESP_LOGI(TAG, "SNTP synced: %ld.%06ld", (long)tv->tv_sec, (long)tv->tv_usec);

        time_t now = (time_t)tv->tv_sec;
        struct tm timeinfo;
        char time_buf[32];
        localtime_r(&now, &timeinfo);
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(TAG, "SNTP synced localtime: %s", time_buf);
    }
}

static void time_sync_wait(void)
{
    int retry = 0;
    const int retry_count = 10;
    while (!s_time_synced && ++retry <= retry_count)
    {
        ESP_LOGI(TAG, "Waiting for SNTP sync (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void time_sync_task(void *arg)
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(s_sync_interval_ms));
        s_time_synced = false;
        esp_sntp_restart();
        time_sync_wait();
    }
}

void time_sync_init(uint32_t sync_interval_ms)
{
    s_time_synced = false;
    if (sync_interval_ms > 0)
    {
        s_sync_interval_ms = sync_interval_ms;
    }
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_cb);
    esp_sntp_init();
    time_sync_wait();
}

void time_sync_task_start(void)
{
    xTaskCreate(time_sync_task, TAG, 4096, NULL, 4, NULL);
}

bool time_sync_is_synced(void)
{
    return s_time_synced;
}

int64_t time_sync_get_epoch_ns(void)
{
    if (!s_time_synced)
    {
        return 0;
    }

    int64_t now_us = s_time_base_us + (esp_timer_get_time() - s_time_base_uptime_us);
    return now_us * 1000LL;
}
