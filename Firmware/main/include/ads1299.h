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
  ADS1299_CONFIG_CHANNELGAIN_1	= 0b000,
  ADS1299_CONFIG_CHANNELGAIN_2	= 0b001,
  ADS1299_CONFIG_CHANNELGAIN_4	= 0b010,
  ADS1299_CONFIG_CHANNELGAIN_6	= 0b011,
  ADS1299_CONFIG_CHANNELGAIN_8	= 0b100,
  ADS1299_CONFIG_CHANNELGAIN_12 = 0b101,
  ADS1299_CONFIG_CHANNELGAIN_24 = 0b110,
} ads1299_config_channelgain_t;

typedef enum {
  ADS1299_CONFIG_CHANNELMODE_PWRDWN   = -1,
  ADS1299_CONFIG_CHANNELMODE_NORMAL   = 0b000,
  ADS1299_CONFIG_CHANNELMODE_SHORTED  = 0b001,
  ADS1299_CONFIG_CHANNELMODE_BIASMEAS = 0b010,
  ADS1299_CONFIG_CHANNELMODE_MVDD     = 0b011,
  ADS1299_CONFIG_CHANNELMODE_TEMP     = 0b100,
  ADS1299_CONFIG_CHANNELMODE_TEST     = 0b101,
  ADS1299_CONFIG_CHANNELMODE_BIASDRP  = 0b110,
  ADS1299_CONFIG_CHANNELMODE_BIASDRN  = 0b111,
} ads1299_config_channelmode_t;

typedef struct {
  ads1299_config_samplerate_t sample_rate;
  ads1299_config_channelmode_t electrodes[NUM_ELECTRODES];
  ads1299_config_channelgain_t gains[NUM_ELECTRODES];
  bool comm_ref;
} ads1299_config_t;

typedef struct {
  struct timeval timestamp;
  uint8_t status[3];
  uint32_t channel[8];
} ads1299_data_t;

void ads1299_init(uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_sck, uint8_t pin_cs, uint8_t pin_drdy, uint8_t pin_start, void (*drdy_callback)(void));
void ads1299_send_config(const ads1299_config_t* conf);
void ads1299_start(void);
void ads1299_stop(void);
void ads1299_read(ads1299_data_t* data);
