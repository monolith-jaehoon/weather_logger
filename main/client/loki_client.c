#include "loki_client.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "esp_http_client.h"
#include "esp_log.h"

#include "time_sync_task.h"

#define TAG "loki_client"
static char s_loki_url[192];
static char s_weather_label[64];
static bool s_initialized = false;

#define LINE_BUF_SIZE 128
#define TS_BUF_SIZE 32
#define PAYLOAD_BUF_SIZE 1024
static char s_line_buf[LINE_BUF_SIZE];
static char s_ts_buf[TS_BUF_SIZE];
static char s_payload_buf[PAYLOAD_BUF_SIZE];


esp_err_t loki_client_init(const char *url, const char *weather_label)
{
    if (url == NULL || weather_label == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(s_loki_url, url, sizeof(s_loki_url));
    s_loki_url[sizeof(s_loki_url) - 1] = '\0';

    strncpy(s_weather_label, weather_label, sizeof(s_weather_label));
    s_weather_label[sizeof(s_weather_label) - 1] = '\0';

    s_initialized = true;
    return ESP_OK;
}

esp_err_t loki_client_send(float temperature, float humidity, float pressure, bool humidity_supported)
{
    if (!s_initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    if (!time_sync_is_synced())
    {
        ESP_LOGW(TAG, "Time not synced yet, skip send");
        return ESP_FAIL;
    }

    int64_t ts_ns = time_sync_get_epoch_ns();
    if (ts_ns == 0)
    {
        ESP_LOGW(TAG, "Invalid time, skip send");
        return ESP_FAIL;
    }

    if (humidity_supported)
    {
        snprintf(s_line_buf, LINE_BUF_SIZE, "temperature=%.2f,humidity=%.2f,pressure=%.2f", temperature, humidity, pressure);
    }
    else
    {
        snprintf(s_line_buf, LINE_BUF_SIZE, "temperature=%.2f,humidity=,pressure=%.2f", temperature, pressure);
    }

    snprintf(s_ts_buf, TS_BUF_SIZE, "%" PRId64, ts_ns);

    const char *ts_str = s_ts_buf;
    int len = snprintf(s_payload_buf, PAYLOAD_BUF_SIZE,
                       "{\"streams\":[{\"stream\":{\"weather\":\"%s\"},\"values\":[[\"%s\",\"%s\"]]}]}",
                       s_weather_label, ts_str, s_line_buf);

    esp_http_client_config_t config = {
        .url = s_loki_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "HTTP client init failed (no mem)");
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGD(TAG, "Loki payload: %s", s_payload_buf);
    esp_http_client_set_post_field(client, s_payload_buf, len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "Loki response status = %d", status);

        if (status < 200 || status >= 300)
        {
            char resp_buf[256] = {0};
            int read_len = esp_http_client_read_response(client, resp_buf, sizeof(resp_buf) - 1);
            if (read_len >= 0)
            {
                ESP_LOGE(TAG, "Loki error payload: %s", s_payload_buf);
                ESP_LOGE(TAG, "Loki response body: %s", resp_buf);
            }
            else
            {
                ESP_LOGE(TAG, "Loki error payload: %s", s_payload_buf);
                ESP_LOGE(TAG, "Loki response body read failed");
            }
        }
    }
    else
    {
        ESP_LOGE(TAG, "Loki send failed: %s", esp_err_to_name(err));
        ESP_LOGE(TAG, "Loki error payload: %s", s_payload_buf);
    }

    esp_http_client_cleanup(client);
    return err;
}
