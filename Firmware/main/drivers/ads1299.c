#include "ads1299.h"

#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ADS1299";

#define ADS1299_SPI_HOST SPI2_HOST
#define ADS1299_SPI_FREQ_HZ 1000000

#define ADS1299_CMD_WAKEUP  0x02
#define ADS1299_CMD_STANDBY 0x04
#define ADS1299_CMD_RESET   0x06
#define ADS1299_CMD_START   0x08
#define ADS1299_CMD_STOP    0x0A
#define ADS1299_CMD_RDATAC  0x10
#define ADS1299_CMD_SDATAC  0x11
#define ADS1299_CMD_RDATA   0x12
#define ADS1299_CMD_RREG    0x20
#define ADS1299_CMD_WREG    0x40

#define ADS1299_REG_ID          0x00
#define	ADS1299_REG_CONFIG1	0x01
#define ADS1299_REG_CONFIG2	0x02
#define ADS1299_REG_CONFIG3	0x03
#define ADS1299_REG_CH1SET	0x05
#define ADS1299_REG_BIAS_SENSP	0x0D
#define ADS1299_REG_BIAS_SENSN	0x0E
#define ADS1299_REG_MISC1	0x15

#define ADS1299_FRAME_SIZE 27

static gpio_num_t ads1299_drdy_pin = GPIO_NUM_NC;
static void (*ads1299_drdy_callback)(void) = NULL;

static spi_device_handle_t ads_spi = NULL;
static volatile bool ads_running = false;
static uint16_t ads_sample_rate = 250;

static void ads1299_spi_txrx(const uint8_t *tx, uint8_t *rx, size_t len) {
  spi_transaction_t t = {
    .length = len * 8,
    .tx_buffer = tx,
    .rx_buffer = rx
  };

  ESP_ERROR_CHECK(spi_device_transmit(ads_spi, &t));
}

static void ads1299_cmd(uint8_t cmd) {
  ads1299_spi_txrx(&cmd, NULL, 1);
  vTaskDelay(pdMS_TO_TICKS(4));
}

static void ads1299_write_reg(uint8_t reg, uint8_t value) {
  uint8_t tx[3] = {
    ADS1299_CMD_WREG | reg,
    0x00,
    value
  };

  ads1299_spi_txrx(tx, NULL, sizeof(tx));
  vTaskDelay(pdMS_TO_TICKS(4));
}

static uint8_t ads1299_read_register(uint8_t reg) {
  uint8_t tx[3] = {
    ADS1299_CMD_RREG | reg,
    0x00,
    0x00
  };

  uint8_t rx[3] = {0};

  ads1299_spi_txrx(tx, rx, sizeof(tx));

  vTaskDelay(pdMS_TO_TICKS(4));

  return rx[2];
}

static uint8_t ads1299_sample_rate_bits(uint16_t sample_rate) {
  switch (sample_rate) {
  case 16000: return 0x00;
  case 8000:  return 0x01;
  case 4000:  return 0x02;
  case 2000:  return 0x03;
  case 1000:  return 0x04;
  case 500:   return 0x05;
  case 250:   return 0x06;
  default:    return 0x06;   // Default: 250 SPS
  }
}

static void IRAM_ATTR ads1299_drdy_isr_handler(void *arg) {
  ads1299_drdy_callback();
}

void ads1299_init(uint8_t pin_mosi,
    uint8_t pin_miso,
    uint8_t pin_sck,
    uint8_t pin_cs,
    uint8_t pin_drdy,
    void (*drdy_callback)(void)) {
    ads1299_drdy_pin = (gpio_num_t)pin_drdy;
    ads1299_drdy_callback = drdy_callback;

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = ADS1299_FRAME_SIZE
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = ADS1299_SPI_FREQ_HZ,
        .mode = 1,
        .spics_io_num = pin_cs,
        .queue_size = 1
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            ADS1299_SPI_HOST,
            &buscfg,
            SPI_DMA_CH_AUTO
        )
    );

    ESP_ERROR_CHECK(
        spi_bus_add_device(
            ADS1299_SPI_HOST,
            &devcfg,
            &ads_spi
        )
    );

    if (ads1299_drdy_callback != NULL) {
        gpio_config_t drdy_conf = {
            .pin_bit_mask = 1ULL << ads1299_drdy_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,

            /*
             * ADS1299 DRDY is active low.
             * Interrupt on falling edge.
             */
            .intr_type = GPIO_INTR_NEGEDGE,
        };

        ESP_ERROR_CHECK(gpio_config(&drdy_conf));

        ESP_ERROR_CHECK(
            gpio_isr_handler_add(
                ads1299_drdy_pin,
                ads1299_drdy_isr_handler,
                NULL
            )
        );
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    ads1299_cmd(ADS1299_CMD_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));

    ads1299_cmd(ADS1299_CMD_SDATAC);
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t id_reg = ads1299_read_register(ADS1299_REG_ID);

    if ((id_reg & 0b00011100) == 0b00011100) {
        char n_channels =
            ((id_reg & 0b11) == 0b10) ? '8' :
            ((id_reg & 0b11) == 0b01) ? '6' :
                                        '4' ;

        ESP_LOGI(TAG, "Found ADS1299 with %c channels", n_channels);
    } else {
      ESP_LOGE(TAG,
            "ADS1299 not found, ID register = 0x%02X",
            id_reg
        );
    }
}

void ads1299_send_config(const config_t* conf) {
  if (conf == NULL) {
    return;
  }

  ads_sample_rate = conf->sample_rate;

  ads1299_cmd(ADS1299_CMD_SDATAC);

  uint8_t dr = ads1299_sample_rate_bits(conf->sample_rate);

  // CONFIG1:
  // 0x90 keeps high-resolution mode and internal clock-related defaults.
  // Lower 3 bits select data rate.
  ads1299_write_reg(ADS1299_REG_CONFIG1, 0xD8 | dr);

  // CONFIG2:
  // Internal test signal disabled, normal operation.
  ads1299_write_reg(ADS1299_REG_CONFIG2, 0xC0);

  // CONFIG3:
  // Enable internal reference buffer and bias amplifier.
  ads1299_write_reg(ADS1299_REG_CONFIG3, 0xEC);

  // CONFIG4:
  // 

  for (int i = 0; i < NUM_ELECTRODES; i++) {
    uint8_t ch_reg = ADS1299_REG_CH1SET + i;

    if (conf->electrodes[i]) {
      // Enabled channel:
      // gain = 24, normal electrode input.
      ads1299_write_reg(ch_reg, 0x60);
    } else {
      // Disabled channel:
      // power down channel, input shorted.
      ads1299_write_reg(ch_reg, 0x81);
    }
  }

  if (conf->comm_ref) {
    uint8_t bias_mask = 0x00;

    for (int i = 0; i < NUM_ELECTRODES; i++) {
      if (conf->electrodes[i]) {
	bias_mask |= (1 << i);
      }
    }

    // Include active channels in bias/common-reference generation.
    ads1299_write_reg(ADS1299_REG_BIAS_SENSP, bias_mask);
    ads1299_write_reg(ADS1299_REG_BIAS_SENSN, bias_mask);

    // Enable SRB1 common reference routing.
    ads1299_write_reg(ADS1299_REG_MISC1, 0x20);
  } else {
    ads1299_write_reg(ADS1299_REG_BIAS_SENSP, 0x00);
    ads1299_write_reg(ADS1299_REG_BIAS_SENSN, 0x00);
    ads1299_write_reg(ADS1299_REG_MISC1, 0x00);
  }
}

void ads1299_start(void) {
  ads1299_cmd(ADS1299_CMD_SDATAC);
  ads1299_cmd(ADS1299_CMD_START);
  ads1299_cmd(ADS1299_CMD_RDATAC);

  ads_running = true;
}

void ads1299_stop(void) {
  ads_running = false;

  ads1299_cmd(ADS1299_CMD_SDATAC);
  ads1299_cmd(ADS1299_CMD_STOP);
}

void ads1299_read(ads1299_data_t *data) {
  if (data == NULL) {
    return;
  }

  uint8_t tx[ADS1299_FRAME_SIZE] = {0};
  uint8_t raw[ADS1299_FRAME_SIZE] = {0};
    
  ads1299_spi_txrx(tx, raw, ADS1299_FRAME_SIZE);

  /*
   * Status bytes
   */
  data->status[0] = raw[0];
  data->status[1] = raw[1];
  data->status[2] = raw[2];

  /*
   * ADS1299 channels:
   * signed 24-bit big-endian
   */
  for (uint8_t i = 0; i < 8; i++) {
    uint32_t idx = 3 + (i * 3);

    int32_t value =
      ((int32_t)raw[idx] << 16) |
      ((int32_t)raw[idx + 1] << 8)  |
      ((int32_t)raw[idx + 2]);

    /*
     * Sign extension:
     * 24-bit -> 32-bit
     */
    if (value & 0x00800000) {
      value |= 0xFF000000;
    }

    data->channel[i] = value;
  }
}
