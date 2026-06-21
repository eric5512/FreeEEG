#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "ads1299.h"

typedef enum {
  CONFIG_FILTFREQ_50HZ = 0,
  CONFIG_FILTFREQ_60HZ,
} config_filtfreq_t;

typedef struct {
  config_filtfreq_t filt_freq;
  ads1299_config_t ads1299_config;
} config_t;

config_t* get_config();
void default_config(config_t *conf);
void send_config(const config_t *conf);
void log_config(const config_t *cfg);
