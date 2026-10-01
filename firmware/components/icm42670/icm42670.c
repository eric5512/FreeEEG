#include "icm42670.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_check.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "icm42670"

#define ICM42670_I2C_PORT      I2C_NUM_0
#define ICM42670_I2C_FREQ_HZ   500000
#define ICM42670_ADDR          0x68

#define ICM42670_REG_WHO_AM_I  0x75

#define ICM42670_REG_INT_CONFIG       0x06
#define ICM42670_REG_TEMP_DATA1       0x09
#define ICM42670_REG_ACCEL_X1         0x0B
#define ICM42670_REG_ACCEL_Y1         0x0D
#define ICM42670_REG_ACCEL_Z1         0x0F
#define ICM42670_REG_GYRO_X1          0x11
#define ICM42670_REG_GYRO_Y1          0x13
#define ICM42670_REG_GYRO_Z1          0x15

#define ICM42670_REG_PWR_MGMT0        0x1F
#define ICM42670_REG_GYRO_CONFIG0     0x20
#define ICM42670_REG_ACCEL_CONFIG0    0x21

#define ICM42670_REG_INT_SOURCE0      0x2B

#define ICM42670_INT1_DRDY            (1 << 3)

static gpio_num_t icm_int_pin = GPIO_NUM_NC;

static void (*icm_int_callback)(void) = NULL;

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t icm_dev_handle = NULL;

static esp_err_t icm42670_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { reg, value };

    return i2c_master_transmit(
        icm_dev_handle,
        tx,
        sizeof(tx),
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t icm42670_read_regs(uint8_t reg, uint8_t *rx, size_t len) {
    return i2c_master_transmit_receive(
        icm_dev_handle,
        &reg,
        1,
        rx,
        len,
        pdMS_TO_TICKS(100)
    );
}

static uint8_t icm42670_read_reg(uint8_t reg) {
    uint8_t rx = 0xFF;

    esp_err_t ret = icm42670_read_regs(reg, &rx, 1);

    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Register read failed: 0x%02X: %s",
	       reg,
	       esp_err_to_name(ret));
    }

    return rx;
}

static void IRAM_ATTR icm42670_isr_handler(void *arg) {
  icm_int_callback();
}

void icm42670_init(uint8_t pin_scl, uint8_t pin_sda, uint8_t pin_int, void (*int_callback)(void)) {
    icm_int_pin = (gpio_num_t)pin_int;
    icm_int_callback = int_callback;

    i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = ICM42670_I2C_PORT,
      .scl_io_num = (gpio_num_t)pin_scl,
      .sda_io_num = (gpio_num_t)pin_sda,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
		    i2c_new_master_bus(&bus_config, &i2c_bus_handle)
		    );

    i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ICM42670_ADDR,
      .scl_speed_hz = ICM42670_I2C_FREQ_HZ,
    };

    ESP_ERROR_CHECK(
		    i2c_master_bus_add_device(
					      i2c_bus_handle,
					      &dev_config,
					      &icm_dev_handle
					      )
		    );
    

    uint8_t whoami = icm42670_read_reg(ICM42670_REG_WHO_AM_I);

    if (whoami == 0x67) {
      ESP_LOGI(TAG, "ICM-42670 device found");
    } else {
      ESP_LOGE(TAG, "ICM-42670 device not found, WHO_AM_I = 0x%02X", whoami);
    }

    /*
     * Gyroscope:
     *
     * FS = ±250 dps
     * ODR = 100 Hz
     *
     * GYRO_CONFIG0:
     *
     * bits 6:5 = 11 -> ±250 dps
     * bits 3:0 = 1001 -> 100 Hz
     *
     * 0b01101001 = 0x69
     */
    ESP_ERROR_CHECK(icm42670_write_reg(
				       ICM42670_REG_GYRO_CONFIG0,
				       0x69
				       ));

    /*
     * Accelerometer:
     *
     * FS = ±2 g
     * ODR = 100 Hz
     *
     * ACCEL_CONFIG0:
     *
     * bits 6:5 = 11 -> ±2 g
     * bits 3:0 = 1001 -> 100 Hz
     *
     * 0b01101001 = 0x69
     */
    ESP_ERROR_CHECK(icm42670_write_reg(
				       ICM42670_REG_ACCEL_CONFIG0,
				       0x69
				       ));

    /*
     * Configure INT1:
     *
     * INT1_MODE          = 0 -> pulsed
     * INT1_DRIVE_CIRCUIT = 1 -> push-pull
     * INT1_POLARITY      = 1 -> active high
     *
     * INT2 remains at its reset configuration.
     *
     * INT_CONFIG = 0b00000011 = 0x03
     */
    ESP_ERROR_CHECK(icm42670_write_reg(
				       ICM42670_REG_INT_CONFIG,
				       0x03
				       ));
    
    if (icm_int_callback != NULL) {
      gpio_config_t int_conf = {
        .pin_bit_mask = 1ULL << icm_int_pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
      };

      ESP_ERROR_CHECK(gpio_config(&int_conf));
      ESP_ERROR_CHECK(gpio_isr_handler_add(icm_int_pin, icm42670_isr_handler, NULL));

    }
}


void icm42670_start(void) {
    ESP_ERROR_CHECK(
        icm42670_write_reg(ICM42670_REG_PWR_MGMT0, 0x0F)
    );

    /*
     * Datasheet requires at least 200 us before another register write.
     * Wait longer here so the gyro has time to start as well.
     */
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "ICM42670 started");
}

void icm42670_stop(void) {
    /*
     * Disable all INT1 sources.
     */
    ESP_ERROR_CHECK(icm42670_write_reg(
				       ICM42670_REG_INT_SOURCE0,
				       0x00
				       ));

    /*
     * Turn off gyro and accelerometer.
     *
     * GYRO_MODE   = 00 -> OFF
     * ACCEL_MODE  = 00 -> OFF
     */
    ESP_ERROR_CHECK(icm42670_write_reg(
				       ICM42670_REG_PWR_MGMT0,
				       0x00
				       ));

    vTaskDelay(pdMS_TO_TICKS(1));

    ESP_LOGI(TAG, "ICM42670 stopped");
}

void icm42670_read_data(icm42670_data_t *data) {
    if (data == NULL) {
        return;
    }

    uint8_t raw[12];

    esp_err_t ret = icm42670_read_regs(
        ICM42670_REG_ACCEL_X1,
        raw,
        sizeof(raw)
    );

    if (ret != ESP_OK) {
      memset(data, 0, sizeof(icm42670_data_t));
      ESP_LOGE(TAG, "ICM42670 data read failed: %s",
                 esp_err_to_name(ret));
      return;
    }

    /*
     * Accelerometer
     */

    data->accel.x = (int16_t)(
        ((uint16_t)raw[0] << 8) |
        raw[1]
    );

    data->accel.y = (int16_t)(
        ((uint16_t)raw[2] << 8) |
        raw[3]
    );

    data->accel.z = (int16_t)(
        ((uint16_t)raw[4] << 8) |
        raw[5]
    );


    /*
     * Gyroscope
     */

    data->gyro.x = (int16_t)(
        ((uint16_t)raw[6] << 8) |
        raw[7]
    );

    data->gyro.y = (int16_t)(
        ((uint16_t)raw[8] << 8) |
        raw[9]
    );

    data->gyro.z = (int16_t)(
        ((uint16_t)raw[10] << 8) |
        raw[11]
    );
}
