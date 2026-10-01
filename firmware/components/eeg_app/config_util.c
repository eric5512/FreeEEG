#include "config_util.h"

#include "esp_log.h"

#include <stdlib.h>

#define TAG "config_util"

ads1299_config_t config_to_ads1299(const config_t *config) {
    assert(config != NULL);

    ads1299_config_t ads_config = {0};

    switch (config->sample_rate) {
    case CONFIG_SAMPLERATE_250:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_250SPS;
        break;
    case CONFIG_SAMPLERATE_500:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_500SPS;
        break;
    case CONFIG_SAMPLERATE_1000:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_1000SPS;
        break;
    case CONFIG_SAMPLERATE_2000:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_2000SPS;
        break;
    case CONFIG_SAMPLERATE_4000:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_4000SPS;
        break;
    case CONFIG_SAMPLERATE_8000:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_8000SPS;
        break;
    case CONFIG_SAMPLERATE_16000:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_16000SPS;
        break;
    default:
        ads_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_250SPS;
        break;
    }

    ads_config.comm_ref = config->common_reference;

    for (uint8_t i = 0; i < FREEEEG_NUM_CHANNELS; i++) {
        switch (config->channel_mode[i]) {
        case CONFIG_CHANNEL_PWRDWN:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_PWRDWN;
            break;
        case CONFIG_CHANNEL_NORMAL:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_NORMAL;
            break;
        case CONFIG_CHANNEL_SHORTED:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_SHORTED;
            break;
        case CONFIG_CHANNEL_BIASMEAS:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_BIASMEAS;
            break;
        case CONFIG_CHANNEL_MVDD:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_MVDD;
            break;
        case CONFIG_CHANNEL_TEMP:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_TEMP;
            break;
        case CONFIG_CHANNEL_TEST:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_TEST;
            break;
        case CONFIG_CHANNEL_BIASDRP:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_BIASDRP;
            break;
        case CONFIG_CHANNEL_BIASDRN:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_BIASDRN;
            break;
        default:
            ads_config.electrodes[i] = ADS1299_CONFIG_MODE_SHORTED;
            break;
        }

        switch (config->gain[i]) {
        case CONFIG_GAIN_1:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_1;
            break;
        case CONFIG_GAIN_2:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_2;
            break;
        case CONFIG_GAIN_4:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_4;
            break;
        case CONFIG_GAIN_6:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_6;
            break;
        case CONFIG_GAIN_8:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_8;
            break;
        case CONFIG_GAIN_12:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_12;
            break;
        case CONFIG_GAIN_24:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_24;
            break;
        default:
            ads_config.gains[i] = ADS1299_CONFIG_GAIN_1;
            break;
        }
    }

    return ads_config;
}

void update_config(const config_t *cfg) {
  set_config(cfg);
}
