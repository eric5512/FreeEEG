#pragma once

#include <inttypes.h>
#include <sys/time.h>

typedef struct {
  uint16_t x, y, z;
} tuple3_t;

typedef struct {
  struct timeval timestamp;
  tuple3_t accel, gyro;
} icm42670_data_t;

void icm42670_init(uint8_t pin_sdc, uint8_t pin_sda, uint8_t pin_int, void (*int_callback)(void));
void icm42670_start(void);
void icm42670_stop(void);
void icm42670_read_data(icm42670_data_t *data);
