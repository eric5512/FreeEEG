#pragma once

#include "config.h"
#include <inttypes.h>
#include "freertos/ringbuf.h"

/* typedef struct { */
/*   uint8_t active[8]; */
/*   uint24_t data[8]; */
/* } ads1299_data_t; */

void ads1299_init(uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_sck, uint8_t pin_cs);
void ads1299_send_config(const config_t* conf);
void ads1299_start(void);
void ads1299_stop(void);
void ads1299_read(RingbufHandle_t* ads1299_ringbuff);
