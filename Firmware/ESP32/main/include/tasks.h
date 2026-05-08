#pragma once

#include "freertos/FreeRTOS.h"

void task_afe(void *args);
void task_imu(void *args);
void task_write(void *args);
void task_recv(void *args);
void task_conf(void *args);
