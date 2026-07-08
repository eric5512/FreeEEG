#include "config.h"

#include "esp_log.h"

#define TAG "config"

static config_t config = {0};

config_t* get_config() {
  return &config;
}

void default_config(config_t *conf) {
  conf->filt_freq = CONFIG_FILTFREQ_50HZ;
  conf->ads1299_config.sample_rate = ADS1299_CONFIG_SAMPLERATE_250SPS;
  conf->ads1299_config.comm_ref = false;

  for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
    conf->ads1299_config.electrodes[i] = ADS1299_CONFIG_CHANNELMODE_SHORTED;
    conf->ads1299_config.gains[i] = ADS1299_CONFIG_CHANNELGAIN_1;
  }
}

void send_config(const config_t *conf) {
  ads1299_send_config(&conf->ads1299_config);
}

static const char *channel_mode_str(ads1299_config_channelmode_t mode) {
    switch (mode) {
        case ADS1299_CONFIG_CHANNELMODE_PWRDWN:   return "PWRDWN";
        case ADS1299_CONFIG_CHANNELMODE_NORMAL:   return "NORMAL";
        case ADS1299_CONFIG_CHANNELMODE_SHORTED:  return "SHORTED";
        case ADS1299_CONFIG_CHANNELMODE_BIASMEAS: return "BIASMEAS";
        case ADS1299_CONFIG_CHANNELMODE_MVDD:     return "MVDD";
        case ADS1299_CONFIG_CHANNELMODE_TEMP:     return "TEMP";
        case ADS1299_CONFIG_CHANNELMODE_TEST:     return "TEST";
        case ADS1299_CONFIG_CHANNELMODE_BIASDRP:  return "BIASDRP";
        case ADS1299_CONFIG_CHANNELMODE_BIASDRN:  return "BIASDRN";
        default:                                  return "UNKNOWN";
    }
}

static const char *gain_str(ads1299_config_channelgain_t gain) {
    switch (gain) {
        case ADS1299_CONFIG_CHANNELGAIN_1:  return "1";
        case ADS1299_CONFIG_CHANNELGAIN_2:  return "2";
        case ADS1299_CONFIG_CHANNELGAIN_4:  return "4";
        case ADS1299_CONFIG_CHANNELGAIN_6:  return "6";
        case ADS1299_CONFIG_CHANNELGAIN_8:  return "8";
        case ADS1299_CONFIG_CHANNELGAIN_12: return "12";
        case ADS1299_CONFIG_CHANNELGAIN_24: return "24";
        default:                            return "?";
    }
}

static const char *sample_rate_str(ads1299_config_samplerate_t rate) {
    switch (rate) {
        case ADS1299_CONFIG_SAMPLERATE_250SPS:   return "250 SPS";
        case ADS1299_CONFIG_SAMPLERATE_500SPS:   return "500 SPS";
        case ADS1299_CONFIG_SAMPLERATE_1000SPS:  return "1000 SPS";
        case ADS1299_CONFIG_SAMPLERATE_2000SPS:  return "2000 SPS";
        case ADS1299_CONFIG_SAMPLERATE_4000SPS:  return "4000 SPS";
        case ADS1299_CONFIG_SAMPLERATE_8000SPS:  return "8000 SPS";
        case ADS1299_CONFIG_SAMPLERATE_16000SPS: return "16000 SPS";
        default:                                 return "UNKNOWN";
    }
}

void log_config(const config_t *cfg) {
    ESP_LOGI(TAG, "===== CONFIGURATION =====");
    ESP_LOGI(TAG, "Filter: %s",
             cfg->filt_freq == CONFIG_FILTFREQ_50HZ ? "50 Hz" : "60 Hz");

    ESP_LOGI(TAG, "Sample rate: %s",
             sample_rate_str(cfg->ads1299_config.sample_rate));

    ESP_LOGI(TAG, "Common reference: %s",
             cfg->ads1299_config.comm_ref ? "enabled" : "disabled");

    for (int i = 0; i < NUM_ELECTRODES; i++) {
        ESP_LOGI(TAG,
                 "CH%d: mode=%s gain=%sx",
                 i + 1,
                 channel_mode_str(cfg->ads1299_config.electrodes[i]),
                 gain_str(cfg->ads1299_config.gains[i]));
    }

    ESP_LOGI(TAG, "=========================");
}
