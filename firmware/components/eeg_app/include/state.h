#pragma once
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"

extern EventGroupHandle_t system_events;
extern RingbufHandle_t rb_handle;

#define WIFI_ACTIVE    (1 << 0)
#define OFFLINE_ACTIVE (1 << 1)
