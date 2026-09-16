#pragma once

#include "freertos/FreeRTOS.h"
#include "stddef.h"

void ics43434_init(uint8_t, uint8_t, uint8_t);
void ics43434_start();
void ics43434_stop();
size_t ics43434_read(uint32_t*, size_t);
