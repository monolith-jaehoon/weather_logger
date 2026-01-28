#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t loki_client_init(const char *url, const char *weather_label);
esp_err_t loki_client_send(float temperature, float humidity, float pressure, bool humidity_supported);
