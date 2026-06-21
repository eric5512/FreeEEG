#include "wifi_app.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "config.h"
#include "index.html.h"

#define TAG "wifi_http"

#define MIN(a,b) ((a < b) ? a : b)

static httpd_handle_t http_server = NULL;

static esp_err_t http_root_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *) net_index_html, net_index_html_len);
}

/* static esp_err_t http_status_handler(httpd_req_t *req) { */
/*   char content[] = ""; */
/*   return httpd_resp_send(req,  */
/* } */

ads1299_config_samplerate_t parse_sample_rate(const char *value) {
    if (strcmp(value, "250") == 0) return ADS1299_CONFIG_SAMPLERATE_250SPS;
    if (strcmp(value, "500") == 0) return ADS1299_CONFIG_SAMPLERATE_500SPS;
    if (strcmp(value, "1000") == 0) return ADS1299_CONFIG_SAMPLERATE_1000SPS;
    if (strcmp(value, "2000") == 0) return ADS1299_CONFIG_SAMPLERATE_2000SPS;
    if (strcmp(value, "4000") == 0) return ADS1299_CONFIG_SAMPLERATE_4000SPS;
    if (strcmp(value, "8000") == 0) return ADS1299_CONFIG_SAMPLERATE_8000SPS;
    if (strcmp(value, "16000") == 0) return ADS1299_CONFIG_SAMPLERATE_16000SPS;

    return ADS1299_CONFIG_SAMPLERATE_250SPS;
}

ads1299_config_channelgain_t parse_channel_gain(const char *value) {
    if (strcmp(value, "1") == 0) return ADS1299_CONFIG_CHANNELGAIN_1;
    if (strcmp(value, "2") == 0) return ADS1299_CONFIG_CHANNELGAIN_2;
    if (strcmp(value, "4") == 0) return ADS1299_CONFIG_CHANNELGAIN_4;
    if (strcmp(value, "6") == 0) return ADS1299_CONFIG_CHANNELGAIN_6;
    if (strcmp(value, "8") == 0) return ADS1299_CONFIG_CHANNELGAIN_8;
    if (strcmp(value, "12") == 0) return ADS1299_CONFIG_CHANNELGAIN_12;
    if (strcmp(value, "24") == 0) return ADS1299_CONFIG_CHANNELGAIN_24;

    return ADS1299_CONFIG_CHANNELGAIN_24;
}

ads1299_config_channelmode_t parse_channel_mode(const char *value) {
    if (strcmp(value, "-1") == 0) return ADS1299_CONFIG_CHANNELMODE_PWRDWN;
    if (strcmp(value, "0") == 0) return ADS1299_CONFIG_CHANNELMODE_NORMAL;
    if (strcmp(value, "1") == 0) return ADS1299_CONFIG_CHANNELMODE_SHORTED;
    if (strcmp(value, "2") == 0) return ADS1299_CONFIG_CHANNELMODE_BIASMEAS;
    if (strcmp(value, "3") == 0) return ADS1299_CONFIG_CHANNELMODE_MVDD;
    if (strcmp(value, "4") == 0) return ADS1299_CONFIG_CHANNELMODE_TEMP;
    if (strcmp(value, "5") == 0) return ADS1299_CONFIG_CHANNELMODE_TEST;
    if (strcmp(value, "6") == 0) return ADS1299_CONFIG_CHANNELMODE_BIASDRP;
    if (strcmp(value, "7") == 0) return ADS1299_CONFIG_CHANNELMODE_BIASDRN;

    return ADS1299_CONFIG_CHANNELMODE_PWRDWN;
}

config_filtfreq_t parse_filter_freq(const char *value) {
    if (strcmp(value, "60") == 0)
        return CONFIG_FILTFREQ_60HZ;

    return CONFIG_FILTFREQ_50HZ;
}

static esp_err_t http_update_handler(httpd_req_t *req) {
    char content[256];

    int recv_len = httpd_req_recv(
        req,
        content,
        MIN(req->content_len, sizeof(content) - 1)
    );

    if (recv_len <= 0) {
        return ESP_FAIL;
    }

    content[recv_len] = '\0';

    ESP_LOGI(TAG, "POST content: %s\n", content);

    const uint8_t BUFF_LEN = 16;
    char buff[BUFF_LEN];

    config_t *config = get_config();
    
    for (uint8_t i = 0; i < 8; ++i) {
      char mode[10];
      char gain[10];

      sprintf(mode, "ch%d_mode", i+1);
      if (httpd_query_key_value(content, mode, buff, BUFF_LEN) == ESP_OK) {
        config->ads1299_config.electrodes[i] = parse_channel_mode(buff);
      } else goto fail;
      
      sprintf(gain, "ch%d_gain", i+1);
      if (httpd_query_key_value(content, gain, buff, BUFF_LEN) == ESP_OK) {
        config->ads1299_config.gains[i] = parse_channel_gain(buff);
      } else goto fail;
    }

    if (httpd_query_key_value(content, "filt_freq", buff, BUFF_LEN) == ESP_OK) {
      config->filt_freq = parse_filter_freq(buff);
    } else goto fail;

    if (httpd_query_key_value(content, "sample_rate", buff, BUFF_LEN) == ESP_OK) {
      config->ads1299_config.sample_rate = parse_sample_rate(buff);
    } else goto fail;

    
    if (httpd_query_key_value(content, "comm_ref", buff, BUFF_LEN) == ESP_OK)
      config->ads1299_config.comm_ref = true;
    else
      config->ads1299_config.comm_ref = false;
    
    httpd_resp_sendstr(req, "Configuration updated");

    send_config(config);
    
    return ESP_OK;

 fail:
    ESP_LOGE(TAG, "Malfromed configuration request"); 
    httpd_resp_sendstr(req, "Error updating configuration");
    return ESP_FAIL;
}

void wifi_http_start(void) {
  if (http_server != NULL) {
    return;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  ESP_ERROR_CHECK(httpd_start(&http_server, &config));

  httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = http_root_handler,
    .user_ctx = NULL,
  };

  httpd_uri_t update_uri = {
    .uri = "/update_config",
    .method = HTTP_POST,
    .handler = http_update_handler,
    .user_ctx = NULL,
  };
  
  ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &root_uri));
  ESP_ERROR_CHECK(httpd_register_uri_handler(http_server, &update_uri));

  ESP_LOGI(TAG, "HTTP server initialized");
}

void wifi_http_stop(void) {
  if (http_server != NULL) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(httpd_stop(http_server));
    http_server = NULL;
  }
}
