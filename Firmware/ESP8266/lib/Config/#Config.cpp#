#include "Config.h"

#include <string.h>

uint8_t Config::electrodes[N_ELECTRODES] = {0};
uint32_t Config::sample_rate = 1000;
uint8_t Config::precission = 24;

void Config::send_config() {

}

void Config::update_config(const char *name, const void *value) {
    if(strcmp(name, "rate") == 0) {
        Config::sample_rate = *((uint32_t *) value);
    } else if (strcmp(name, "precission") == 0) {
        Config::precission = *((uint8_t *) value);
    } else if (strncmp(name, "electrode", 9) == 0) {
        uint8_t n = name[9]-'0';
        Config::electrodes[n] = *((uint8_t *) value);
    }
}
