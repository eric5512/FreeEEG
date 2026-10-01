#include "config.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#define TAG "config"

#define STORAGE_NAMESPACE "config"

static config_t config = {0};

const config_t* get_config() {
  return &config;
}

void config_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_LOGW(TAG, "NVS partition invalid, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());

    } else {
        ESP_ERROR_CHECK(err);
    }

    if (!load_config()) {
        ESP_LOGI(TAG, "No stored configuration found, using defaults");

        default_config(&config);
        store_config();
    } else {
        ESP_LOGI(TAG, "Configuration loaded from NVS");
    }

    log_config(&config);
}

void default_config(config_t *conf) {
  conf->filt_freq = CONFIG_FILTFREQ_50HZ;
  conf->sample_rate = CONFIG_SAMPLERATE_250;
  conf->common_reference = false;

  for (uint8_t i = 0; i < FREEEEG_NUM_CHANNELS; i++) {
    conf->channel_mode[i] = CONFIG_CHANNEL_SHORTED;
    conf->gain[i] = CONFIG_GAIN_1;
  }
}

static const char *channel_mode_str(config_channel_mode_t mode) {
  switch (mode) {
  case CONFIG_CHANNEL_PWRDWN:   return "PWRDWN";
  case CONFIG_CHANNEL_NORMAL:   return "NORMAL";
  case CONFIG_CHANNEL_SHORTED:  return "SHORTED";
  case CONFIG_CHANNEL_BIASMEAS: return "BIASMEAS";
  case CONFIG_CHANNEL_MVDD:     return "MVDD";
  case CONFIG_CHANNEL_TEMP:     return "TEMP";
  case CONFIG_CHANNEL_TEST:     return "TEST";
  case CONFIG_CHANNEL_BIASDRP:  return "BIASDRP";
  case CONFIG_CHANNEL_BIASDRN:  return "BIASDRN";
  default:                                  return "UNKNOWN";
  }
}

static const char *gain_str(config_gain_t gain) {
  switch (gain) {
  case CONFIG_GAIN_1:  return "1";
  case CONFIG_GAIN_2:  return "2";
  case CONFIG_GAIN_4:  return "4";
  case CONFIG_GAIN_6:  return "6";
  case CONFIG_GAIN_8:  return "8";
  case CONFIG_GAIN_12: return "12";
  case CONFIG_GAIN_24: return "24";
  default:                            return "?";
  }
}

static const char *sample_rate_str(config_samplerate_t rate) {
  switch (rate) {
  case CONFIG_SAMPLERATE_250:   return "250 SPS";
  case CONFIG_SAMPLERATE_500:   return "500 SPS";
  case CONFIG_SAMPLERATE_1000:  return "1000 SPS";
  case CONFIG_SAMPLERATE_2000:  return "2000 SPS";
  case CONFIG_SAMPLERATE_4000:  return "4000 SPS";
  case CONFIG_SAMPLERATE_8000:  return "8000 SPS";
  case CONFIG_SAMPLERATE_16000: return "16000 SPS";
  default:                                 return "UNKNOWN";
  }
}

void log_config(const config_t *cfg) {
  ESP_LOGI(TAG, "===== CONFIGURATION =====");
  ESP_LOGI(TAG, "Filter: %s",
	   cfg->filt_freq == CONFIG_FILTFREQ_50HZ ? "50 Hz" : "60 Hz");

  ESP_LOGI(TAG, "Sample rate: %s",
	   sample_rate_str(cfg->sample_rate));

  ESP_LOGI(TAG, "Common reference: %s",
	   cfg->common_reference ? "enabled" : "disabled");

  for (int i = 0; i < FREEEEG_NUM_CHANNELS; i++) {
    ESP_LOGI(TAG,
	     "CH%d: mode=%s gain=%sx",
	     i + 1,
	     channel_mode_str(cfg->channel_mode[i]),
	     gain_str(cfg->gain[i]));
  }

  ESP_LOGI(TAG, "=========================");
}

void set_config(const config_t *cfg) {
    assert(cfg != NULL);
    config = *cfg;
    log_config(&config);
}

void store_config(void) {
    nvs_handle_t my_handle;

    esp_err_t err = nvs_open(
        STORAGE_NAMESPACE,
        NVS_READWRITE,
        &my_handle
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle",
                 esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Saving configuration");

    err = nvs_set_blob(
        my_handle,
        "config_data",
        &config,
        sizeof(config)
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write configuration: %s",
                 esp_err_to_name(err));
        nvs_close(my_handle);
        return;
    }

    err = nvs_commit(my_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit configuration: %s",
                 esp_err_to_name(err));
    }

    nvs_close(my_handle);
}

bool load_config(void) {
    nvs_handle_t my_handle;

    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        return false;
    }

    size_t config_size = sizeof(config_t);

    err = nvs_get_blob(
        my_handle,
        "config_data",
        &config,
        &config_size
    );

    nvs_close(my_handle);

    if (err != ESP_OK) {
        return false;
    }

    if (config_size != sizeof(config_t)) {
        ESP_LOGW(TAG, "Stored config has incorrect size");
        return false;
    }

    return true;
}
