#pragma once

#include <inttypes.h>
#include <sys/time.h>

#define NUM_ELECTRODES 8

typedef enum {
  ADS1299_CONFIG_SAMPLERATE_250SPS   = 0b110,
  ADS1299_CONFIG_SAMPLERATE_500SPS   = 0b101,
  ADS1299_CONFIG_SAMPLERATE_1000SPS  = 0b100,
  ADS1299_CONFIG_SAMPLERATE_2000SPS  = 0b011,
  ADS1299_CONFIG_SAMPLERATE_4000SPS  = 0b010,
  ADS1299_CONFIG_SAMPLERATE_8000SPS  = 0b001,
  ADS1299_CONFIG_SAMPLERATE_16000SPS = 0b000,
} ads1299_config_samplerate_t;

typedef enum {
  ADS1299_CONFIG_GAIN_1	= 0b000,
  ADS1299_CONFIG_GAIN_2	= 0b001,
  ADS1299_CONFIG_GAIN_4	= 0b010,
  ADS1299_CONFIG_GAIN_6	= 0b011,
  ADS1299_CONFIG_GAIN_8	= 0b100,
  ADS1299_CONFIG_GAIN_12 = 0b101,
  ADS1299_CONFIG_GAIN_24 = 0b110,
} ads1299_config_gain_t;

typedef enum {
  ADS1299_CONFIG_MODE_PWRDWN   = -1,
  ADS1299_CONFIG_MODE_NORMAL   = 0b000,
  ADS1299_CONFIG_MODE_SHORTED  = 0b001,
  ADS1299_CONFIG_MODE_BIASMEAS = 0b010,
  ADS1299_CONFIG_MODE_MVDD     = 0b011,
  ADS1299_CONFIG_MODE_TEMP     = 0b100,
  ADS1299_CONFIG_MODE_TEST     = 0b101,
  ADS1299_CONFIG_MODE_BIASDRP  = 0b110,
  ADS1299_CONFIG_MODE_BIASDRN  = 0b111,
} ads1299_config_channel_mode_t;

typedef struct {
  ads1299_config_samplerate_t sample_rate;
  ads1299_config_channel_mode_t electrodes[NUM_ELECTRODES];
  ads1299_config_gain_t gains[NUM_ELECTRODES];
  bool comm_ref;
} ads1299_config_t;

typedef struct {
  struct timeval timestamp;
  uint8_t status[3];
  int32_t channel[8];
} ads1299_data_t;

void ads1299_init(uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_sck, uint8_t pin_cs, uint8_t pin_drdy, uint8_t pin_start, void (*drdy_callback)(void));
void ads1299_send_config(const ads1299_config_t* conf);
void ads1299_start(void);
void ads1299_stop(void);
void ads1299_read(ads1299_data_t* data);
