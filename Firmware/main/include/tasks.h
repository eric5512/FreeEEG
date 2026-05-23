#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"

void tasks_init();

void task_afe(void *args);
void task_imu(void *args);
void task_write(void *args);
void task_config(void *args);
void task_recv(void *args);
