#ifndef SIPPY_INIT_H
#define SIPPY_INIT_H

#include "main.h"


// public function declaration
void sippy_gpio_init();
void sippy_spi_init();
spi_device_handle_t *get_lsm6dsl_handle();
uint8_t sippy_battery_voltage();
SemaphoreHandle_t *get_spi_mutex();

#endif