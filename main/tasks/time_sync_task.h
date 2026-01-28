#pragma once

#include <stdbool.h>
#include <stdint.h>

void time_sync_init(uint32_t sync_interval_ms);
void time_sync_task_start(void);
bool time_sync_is_synced(void);
int64_t time_sync_get_epoch_ns(void);
