#pragma once

#include <inttypes.h>

class Config {
    private:
    static const uint8_t N_ELECTRODES = 8;
    static uint8_t electrodes[N_ELECTRODES];
    static uint32_t sample_rate;
    static uint8_t precission;

    public:
    static void send_config();
    static void update_config(const char *name, const void *value);
};
