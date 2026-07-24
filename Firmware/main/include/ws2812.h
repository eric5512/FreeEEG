#pragma once

#include <inttypes.h>
#include "esp_check.h"
#include "esp_log.h"


typedef struct {
  uint8_t r,g,b;
} ws2812_color_t;

extern const ws2812_color_t WS2812_RED;
extern const ws2812_color_t WS2812_GREEN;
extern const ws2812_color_t WS2812_BLUE;
extern const ws2812_color_t WS2812_WHITE;
extern const ws2812_color_t WS2812_BLACK;

void ws2812_init(uint8_t);
esp_err_t ws2812_set(const ws2812_color_t *color);
