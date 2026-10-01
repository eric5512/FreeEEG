#pragma once

#include <inttypes.h>


void sdcard_mount(uint8_t sd_pin_clk, uint8_t sd_pin_cmd, uint8_t sd_pin_d0, uint8_t sd_pin_d1, uint8_t sd_pin_d2, uint8_t sd_pin_d3, uint8_t sd_pin_cd);
FILE* sdcard_new_file(void);
void sdcard_unmount(void);
