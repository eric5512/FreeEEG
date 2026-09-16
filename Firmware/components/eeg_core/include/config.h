#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FREEEEG_NUM_CHANNELS 8

typedef enum {
    CONFIG_FILTFREQ_50HZ = 0,
    CONFIG_FILTFREQ_60HZ,
} config_filtfreq_t;

typedef enum {
    CONFIG_SAMPLERATE_250,
    CONFIG_SAMPLERATE_500,
    CONFIG_SAMPLERATE_1000,
    CONFIG_SAMPLERATE_2000,
    CONFIG_SAMPLERATE_4000,
    CONFIG_SAMPLERATE_8000,
    CONFIG_SAMPLERATE_16000,
} config_samplerate_t;

typedef enum {
    CONFIG_CHANNEL_PWRDWN,
    CONFIG_CHANNEL_NORMAL,
    CONFIG_CHANNEL_SHORTED,
    CONFIG_CHANNEL_BIASMEAS,
    CONFIG_CHANNEL_MVDD,
    CONFIG_CHANNEL_TEMP,
    CONFIG_CHANNEL_TEST,
    CONFIG_CHANNEL_BIASDRP,
    CONFIG_CHANNEL_BIASDRN,
} config_channel_mode_t;

typedef enum {
    CONFIG_GAIN_1,
    CONFIG_GAIN_2,
    CONFIG_GAIN_4,
    CONFIG_GAIN_6,
    CONFIG_GAIN_8,
    CONFIG_GAIN_12,
    CONFIG_GAIN_24,
} config_gain_t;

typedef struct {
    config_filtfreq_t filt_freq;
    config_samplerate_t sample_rate;

    bool common_reference;

    config_channel_mode_t channel_mode[FREEEEG_NUM_CHANNELS];
    config_gain_t gain[FREEEEG_NUM_CHANNELS];
} config_t;

const config_t *get_config(void);
void set_config(const config_t *config);
void store_config();
bool load_config();
void default_config(config_t *config);
void log_config(const config_t *config);
