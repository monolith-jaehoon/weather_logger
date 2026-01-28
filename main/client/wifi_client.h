#pragma once

#include "esp_err.h"

esp_err_t wifi_client_init(const char *ssid, const char *password);
