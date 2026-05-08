#pragma once

#include "config.h"

void ads1299_init(void);
void ads1299_send_config(const config_t* conf);
void ads1299_start(void);
void ads1299_stop(void);
void ads1299_read(void);
