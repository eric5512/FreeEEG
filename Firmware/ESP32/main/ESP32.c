#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ads1299.h"
#include "wifi_app.h"

#define SPI_MOSI_PIN 18 
#define SPI_MISO_PIN 19
#define SPI_SCK_PIN  5
#define SPI_DRDY_PIN 23
#define SPI_CS_PIN   13

void app_main(void) {
  ESP_LOGI("main","App initialized");

  ads1299_init(SPI_MOSI_PIN, SPI_MISO_PIN, SPI_SCK_PIN, SPI_CS_PIN); // Need to change JP21
  wifi_ap_init();
  wifi_ap_start();
  wifi_http_start();
}
