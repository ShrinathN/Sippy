// transmitter
#include "main.h"
#include "sippy_init.h"
#include "sippy_wifi.h"
#include "LSM6DSL.h"
#include "ws2812b.h"

static const char *TAG = "example_main";

int app_main(void)
{
    // Change these to your wiring
    vTaskDelay(pdMS_TO_TICKS(1000));

    sippy_gpio_init();
    sippy_spi_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LSM6DSL_set_handle(get_lsm6dsl_handle(), get_spi_mutex());
    vTaskDelay(100 / portTICK_PERIOD_MS);
    LSM6DSL_Init();

    ws2812b_init();

    while (1)
    {
        for (uint8_t i = 0; i < 10; i++)
        {
            ws2812b_write(255,0,0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            // Off
            ws2812b_write(0, 0, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        esp_deep_sleep((60 * 30) * 1000000ULL);
    }
}
