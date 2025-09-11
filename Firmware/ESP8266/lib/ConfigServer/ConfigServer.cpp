#include "ConfigServer.h"

#include "Config.h"
#include "index_html.h"

void ConfigServer::on_root() {
    server.send(200, "text/html", (const char *)index_html);
}

void ConfigServer::on_config(){
    server.send(200, "text/plain", "");
    if (server.args() == 1) {
        uint32_t value = server.arg(0).toInt();
        Config::update_config(server.argName(0).c_str(), &value);
    }
}

void ConfigServer::init() {
    server.on("/", [this](){ on_root(); });
    server.on("/config", [this](){ on_config(); });
    server.begin();
}

void ConfigServer::handle_client() {
    server.handleClient();
}