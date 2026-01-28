#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef void (*sensor_task_callback_t)(float temperature,
									  float humidity,
									  float pressure,
									  bool humidity_supported);

void sensor_task_init(uint32_t interval_ms,
					  uint32_t initial_delay_ms,
					  int sda_gpio,
					  int scl_gpio);
void sensor_task_start(void);
void sensor_task_set_callback(sensor_task_callback_t callback);
