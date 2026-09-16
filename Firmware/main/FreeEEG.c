#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tasks.h"

#define TAG "mainApp"

TaskHandle_t task_afe_handle;
TaskHandle_t task_imu_handle;
TaskHandle_t task_write_handle;
TaskHandle_t task_config_handle;
TaskHandle_t task_recv_handle;
TaskHandle_t task_sound_handle;

/* #define SDCARD_PIN_DATA0	2 */
/* #define SDCARD_PIN_DATA1	1 */
/* #define SDCARD_PIN_DATA2	3 */
/* #define SDCARD_PIN_DATA3	8 */
/* #define SDCARD_PIN_CMD		7 */
/* #define SDCARD_PIN_CLK		6 */
/* #define SDCARD_PIN_CD		9 */

/* #include "config.h" */

/* #include "sd_card.h" */

/* #define MOUNT_POINT "/sdcard" */

/* void sdcard_read_test(void) { */
/*   FILE *f = fopen(MOUNT_POINT "/test.txt", "r"); */

/*   if (f == NULL) { */
/*     ESP_LOGE(TAG, "Failed to open file for reading"); */
/*     return; */
/*   } */

/*   char line[128]; */

/*   while (fgets(line, sizeof(line), f) != NULL) { */
/*     ESP_LOGI(TAG, "File contents: \"%s\"", line); */
/*   } */

/*   fclose(f); */
/* } */

/* void sdcard_write_test(void) { */
/*     FILE *f = fopen(MOUNT_POINT "/test.txt", "w+"); */

/*     if (f == NULL) { */
/*         ESP_LOGE(TAG, "Failed to open file for writing"); */
/*         return; */
/*     } */

/*     fprintf(f, "Test file"); */

/*     fclose(f); */
/* } */

void app_main(void) {
  ESP_LOGI("main","App initialized");
  
  tasks_init();

  /* xTaskCreatePinnedToCore(task_afe,"task_afe", 10000, NULL, 20, &task_afe_handle, 1); */
  /* xTaskCreatePinnedToCore(task_imu,"task_imu", 10000, NULL, 0, &task_imu_handle, 1); */
  /* xTaskCreatePinnedToCore(task_write,"task_write", 10000, NULL, 0, &task_write_handle, 0); */
  /* xTaskCreatePinnedToCore(task_recv,"task_recv", 10000, NULL, 0, &task_recv_handle, 0); */
  xTaskCreatePinnedToCore(task_sound,"task_sound", 1024*120, NULL, 0, &task_sound_handle, 0);

  /* sdcard_mount(SDCARD_PIN_CLK, SDCARD_PIN_CMD, SDCARD_PIN_DATA0, SDCARD_PIN_DATA1, SDCARD_PIN_DATA2, SDCARD_PIN_DATA3, SDCARD_PIN_CD); */

  /* sdcard_write_test(); */
  /* sdcard_read_test(); */
  
  /* sdcard_unmount(); */
}
