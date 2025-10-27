// transmitter

#include "main.h"
#include "sippy_init.h"
#include "sippy_wifi.h"
#include "LSM6DSL.h"

#define BROADCAST_PORT 8080

void ip_broadcaster(void *arg)
{
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in info = {
        .sin_addr.s_addr = INADDR_BROADCAST,
        .sin_family = AF_INET,
        .sin_port = htons(BROADCAST_PORT)};
    char one = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
    while (1)
    {
        char buffer[100] = ZEROS;
        LSM6DSL_Data d = ZEROS;
        LSM6DSL_ReadData(&d);

        sprintf(buffer, "%f %f %f %f %f %f",
                d.accel_x, d.accel_y, d.accel_z,
                d.gyro_x, d.gyro_y, d.gyro_z);
        sendto(s, buffer, strlen(buffer), 0, (struct sockaddr *)&info, sizeof(info));
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    const char *TAG = __func__;
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Running!");

    // starting BLE
    //  sippy_ble_start();

    // starting wifi
    // sippy_wifi_start();

    sippy_gpio_init();
    sippy_spi_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LSM6DSL_set_handle(get_lsm6dsl_handle(), get_spi_mutex());
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LSM6DSL_Init();

    //wait until IP has been received by DHCP server
    do
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } while (sippy_connected_to_wifi());

    //starting broadcaster task
    xTaskCreate(ip_broadcaster, "ip_broadcaster", 4096, NULL, tskIDLE_PRIORITY, NULL);

    while (1)
    {
        ESP_LOGI(TAG, "Running...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}