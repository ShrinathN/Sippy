#ifndef WS2812B_H
#define WS2812B_H
#include "main.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM PIN_LED_DOUT


void ws2812b_write(uint8_t r, uint8_t g, uint8_t b);
void ws2812b_init();

#endif