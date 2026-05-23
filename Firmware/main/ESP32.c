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

void app_main(void) {
  ESP_LOGI("main","App initialized");

  tasks_init();
  
  xTaskCreatePinnedToCore(task_afe,"task_afe", 10000, NULL, 0, &task_afe_handle, 1);
  /* xTaskCreatePinnedToCore(task_imu,"task_imu", 10000, NULL, 0, &task_imu_handle, 1); */
  xTaskCreatePinnedToCore(task_write,"task_write", 10000, NULL, 0, &task_write_handle, 0);
  /* xTaskCreatePinnedToCore(task_config,"task_config", 10000, NULL, 0, &task_config_handle, 0); */
  xTaskCreatePinnedToCore(task_recv,"task_recv", 10000, NULL, 0, &task_recv_handle, 0);
}
