#include "sensor_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "bmp280.h"
#include "i2cdev.h"

// I2C defaults (configurable via sensor_task_init for SDA/SCL only)
#define SENSOR_I2C_PORT I2C_NUM_0
#define SENSOR_I2C_FREQ_HZ 100000
#define SENSOR_I2C_ADDR BMP280_I2C_ADDRESS_0

static gpio_num_t s_i2c_sda_gpio = GPIO_NUM_6;
static gpio_num_t s_i2c_scl_gpio = GPIO_NUM_7;
static uint32_t s_read_interval_ms = 10000;
static uint32_t s_initial_delay_ms = 2000;

#define TAG "sensor_task"
static bmp280_t s_bmp;
static bool s_humidity_supported = false;
static bool s_initialized = false;
static sensor_task_callback_t s_callback = NULL;

static void sensor_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(s_initial_delay_ms));

    while (true)
    {
        float temperature = 0.0f;
        float pressure = 0.0f;
        float humidity = 0.0f;

        esp_err_t err = bmp280_read_float(&s_bmp, &temperature, &pressure, &humidity);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Sensor read failed: %s", esp_err_to_name(err));
        }
        else
        {
            if (s_humidity_supported)
            {
                ESP_LOGI(TAG, "Temp: %.2f C | Humidity: %.2f %% | Pressure: %.2f Pa",
                         temperature, humidity, pressure);
            }
            else
            {
                ESP_LOGI(TAG, "Temp: %.2f C | Humidity: N/A | Pressure: %.2f Pa",
                         temperature, pressure);
            }

            if (s_callback)
            {
                s_callback(temperature, humidity, pressure, s_humidity_supported);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(s_read_interval_ms));
    }
}

void sensor_task_init(uint32_t interval_ms,
                      uint32_t initial_delay_ms,
                      int sda_gpio,
                      int scl_gpio)
{
    if (interval_ms > 0)
    {
        s_read_interval_ms = interval_ms;
    }
    if (initial_delay_ms > 0)
    {
        s_initial_delay_ms = initial_delay_ms;
    }
    s_i2c_sda_gpio = (gpio_num_t)sda_gpio;
    s_i2c_scl_gpio = (gpio_num_t)scl_gpio;
    ESP_ERROR_CHECK(i2cdev_init());

    bmp280_params_t params;
    ESP_ERROR_CHECK(bmp280_init_default_params(&params));

    params.mode = BMP280_MODE_NORMAL;
    params.filter = BMP280_FILTER_4;
    params.oversampling_pressure = BMP280_HIGH_RES;
    params.oversampling_temperature = BMP280_HIGH_RES;
    params.oversampling_humidity = BMP280_HIGH_RES;
    params.standby = BMP280_STANDBY_250;

    ESP_ERROR_CHECK(bmp280_init_desc(&s_bmp, SENSOR_I2C_ADDR, SENSOR_I2C_PORT,
                                     s_i2c_sda_gpio, s_i2c_scl_gpio));
    s_bmp.i2c_dev.cfg.master.clk_speed = SENSOR_I2C_FREQ_HZ;
    s_bmp.i2c_dev.cfg.sda_pullup_en = true;
    s_bmp.i2c_dev.cfg.scl_pullup_en = true;

    esp_err_t err = bmp280_init(&s_bmp, &params);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "BMP280 init failed: %s", esp_err_to_name(err));
        s_initialized = false;
        return;
    }

    s_humidity_supported = (s_bmp.id == BME280_CHIP_ID);
    if (!s_humidity_supported)
    {
        ESP_LOGW(TAG, "Sensor is BMP280. Humidity is not supported.");
    }

    s_initialized = true;
}

void sensor_task_set_callback(sensor_task_callback_t callback)
{
    s_callback = callback;
}

void sensor_task_start(void)
{
    if (!s_initialized)
    {
        ESP_LOGE(TAG, "Sensor task not initialized");
        return;
    }

    xTaskCreate(sensor_task, TAG, 12288, NULL, 5, NULL);
}
