#pragma once

#include "config.h"
#include <inttypes.h>
#include <sys/time.h>

typedef struct {
  struct timeval timestamp;
  uint8_t status[3];
  uint32_t channel[8];
} ads1299_data_t;

void ads1299_init(uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_sck, uint8_t pin_cs, uint8_t pin_drdy, uint8_t pin_start, void (*drdy_callback)(void));
void ads1299_send_config(const config_t* conf);
void ads1299_start(void);
void ads1299_stop(void);
void ads1299_read(ads1299_data_t* data);
