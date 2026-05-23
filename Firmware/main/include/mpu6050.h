#pragma once

#include <inttypes.h>

typedef struct {
  uint16_t x, y, z;
} tuple3_t;

typedef struct {
  tuple3_t acc, gyr;
} mpu_data_t;

void mpu6050_init(uint8_t pin_sdc, uint8_t pin_sda, uint8_t pin_int, void (*int_callback)(void));
void mpu6050_start(void);
void mpu6050_stop(void);
void mpu6050_read_data(mpu_data_t *data);
