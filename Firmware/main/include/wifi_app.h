#pragma once

#include <inttypes.h>
#include <stdbool.h>

// AP creation
#define AP_NAME   "FreeEEG"
#define AP_PASSWD "12345678"
void wifi_ap_init(void);
void wifi_ap_start(void);
void wifi_ap_stop(void);

// HTTP communication
void wifi_http_start(void);
void wifi_http_stop(void);

// UDP communication
#define UDP_ELEC_BUFF_SIZE  20
#define UDP_IMU_BUFF_SIZE   10
#define UDP_EVENT_BUFF_SIZE 2

typedef struct  __attribute__((packed)) {
  uint32_t abs_time;
  uint8_t num_elec;
  uint16_t elec_time[UDP_ELEC_BUFF_SIZE];
  uint32_t elec_data[UDP_ELEC_BUFF_SIZE][8];

  uint8_t num_imu;
  uint16_t imu_time[UDP_IMU_BUFF_SIZE];
  uint16_t accel_data[UDP_IMU_BUFF_SIZE][3];
  uint16_t gyro_data[UDP_IMU_BUFF_SIZE][3];

  uint8_t num_events;
  uint16_t events_time[UDP_EVENT_BUFF_SIZE];
  uint8_t events[UDP_EVENT_BUFF_SIZE];
} udp_packet_t;

void wifi_udp_start(void);
void wifi_udp_stop(void);
void wifi_udp_send_data(const uint32_t* data);
void wifi_udp_send_packet(const udp_packet_t* data);
bool wifi_udp_recv_packet(uint8_t *buffer);
