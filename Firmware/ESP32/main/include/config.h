#pragma once

#include <inttypes.h>
#include <stdbool.h>

const uint8_t NUM_ELECTRODES = 8;

typedef struct {
  uint16_t sample_rate;
  uint8_t precission;
  uint8_t filt_freq;
  
  bool electrodes[N_ELECTRODES];
  bool comm_ref;
} config_t;


void send_config(const config_t *conf);
void update_config(config_t *conf, const char *name, const void *value);
