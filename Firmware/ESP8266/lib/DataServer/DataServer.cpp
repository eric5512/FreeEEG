#include "DataServer.h"

uint8_t DataServer::init() {
    active = 0;
    return udp.begin(UDP_PORT);
}

uint8_t DataServer::is_active() {
    return active;
}

void DataServer::recv_packet() {
    uint8_t buff[20];
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len; 
        if ((len = udp.read(buff, 20))) buff[len] = '\0';

        if (strcmp("start", (const char *)buff) == 0) {
            user_ip = udp.remoteIP();
            user_port = udp.remotePort();
            active = 1;
        } else if (strcmp("stop", (const char *)buff) == 0) 
            active = 0;
    }
}

void DataServer::send_packet() {
    udp.beginPacket(user_ip, user_port);
    udp.write("test", 4); // TODO: Send the real data from the EOS S3 SPI interface
    udp.endPacket();
}
