#pragma once

#include <WiFiUdp.h>

#define UDP_PORT 69

class DataServer {
private:
    WiFiUDP udp;
    uint8_t active;
    IPAddress user_ip;
    uint16_t user_port;
public:
    uint8_t init();
    uint8_t is_active();
    void recv_packet();
    void send_packet();
};
