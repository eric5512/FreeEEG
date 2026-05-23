#include "tasks.h"

#include "esp_log.h"

#include "ads1299.h"
#include "wifi_app.h"
#include "RGB_LED.h"
#include "mpu6050.h"

#include "driver/gpio.h"

#define TAG "tasks"

#define ADS1299_PIN_MOSI	11 
#define ADS1299_PIN_MISO	13
#define ADS1299_PIN_SCK		12
#define ADS1299_PIN_DRDY	15
#define ADS1299_PIN_CS		10
#define ADS1299_PIN_START	10

#define RGB_PIN_R	17
#define RGB_PIN_G	21
#define RGB_PIN_B	18

#define MPU6050_PIN_SDA	38
#define MPU6050_PIN_SCL	39
#define MPU6050_PIN_INT	40

#define SDCARD_PIN_DAT0 2
#define SDCARD_PIN_DAT1 1
#define SDCARD_PIN_DAT2 3
#define SDCARD_PIN_DAT3 8
#define SDCARD_PIN_CMD  7
#define SDCARD_PIN_CLK  6
#define SDCARD_PIN_CD   9

#define BATTERY_PIN	4

#define SWITCH_PIN      16

#define EVENT_BIT_OFFLINE_MEAS  (1<<0)
#define EVENT_BIT_ONLINE_MODE	(1<<1)
#define EVENT_BIT_ONLINE_MEAS	(1<<2)
#define EVENT_BIT_IDLE		(1<<3)
#define EVENT_BIT_IMU_AVAIL	(1<<4)
#define EVENT_BIT_AFE_AVAIL	(1<<5)


static EventGroupHandle_t event_group;
static RingbufHandle_t buff_afe;
static RingbufHandle_t buff_imu;

static void imu_isr() {
  xEventGroupSetBitsFromISR(event_group, EVENT_BIT_IMU_AVAIL, NULL);
}

static void afe_isr() {
  xEventGroupSetBitsFromISR(event_group, EVENT_BIT_AFE_AVAIL, NULL);
}

static int level;

static void IRAM_ATTR switch_isr(void *args) {
  level = gpio_get_level(SWITCH_PIN);
  if (level == 1) {
    xEventGroupClearBitsFromISR(event_group, EVENT_BIT_ONLINE_MODE | EVENT_BIT_ONLINE_MEAS | EVENT_BIT_IDLE);
    xEventGroupSetBitsFromISR(event_group, EVENT_BIT_OFFLINE_MEAS, NULL); 
  } else {
    xEventGroupClearBitsFromISR(event_group, EVENT_BIT_OFFLINE_MEAS); 
    xEventGroupSetBitsFromISR(event_group, EVENT_BIT_ONLINE_MODE | EVENT_BIT_IDLE, NULL);
  }
}

void tasks_init() {
  event_group = xEventGroupCreate();
  
  gpio_config_t switch_conf = {
            .pin_bit_mask = 1ULL << SWITCH_PIN,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,

            /*
             * ADS1299 DRDY is active low.
             * Interrupt on falling edge.
             */
            .intr_type = GPIO_INTR_ANYEDGE,
        };

  ESP_ERROR_CHECK(gpio_config(&switch_conf));

  ESP_ERROR_CHECK(gpio_install_isr_service(0));

  ESP_ERROR_CHECK(gpio_isr_handler_add(
				       SWITCH_PIN,
				       switch_isr,
				       NULL
				       )
		 );

  wifi_ap_init();
  wifi_ap_start();
  wifi_http_start();
  wifi_udp_start();

  RGB_init(RGB_PIN_R, RGB_PIN_G, RGB_PIN_B);
  /* RGB_set(0,0,1); */

  /* mpu6050_init(MPU6050_PIN_SCL, MPU6050_PIN_SDA, MPU6050_PIN_INT, imu_isr); */
  ads1299_init(ADS1299_PIN_MOSI, ADS1299_PIN_MISO, ADS1299_PIN_SCK, ADS1299_PIN_CS, ADS1299_PIN_DRDY, afe_isr);

  config_t config = {
    .sample_rate = 250,
    .filt_freq = 50,
    .electrodes = {0},
    .comm_ref = false
  };

  config.electrodes[0] = true;

  ads1299_send_config(&config);

  xEventGroupSetBits(event_group, EVENT_BIT_ONLINE_MODE | EVENT_BIT_IDLE);

  buff_afe = xRingbufferCreateNoSplit(sizeof(uint32_t), 500);
  buff_imu = xRingbufferCreateNoSplit(sizeof(uint32_t), 500);

  /* while(1) { */
  /*   xEventGroupWaitBits(event_group, EVENT_BIT_OFFLINE_MEAS, pdFALSE, pdFALSE, portMAX_DELAY); */
  /*   RGB_set(0,1,0); */
  /*   ESP_LOGI(TAG, "Measurement on"); */
  /*   xEventGroupWaitBits(event_group, EVENT_BIT_IDLE, pdFALSE, pdFALSE, portMAX_DELAY); */
  /*   RGB_set(0,0,1); */
  /*   ESP_LOGI(TAG, "Measurement off"); */
  /* } */
}

void task_afe(void *args) {
  ads1299_data_t data;
  ads1299_start();
  while(1) {
    xEventGroupWaitBits(event_group, EVENT_BIT_ONLINE_MEAS | EVENT_BIT_OFFLINE_MEAS, pdFALSE, pdFALSE, portMAX_DELAY);
    xEventGroupWaitBits(event_group, EVENT_BIT_AFE_AVAIL, pdTRUE, pdFALSE, portMAX_DELAY);
    ads1299_read(&data);
    xRingbufferSend(buff_afe,
                    data.channel,
                    sizeof(uint32_t),
                    portMAX_DELAY);
  }
}

void task_imu(void *args) {
  mpu_data_t data;
  while(1) {
    xEventGroupWaitBits(event_group, EVENT_BIT_IMU_AVAIL, pdTRUE, pdFALSE, portMAX_DELAY);
    mpu6050_read_data(&data);
    ESP_LOGI(TAG, "Accel Z axis data: %d", data.acc.z);
  }
}

void task_write(void *args) {
  while(1) {
    EventBits_t bits = xEventGroupWaitBits(event_group, EVENT_BIT_OFFLINE_MEAS | EVENT_BIT_ONLINE_MEAS, pdFALSE, pdFALSE, portMAX_DELAY);
    if ((bits & EVENT_BIT_OFFLINE_MEAS) == EVENT_BIT_OFFLINE_MEAS) {
      // TODO: Offline data saving
      
    } else {
      udp_packet_t packet = {0};

      size_t size;

      uint32_t* data = (uint32_t *) xRingbufferReceive(buff_afe, &size, portMAX_DELAY);

      vRingbufferReturnItem(buff_afe, data);
      wifi_udp_send_data(data);
      /* vTaskDelay(pdMS_TO_TICKS(100)); */
    }
  }
}

void task_config(void *args) {
  while(1) {
    xEventGroupWaitBits(event_group, EVENT_BIT_IDLE, pdFALSE, pdFALSE, portMAX_DELAY);
  }
}

void task_recv(void *args) {
  uint8_t buff[20]; // TODO: Stack overflow
  
  while (1) {
    xEventGroupWaitBits(event_group, EVENT_BIT_ONLINE_MODE, pdFALSE, pdFALSE, portMAX_DELAY);
    if (wifi_udp_recv_packet(buff)) {
      ESP_LOGI(TAG, "MSG received from udp: \"%s\"", buff);
      if (strcmp((const char*) buff, "start") == 0) {
	xEventGroupClearBits(event_group, EVENT_BIT_IDLE);
	xEventGroupSetBits(event_group, EVENT_BIT_ONLINE_MEAS);
      } else if (strcmp((const char*) buff, "stop") == 0) {
	xEventGroupClearBits(event_group, EVENT_BIT_ONLINE_MEAS);
	xEventGroupSetBits(event_group, EVENT_BIT_IDLE);
      }
    }
  }
}
