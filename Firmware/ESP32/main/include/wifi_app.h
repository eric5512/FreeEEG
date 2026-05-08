#pragma once

// AP creation
char wifi_ap_ip;
void wifi_ap_init(void);
void wifi_ap_start(void);
void wifi_ap_stop(void);

// HTTP communication


// UDP communication
char wifi_udp_user_ip[];
uint16_t wifi_udp_user_port;
void wifi_udp_send_packet(void);
void wifi_udp_recv_packet(void);

