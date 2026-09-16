#include "ics43434.h"

#include "driver/i2s_std.h"
#include "driver/gpio.h"

static i2s_chan_handle_t rx_handle;

void ics43434_init(uint8_t ws_pin, uint8_t sck_pin, uint8_t din_pin)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),

        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_24BIT,
            I2S_SLOT_MODE_MONO
        ),

        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = sck_pin,
            .ws = ws_pin,
            .dout = I2S_GPIO_UNUSED,
            .din = din_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // LR = GND -> microphone transmits in the LEFT slot
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    // 48 kHz × 384 = 18.432 MHz MCLK
    // This gives 64 BCLK cycles per stereo frame.
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));

    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

void ics43434_start() {
  i2s_channel_enable(rx_handle);
}

void ics43434_stop() {
  i2s_channel_disable(rx_handle);  
}

size_t ics43434_read(uint32_t* data_buff, size_t max_len) {
  size_t bytes_read;
  i2s_channel_read(rx_handle, data_buff, max_len, &bytes_read, portMAX_DELAY);
  return bytes_read;
}
