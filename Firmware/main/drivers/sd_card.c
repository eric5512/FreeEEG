#include <stdio.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define TAG "sd_card"
#define MOUNT_POINT "/sdcard"

static sdmmc_card_t *card = NULL;

static uint8_t sdcard_pin_cd;

void sdcard_mount(uint8_t sd_pin_clk, uint8_t sd_pin_cmd, uint8_t sd_pin_d0, uint8_t sd_pin_d1, uint8_t sd_pin_d2, uint8_t sd_pin_d3, uint8_t sd_pin_cd) {
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  gpio_config_t carddetect_conf = {
    .pin_bit_mask = 1ULL << sd_pin_cd,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };
  
  ESP_ERROR_CHECK(gpio_config(&carddetect_conf));

  sdcard_pin_cd = sd_pin_cd;
    
  if (gpio_get_level(sdcard_pin_cd) == 1) {
    ESP_LOGI(TAG, "SD card not present");
  }
    
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  slot_config.width = 4;
  slot_config.clk = sd_pin_clk;
  slot_config.cmd = sd_pin_cmd;
  slot_config.d0 = sd_pin_d0;
  slot_config.d1 = sd_pin_d1;
  slot_config.d2 = sd_pin_d2;
  slot_config.d3 = sd_pin_d3;

  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_err_t ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    return;
  }

  sdmmc_card_print_info(stdout, card);
}

FILE* sdcard_new_file(void) {
  return NULL;
}

void sdcard_unmount(void) {
  esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
}
