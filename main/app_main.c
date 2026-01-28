#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "wifi_client.h"
#include "time_sync_task.h"
#include "loki_client.h"
#include "sensor_task.h"

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
#define LOKI_URL "http://loki.address:3100/loki/api/v1/push"
#define LOKI_LABEL_WEATHER "weather_label"
#define SENSOR_READ_INTERVAL_MS 60000
#define SENSOR_INITIAL_DELAY_MS 2000
#define SENSOR_I2C_SDA_GPIO GPIO_NUM_6
#define SENSOR_I2C_SCL_GPIO GPIO_NUM_7
#define SNTP_SYNC_INTERVAL_MS 3600000

#define TAG "app_main"

static void on_sensor_sample(float temperature,
                             float humidity,
                             float pressure,
                             bool humidity_supported)
{
    loki_client_send(temperature, humidity, pressure, humidity_supported);
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    else
    {
        ESP_ERROR_CHECK(err);
    }

    err = wifi_client_init(WIFI_SSID, WIFI_PASSWORD);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(err));
        return;
    }

    time_sync_init(SNTP_SYNC_INTERVAL_MS);

    err = loki_client_init(LOKI_URL, LOKI_LABEL_WEATHER);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Loki client init failed: %s", esp_err_to_name(err));
        return;
    }

    sensor_task_init(SENSOR_READ_INTERVAL_MS,
                     SENSOR_INITIAL_DELAY_MS,
                     SENSOR_I2C_SDA_GPIO,
                     SENSOR_I2C_SCL_GPIO);
    sensor_task_set_callback(on_sensor_sample);

    time_sync_task_start();
    sensor_task_start();
}
