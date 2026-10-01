#include "wifi_app.h"

#include <string.h>
#include <errno.h>

#include "esp_log.h"

#include "lwip/sockets.h"
#include "lwip/inet.h"

#define TAG "wifi_udp"

#define WIFI_UDP_LOCAL_PORT 69

static int udp_sock = -1;
static struct sockaddr_in udp_dest_addr;

void wifi_udp_send(const uint8_t* data, size_t data_len) {
    if (udp_sock < 0 || data == NULL || data_len == 0) {
        return;
    }

    udp_dest_addr.sin_port = htons(6969);
    
    int err = sendto(
        udp_sock,
        data,
        data_len,
        0,
        (struct sockaddr *)&udp_dest_addr,
        sizeof(udp_dest_addr)
    );

    if (err < 0) {
        ESP_LOGE(TAG, "UDP send failed: errno=%d", errno);
    }
}

void wifi_udp_start(void) {
    if (udp_sock >= 0) {
        close(udp_sock);
        udp_sock = -1;
    }

    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (udp_sock < 0) {
        ESP_LOGE(TAG, "Could not create UDP socket: errno=%d", errno);
        return;
    }

    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(WIFI_UDP_LOCAL_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(udp_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "Could not bind UDP socket: errno=%d", errno);
        close(udp_sock);
        udp_sock = -1;
        return;
    }

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = 10000,
    };

    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ESP_LOGI(TAG, "UDP initialized: local_port=%u",
             WIFI_UDP_LOCAL_PORT);
}

void wifi_udp_stop(void) {
    if (udp_sock >= 0) {
        close(udp_sock);
        udp_sock = -1;
    }
}

void wifi_udp_send_packet(const udp_packet_t* packet) {
  uint8_t buffer[sizeof(udp_packet_t)];
  uint8_t *p_buff = buffer;
  
  *((uint32_t *) p_buff) = packet->abs_time;
  p_buff += 4;
  
  *p_buff = packet->num_elec;
  p_buff += 1;
  *p_buff = packet->num_elec_samp;
  p_buff += 1;
  memcpy(p_buff, packet->elec_time, packet->num_elec_samp*4);
  p_buff += packet->num_elec_samp*4;
  memcpy(p_buff, packet->elec_data, packet->num_elec*packet->num_elec_samp*4);
  p_buff += packet->num_elec*packet->num_elec_samp*4;

  *p_buff = packet->num_imu;
  p_buff += 1;
  memcpy(p_buff, packet->imu_time, packet->num_imu*4);
  p_buff += packet->num_imu*4;
  memcpy(p_buff, packet->accel_data, packet->num_imu*6);
  p_buff += packet->num_imu*6;
  memcpy(p_buff, packet->gyro_data, packet->num_imu*6);
  p_buff += packet->num_imu*6;
  
  *p_buff = packet->num_events;
  p_buff += 1;
  memcpy(p_buff, packet->events_time, packet->num_events*4);
  p_buff += packet->num_events*4;
  memcpy(p_buff, packet->events, packet->num_events*4);
  p_buff += packet->num_events*4;

  uint16_t size = p_buff - buffer;
  
  wifi_udp_send((const uint8_t*) buffer, size);
}

bool wifi_udp_recv_packet(uint8_t *buffer) {
    if (udp_sock < 0 || buffer == NULL) {
        return false;
    }

    socklen_t socklen = sizeof(udp_dest_addr);

    int len = recvfrom(
        udp_sock,
        buffer,
        256,
        0,
        (struct sockaddr *)&udp_dest_addr,
        &socklen
    );

    if (len > 0) {
        buffer[len] = '\0';

        ESP_LOGI(TAG, "UDP packet received from %s:%u",
                 inet_ntoa(udp_dest_addr.sin_addr),
                 udp_dest_addr.sin_port);


	return true;
    }

    return false;
}

void log_udp_packet(const udp_packet_t *packet)
{
    if (packet == NULL) {
        ESP_LOGE(TAG, "udp_packet is NULL");
        return;
    }

    ESP_LOGI(TAG, "===== UDP PACKET =====");
    ESP_LOGI(TAG, "abs_time: %" PRIu32, packet->abs_time);

    ESP_LOGI(TAG,
             "Electrode samples: %u, electrodes: %u",
             packet->num_elec_samp,
             packet->num_elec);

    for (uint8_t i = 0;
         i < packet->num_elec_samp && i < UDP_ELEC_BUFF_SIZE;
         i++) {

        ESP_LOGI(TAG,
                 "EEG[%u] time=%" PRIu32,
                 i,
                 packet->elec_time[i]);

        for (uint8_t ch = 0;
             ch < packet->num_elec && ch < 8;
             ch++) {

            ESP_LOGI(TAG,
                     "  CH%u: %" PRIu32,
                     ch + 1,
                     packet->elec_data[i][ch]);
        }
    }

    ESP_LOGI(TAG, "IMU samples: %u", packet->num_imu);

    for (uint8_t i = 0;
         i < packet->num_imu && i < UDP_IMU_BUFF_SIZE;
         i++) {

        ESP_LOGI(TAG,
                 "IMU[%u] time=%" PRIu32
                 " accel=[%u, %u, %u]"
                 " gyro=[%u, %u, %u]",
                 i,
                 packet->imu_time[i],
                 packet->accel_data[i][0],
                 packet->accel_data[i][1],
                 packet->accel_data[i][2],
                 packet->gyro_data[i][0],
                 packet->gyro_data[i][1],
                 packet->gyro_data[i][2]);
    }

    ESP_LOGI(TAG, "Events: %u", packet->num_events);

    for (uint8_t i = 0;
         i < packet->num_events && i < UDP_EVENT_BUFF_SIZE;
         i++) {

        ESP_LOGI(TAG,
                 "EVENT[%u] time=%" PRIu32 " value=%u",
                 i,
                 packet->events_time[i],
                 packet->events[i]);
    }

    ESP_LOGI(TAG, "======================");
}
