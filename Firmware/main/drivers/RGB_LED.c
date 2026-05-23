#include "RGB_LED.h"

#include "driver/gpio.h"

static uint8_t RGB_PIN_R;
static uint8_t RGB_PIN_G;
static uint8_t RGB_PIN_B;

void RGB_init(uint8_t pin_r, uint8_t pin_g, uint8_t pin_b) {
  RGB_PIN_R = pin_r;
  RGB_PIN_G = pin_g;
  RGB_PIN_B = pin_b;
  
  gpio_config_t config = {0};
  config.mode = GPIO_MODE_OUTPUT;
  config.pin_bit_mask = (1ULL << pin_r) | (1ULL << pin_b) | (1ULL << pin_g);
  gpio_config(&config);
}

void RGB_set(bool r, bool g, bool b) {
  gpio_set_level(RGB_PIN_R, r);
  gpio_set_level(RGB_PIN_G, g);
  gpio_set_level(RGB_PIN_B, b);
}
