#pragma once

#include <stdbool.h>
#include <inttypes.h>

void RGB_init(uint8_t pin_r, uint8_t pin_g, uint8_t pin_b);
void RGB_set(bool r, bool g, bool b);
