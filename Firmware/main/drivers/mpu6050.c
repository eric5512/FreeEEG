#include "mpu6050.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_check.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "mpu6050"

#define MPU6050_I2C_PORT      I2C_NUM_0
#define MPU6050_I2C_FREQ_HZ   400000
#define MPU6050_ADDR          0x68

#define MPU6050_REG_SMPLRT    0x19
#define MPU6050_REG_CONFIG    0x1A
#define MPU6050_REG_GYRO_CFG  0x1B
#define MPU6050_REG_ACCEL_CFG 0x1C
#define MPU6050_REG_INT_EN    0x38
#define MPU6050_REG_INT_CFG   0x37
#define MPU6050_REG_ACCEL_X_H 0x3B
#define MPU6050_REG_PWR_MGMT1 0x6B
#define MPU6050_REG_WHO_AM_I  0x75

static bool mpu_running = false;

static gpio_num_t mpu_int_pin = GPIO_NUM_NC;

static void (*mpu_int_callback)(void) = NULL;

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t mpu_dev_handle = NULL;

static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx[2] = { reg, value };

    return i2c_master_transmit(
        mpu_dev_handle,
        tx,
        sizeof(tx),
        pdMS_TO_TICKS(100)
    );
}

static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *rx, size_t len) {
    return i2c_master_transmit_receive(
        mpu_dev_handle,
        &reg,
        1,
        rx,
        len,
        pdMS_TO_TICKS(100)
    );
}

static uint8_t mpu6050_read_reg(uint8_t reg) {
    uint8_t rx = 0xFF;

    esp_err_t ret = mpu6050_read_regs(reg, &rx, 1);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register read failed: 0x%02X", reg);
    }

    return rx;
}

static void IRAM_ATTR mpu6050_isr_handler(void *arg) {
  mpu_int_callback();
}

void mpu6050_init(uint8_t pin_scl, uint8_t pin_sda, uint8_t pin_int, void (*int_callback)(void)) {
    mpu_int_pin = (gpio_num_t)pin_int;
    mpu_int_callback = int_callback;

    if (i2c_bus_handle == NULL) {
        i2c_master_bus_config_t bus_config = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = MPU6050_I2C_PORT,
            .scl_io_num = (gpio_num_t)pin_scl,
            .sda_io_num = (gpio_num_t)pin_sda,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };

        ESP_ERROR_CHECK(
            i2c_new_master_bus(&bus_config, &i2c_bus_handle)
        );
    }

    if (mpu_dev_handle == NULL) {
        i2c_device_config_t dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = MPU6050_ADDR,
            .scl_speed_hz = MPU6050_I2C_FREQ_HZ,
        };

        ESP_ERROR_CHECK(
            i2c_master_bus_add_device(
                i2c_bus_handle,
                &dev_config,
                &mpu_dev_handle
            )
        );
    }

    uint8_t whoami = mpu6050_read_reg(MPU6050_REG_WHO_AM_I);

    if (whoami == 0x68) {
        ESP_LOGI(TAG, "MPU-6050 device found");
    } else {
        ESP_LOGE(TAG, "MPU-6050 device not found, WHO_AM_I = 0x%02X", whoami);
    }

    mpu_running = false;

    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_PWR_MGMT1, 0x00));
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_SMPLRT, 9));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_CONFIG, 0x03));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_GYRO_CFG, 0x00));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_ACCEL_CFG, 0x00));

    if (mpu_int_callback != NULL) {
        gpio_config_t int_conf = {
            .pin_bit_mask = 1ULL << mpu_int_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_POSEDGE,
        };

        ESP_ERROR_CHECK(gpio_config(&int_conf));

        ESP_ERROR_CHECK(
            gpio_isr_handler_add(
                mpu_int_pin,
                mpu6050_isr_handler,
                NULL
            )
        );

        ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_CFG, 0x10));
        ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_EN, 0x01));
    } else {
        ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_EN, 0x00));
    }
}

void mpu6050_start(void) {
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_PWR_MGMT1, 0x00));

    if (mpu_int_callback != NULL) {
        ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_EN, 0x01));
    } else {
        ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_EN, 0x00));
    }

    mpu_running = true;
}

void mpu6050_stop(void) {
    mpu_running = false;

    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_INT_EN, 0x00));
    ESP_ERROR_CHECK(mpu6050_write_reg(MPU6050_REG_PWR_MGMT1, 0x40));
}

void mpu6050_read_data(mpu_data_t *data) {
    if (data == NULL || !mpu_running) {
        return;
    }

    uint8_t raw[14];

    esp_err_t ret = mpu6050_read_regs(
        MPU6050_REG_ACCEL_X_H,
        raw,
        sizeof(raw)
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 read failed");
        return;
    }

    data->acc.x = (int16_t)((raw[0] << 8) | raw[1]);
    data->acc.y = (int16_t)((raw[2] << 8) | raw[3]);
    data->acc.z = (int16_t)((raw[4] << 8) | raw[5]);

    data->gyr.x = (int16_t)((raw[8]  << 8) | raw[9]);
    data->gyr.y = (int16_t)((raw[10] << 8) | raw[11]);
    data->gyr.z = (int16_t)((raw[12] << 8) | raw[13]);
}
