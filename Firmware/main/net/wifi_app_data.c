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

static void wifi_udp_send_packet(const uint8_t* data, uint8_t data_len) {
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

/* void wifi_udp_send_data(const udp_packet_t* data) { */
/*   uint8_t buff[sizeof(udp_packet_t)]; */

  
/* } */

void wifi_udp_send_data(const uint32_t* data) {
  wifi_udp_send_packet((const uint8_t*) data, sizeof(uint32_t));
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
