#include "tasks.h"

#include "esp_log.h"

#include <sys/time.h>
#include <string.h>

#include "config.h"

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
#define ADS1299_PIN_START	14

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

static EventGroupHandle_t event_group;
static RingbufHandle_t buff_afe;
static RingbufHandle_t buff_imu;
static struct timeval time_afe;
static struct timeval time_imu;

static config_t *config;

static TaskHandle_t imu_task_handle = NULL;
static void IRAM_ATTR imu_isr(void) {
    BaseType_t hp_task_woken = pdFALSE;

    if (imu_task_handle != NULL) {
      gettimeofday(&time_imu, NULL);
      vTaskNotifyGiveFromISR(imu_task_handle, &hp_task_woken);
    }

    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}


static TaskHandle_t afe_task_handle = NULL;
static void IRAM_ATTR afe_isr(void) {
    BaseType_t hp_task_woken = pdFALSE;

    if (afe_task_handle != NULL) {
      gettimeofday(&time_afe, NULL);
      vTaskNotifyGiveFromISR(afe_task_handle, &hp_task_woken);
    }

    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
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
  xEventGroupSetBits(event_group, EVENT_BIT_ONLINE_MODE | EVENT_BIT_IDLE);
  
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

  config = get_config();
  default_config(config);
  
  buff_afe = xRingbufferCreateNoSplit(sizeof(ads1299_data_t), 500);
  buff_imu = xRingbufferCreateNoSplit(sizeof(mpu6050_data_t), 500);
}


static int32_t offset_coeffs[NUM_ELECTRODES] = {0};
static bool calibration = false;

void task_afe(void *args) {
  afe_task_handle = xTaskGetCurrentTaskHandle();

  ads1299_data_t data;
  
  ads1299_init(ADS1299_PIN_MOSI, ADS1299_PIN_MISO, ADS1299_PIN_SCK, ADS1299_PIN_CS, ADS1299_PIN_DRDY, ADS1299_PIN_START, afe_isr);

  //////////////////////CALIBRATION///////////////////////////
  if (calibration) { 
    float acc[NUM_ELECTRODES] = {0};

    memset(acc, 0, sizeof(acc));
  
    ads1299_config_t cal_config = {
      .sample_rate = ADS1299_CONFIG_SAMPLERATE_1000SPS,
      .comm_ref = false
    };

    for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
      cal_config.electrodes[i] = ADS1299_CONFIG_CHANNELMODE_SHORTED;
      cal_config.gains[i] = ADS1299_CONFIG_CHANNELGAIN_1;
    }

    ads1299_send_config(&cal_config);

    ESP_LOGI(TAG, "Starting calibration...");

    ads1299_start();

    const uint16_t OFFSET_CAL_LEN = 2000;

    for (uint16_t i = 0; i < OFFSET_CAL_LEN; i++) {
      ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

      ads1299_read(&data);

      if (strcmp((const char*)data.status, "\xC0\x00\x00") != 0) {
	i--;
	continue;
      }
    
      for (uint8_t j = 0; j < NUM_ELECTRODES; j++) {
	acc[j] += (float) data.channel[j] / OFFSET_CAL_LEN;
      }
    }

    ads1299_stop();

    for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
      offset_coeffs[i] = (int32_t)(acc[i]);
    }

    ESP_LOGI(TAG, "Calibration offsets: %" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32,
      offset_coeffs[0], offset_coeffs[1], offset_coeffs[2], offset_coeffs[3], offset_coeffs[4], offset_coeffs[5], offset_coeffs[6], offset_coeffs[7]);
  }
  ////////////////////////////////////////////////////////////
  
  ads1299_send_config(&(config->ads1299_config));

  while (1) {
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    ads1299_read(&data);

    if (strcmp((const char*)data.status, "\xC0\x00\x00") != 0) continue;
    
    for (uint8_t i = 0; i < NUM_ELECTRODES; i++) {
      data.channel[i] -= offset_coeffs[i];
    }
    
    data.timestamp = time_afe;

    xRingbufferSend(buff_afe, &data, sizeof(data), 0);
  }
}

void task_imu(void *args) {
  imu_task_handle = xTaskGetCurrentTaskHandle();

  mpu6050_data_t data;
  mpu6050_init(MPU6050_PIN_SCL, MPU6050_PIN_SDA, MPU6050_PIN_INT, imu_isr);
  mpu6050_start();
  while(1) {
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    mpu6050_read_data(&data);
    data.timestamp = time_imu;
    
    xRingbufferSend(buff_imu, &data, sizeof(data), 0);
  }
}

void task_write(void *args) {
  ESP_LOGI(TAG, "sizeof(udp_packet_t) = %d", sizeof(udp_packet_t));
  while(1) {
    EventBits_t bits = xEventGroupWaitBits(event_group, EVENT_BIT_OFFLINE_MEAS | EVENT_BIT_ONLINE_MEAS, pdFALSE, pdFALSE, portMAX_DELAY);
    if ((bits & EVENT_BIT_OFFLINE_MEAS) == EVENT_BIT_OFFLINE_MEAS) {
      // TODO: Offline data saving

      size_t size;
      uint8_t count = 0;
      mpu6050_data_t *data_imu = NULL;
      while (count < UDP_IMU_BUFF_SIZE) {
	data_imu = (mpu6050_data_t *) xRingbufferReceive(buff_imu, &size, 0);
	if (data_imu == NULL) break;
        ESP_LOGI(TAG, "X: %d, Y: %d, Z: %d", data_imu->accel.x, data_imu->accel.y, data_imu->accel.z);
	vRingbufferReturnItem(buff_imu, data_imu);
	count++;
      }
      
      vTaskDelay(pdMS_TO_TICKS(30));      
    } else {
      udp_packet_t packet = {0};
      uint8_t count;

      size_t size;

      // Read AFE data from buffer
      count = 0;
      ads1299_data_t* data_afe = NULL;
      data_afe = (ads1299_data_t *) xRingbufferReceive(buff_afe, &size, 0);
      if (data_afe != NULL) {
	packet.abs_time = data_afe->timestamp.tv_sec;
	memcpy(&packet.elec_data[count], data_afe->channel, sizeof(data_afe->channel));
	packet.elec_time[count++] = data_afe->timestamp.tv_usec;
	vRingbufferReturnItem(buff_afe, data_afe);
	while (count < UDP_ELEC_BUFF_SIZE) {
	  data_afe = (ads1299_data_t *) xRingbufferReceive(buff_afe, &size, 0);
	  if (data_afe == NULL) break;
	  memcpy(&packet.elec_data[count], data_afe->channel, sizeof(data_afe->channel));
	  packet.elec_time[count] = data_afe->timestamp.tv_usec;
	  if (data_afe->timestamp.tv_sec > packet.abs_time)
	      packet.elec_time[count] += 1000000;
	  vRingbufferReturnItem(buff_afe, data_afe);
	  count++;
	}
      }
      packet.num_elec = count;

      // Read IMU data from buffer
      count = 0;
      mpu6050_data_t *data_imu = NULL;
      while (count < UDP_IMU_BUFF_SIZE) {
	data_imu = (mpu6050_data_t *) xRingbufferReceive(buff_imu, &size, 0);
	if (data_imu == NULL) break;
	if (data_imu->timestamp.tv_sec > packet.abs_time)
	  packet.imu_time[count] += 1000000;
	packet.accel_data[count][0] = data_imu->accel.x; packet.accel_data[count][1] = data_imu->accel.y; packet.accel_data[count][2] = data_imu->accel.z;
	packet.gyro_data[count][0] = data_imu->gyro.x; packet.gyro_data[count][1] = data_imu->gyro.y; packet.gyro_data[count][2] = data_imu->gyro.z;
	vRingbufferReturnItem(buff_imu, data_imu);
	count++;
      }
      packet.num_imu = count;
      
      wifi_udp_send_packet(&packet);
      vTaskDelay(pdMS_TO_TICKS(30));
    }
  }
}

void task_recv(void *args) {
  uint8_t buff[20]; // TODO: Stack overflow
  
  while (1) {
    xEventGroupWaitBits(event_group, EVENT_BIT_ONLINE_MODE, pdFALSE, pdFALSE, portMAX_DELAY);
    if (wifi_udp_recv_packet(buff)) {
      ESP_LOGI(TAG, "MSG received from udp: \"%s\"", buff);
      if (strcmp((const char*) buff, "start") == 0) {
	// Clean all the data buffers
	xEventGroupClearBits(event_group, EVENT_BIT_IDLE);
	xEventGroupSetBits(event_group, EVENT_BIT_ONLINE_MEAS);	
	wifi_http_stop();
	vRingbufferReset(buff_imu);
	vRingbufferReset(buff_afe);	
	ads1299_start();
      } else if (strcmp((const char*) buff, "stop") == 0) {
	ads1299_stop();
	wifi_http_start();
	xEventGroupClearBits(event_group, EVENT_BIT_ONLINE_MEAS);
	xEventGroupSetBits(event_group, EVENT_BIT_IDLE);
      }
    }
  }
}
