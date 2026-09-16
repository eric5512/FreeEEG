#include "wifi_app.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#define TAG "wifi_app"

char wifi_ap_ip[16] = "192.168.1.1";

static bool netif_initialized = false;
static bool wifi_initialized = false;

void wifi_ap_init(void) {
    if (!netif_initialized) {
        esp_err_t ret = nvs_flash_init();

        if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
            ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        } else {
            ESP_ERROR_CHECK(ret);
        }

        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();

        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_get_ip_info(ap_netif, &ip_info));
        snprintf(wifi_ap_ip, sizeof(wifi_ap_ip), IPSTR, IP2STR(&ip_info.ip));

        netif_initialized = true;
    }

    if (!wifi_initialized) {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config_t wifi_config = {
            .ap = {
                .ssid = AP_NAME,
                .ssid_len = strlen(AP_NAME),
                .channel = 1,
                .password = AP_PASSWD,
                .max_connection = 4,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            },
        };

        if (strlen(AP_PASSWD) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

        wifi_initialized = true;
    }
}

void wifi_ap_start(void)
{
    if (!wifi_initialized) {
        wifi_ap_init();
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started: SSID=%s PASS=%s IP=%s",
             AP_NAME,
             AP_PASSWD,
             wifi_ap_ip);
}

void wifi_ap_stop(void)
{
    if (wifi_initialized) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
    }
}
