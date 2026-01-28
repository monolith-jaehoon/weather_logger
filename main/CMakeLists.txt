idf_component_register(SRCS "app_main.c"
                       "tasks/time_sync_task.c"
                       "tasks/sensor_task.c"
                       "client/loki_client.c"
                       "client/wifi_client.c"
                       PRIV_REQUIRES spi_flash esp_driver_i2c esp_wifi esp_event esp_netif nvs_flash esp_http_client esp_timer
                       INCLUDE_DIRS "." "tasks" "client")
