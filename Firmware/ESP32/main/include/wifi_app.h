#pragma once

#include <inttypes.h>

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
void wifi_udp_start(void);
void wifi_udp_stop(void);
void wifi_udp_send_packet(const uint8_t* data, uint8_t data_len);
void wifi_udp_recv_packet(uint8_t *buffer);
