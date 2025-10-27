#ifndef MAIN_H
#define MAIN_H

// common SDK defines
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_sleep.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <stdlib.h>
#include <string.h>
#include "driver/rmt_tx.h"
#include "esp_err.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "freertos/FreeRTOSConfig.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/gap/ble_svc_gap.h"

// macros
#define ZEROS { \
    0,          \
}

// ADC pins
#define PIN_VBAT_OUT 1

//SPI Pins
#define PIN_SPI_MISO GPIO_NUM_7
#define PIN_SPI_MOSI GPIO_NUM_5
#define PIN_SPI_SCK GPIO_NUM_15
#define PIN_LSM6DSL_SS GPIO_NUM_13
#define PIN_LSM6DSL_INT1 GPIO_NUM_12
#define PIN_BUZZER GPIO_NUM_45
#define PIN_LED_DOUT GPIO_NUM_14

#endif