#include "sippy_wifi.h"

//private functions
static void sippy_global_wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


//private variables
static bool connected_to_wifi = false;


static void sippy_global_wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const char *TAG = __func__;
    static int s_retry_num = 0;

    // when WiFi station mode is started
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
    }

    // retry forever
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
    }

    // when we get IP
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        connected_to_wifi = true;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

bool sippy_connected_to_wifi(void)
{
    return connected_to_wifi;
}

void sippy_wifi_start(void)
{
    const char *TAG = __func__;

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sippy_global_wifi_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sippy_global_wifi_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi init done");
}
