#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class ConfigServer {
private:
    ESP8266WebServer server;

    void on_root();
    void on_config();
public:
    ConfigServer() : server(80) {};
    void init();
    void handle_client();
};