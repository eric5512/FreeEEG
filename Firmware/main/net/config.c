#include "config.h"

#include "esp_log.h"

#define TAG "config"

void send_config(const config_t *conf) {

}

void update_config(config_t *conf, const char *name, const void *value) {

}

void log_config(const config_t *conf) {
  ESP_LOGI(TAG, "conf: electrodes: 0b%d%d%d%d%d%d%d%d, sample rate: %d, filter frequency: %d, common reference %d", conf->electrodes[0], conf->electrodes[1], conf->electrodes[2], conf->electrodes[3], conf->electrodes[4], conf->electrodes[5], conf->electrodes[6], conf->electrodes[7], conf->sample_rate, conf->filt_freq, conf->comm_ref);
}
