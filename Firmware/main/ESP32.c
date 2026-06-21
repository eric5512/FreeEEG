#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tasks.h"

TaskHandle_t task_afe_handle;
TaskHandle_t task_imu_handle;
TaskHandle_t task_write_handle;
TaskHandle_t task_config_handle;
TaskHandle_t task_recv_handle;

#define SDCARD_PIN_DAT0 2
#define SDCARD_PIN_DAT1 1
#define SDCARD_PIN_DAT2 3
#define SDCARD_PIN_DAT3 8
#define SDCARD_PIN_CMD  7
#define SDCARD_PIN_CLK  6
#define SDCARD_PIN_CD   9

#include "config.h"

void app_main(void) {
  ESP_LOGI("main","App initialized");

  tasks_init();
  
  xTaskCreatePinnedToCore(task_afe,"task_afe", 10000, NULL, 20, &task_afe_handle, 1);
  /* xTaskCreatePinnedToCore(task_imu,"task_imu", 10000, NULL, 0, &task_imu_handle, 1); */
  xTaskCreatePinnedToCore(task_write,"task_write", 10000, NULL, 0, &task_write_handle, 0);
  xTaskCreatePinnedToCore(task_recv,"task_recv", 10000, NULL, 0, &task_recv_handle, 0);
}
