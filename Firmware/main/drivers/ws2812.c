#include "ws2812.h"

#include "driver/rmt_tx.h"

#define BYTES_PER_COLOR 1

static const char *TAG = "ws2812";

static rmt_channel_handle_t tx_chan  = NULL;
static rmt_encoder_handle_t enc_hand = NULL;

const ws2812_color_t WS2812_RED = {
  .r = 0xFF,
  .g = 0x00,
  .b = 0x00,
};

const ws2812_color_t WS2812_GREEN = {
  .r = 0x00,
  .g = 0xFF,
  .b = 0x00,
};

const ws2812_color_t WS2812_BLUE = {
  .r = 0x00,
  .g = 0x00,
  .b = 0xFF,
};

const ws2812_color_t WS2812_WHITE = {
  .r = 0xFF,
  .g = 0xFF,
  .b = 0xFF,
};

const ws2812_color_t WS2812_BLACK = {
  .r = 0x00,
  .g = 0x00,
  .b = 0x00,
};


void ws2812_init(uint8_t ws2812_pin) {
  rmt_tx_channel_config_t tx_chan_config = {
    .clk_src	       = RMT_CLK_SRC_DEFAULT,	// select source clock
    .gpio_num	       = ws2812_pin,		// GPIO number
    .mem_block_symbols = 64,	                // memory block size
    .resolution_hz     = 10000000,		// 10 MHz tick resolution, i.e., 1 tick = 100 ns
    .trans_queue_depth = 1,			// set the number of transactions that can pend in the background
    .flags.invert_out  = false,			// do not invert output signal
    .flags.with_dma    = false,			// do not need DMA backend
  };
  
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));

  rmt_bytes_encoder_config_t enc_conf = {
    .bit0 = {
      .duration0 = 3,
      .level0 = 1,
      .duration1 = 10,
      .level1 = 0,
    },
    .bit1 = {
      .duration0 = 10,
      .level0 = 1,
      .duration1 = 3,
      .level1 = 0,
    },
    .flags = {
      .msb_first = true,
    },
  };
  
  ESP_ERROR_CHECK(rmt_new_bytes_encoder(&enc_conf, &enc_hand));
}


esp_err_t ws2812_set(const ws2812_color_t *color) {
  uint8_t buff[BYTES_PER_COLOR*3] = {0};

  buff[0]		  = color->g;
  buff[BYTES_PER_COLOR]	  = color->r;	
  buff[BYTES_PER_COLOR*2] = color->b;

  rmt_transmit_config_t tx_conf = {0};
  
  ESP_RETURN_ON_ERROR(rmt_enable(tx_chan), TAG, "enable RMT channel failed");
  ESP_RETURN_ON_ERROR(rmt_transmit(tx_chan, enc_hand, buff, sizeof(buff), &tx_conf), TAG, "transmit pixels by RMT failed");
  ESP_RETURN_ON_ERROR(rmt_tx_wait_all_done(tx_chan, -1), TAG, "flush RMT channel failed");
  ESP_RETURN_ON_ERROR(rmt_disable(tx_chan), TAG, "disable RMT channel failed");
  return ESP_OK;
}
