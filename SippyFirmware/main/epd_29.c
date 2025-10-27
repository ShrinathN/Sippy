#include "epd_29.h"

// variables
static spi_device_handle_t *device_handle;
static SemaphoreHandle_t *spi_mutex;
// static const unsigned char lut_full_update[] = {
//     0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const unsigned char lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// private functions
static void epd29_SpiTransfer(unsigned char data)
{
    // transaction stuff
    int ret;
    uint8_t txdata[50] = {data};
    uint8_t rxdata[50] = ZEROS;
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 1); // length in bits

    gpio_set_level(PIN_EPD_CS, 0);
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_EPD_CS, 1);
}

static void epd29_Reset()
{
    gpio_set_level(PIN_EPD_RST, 0); // module epd29_Reset
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_EPD_RST, 1);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

static void epd29_SendCommand(unsigned char command)
{
    gpio_set_level(PIN_EPD_DC, 0);
    epd29_SpiTransfer(command);
}

static void epd29_SendData(unsigned char data)
{
    gpio_set_level(PIN_EPD_DC, 1);
    epd29_SpiTransfer(data);
}

static void epd29_SetLut(const unsigned char *lut)
{
    epd29_SendCommand(WRITE_LUT_REGISTER);
    /* the length of look-up table is 30 bytes */
    for (int i = 0; i < 30; i++)
    {
        epd29_SendData(lut[i]);
    }
}

static void epd29_WaitUntilIdle(void)
{
    uint8_t retry = 0;
    do
    { // LOW: idle, HIGH: busy
        vTaskDelay(20 / portTICK_PERIOD_MS);
    } while ((gpio_get_level(PIN_EPD_BUSY) == 1) && (retry++ < 100));
}

static void epd29_SetMemoryArea(int x_start, int y_start, int x_end, int y_end)
{
    epd29_SendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    epd29_SendData((x_start >> 3) & 0xFF);
    epd29_SendData((x_end >> 3) & 0xFF);
    epd29_SendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    epd29_SendData(y_start & 0xFF);
    epd29_SendData((y_start >> 8) & 0xFF);
    epd29_SendData(y_end & 0xFF);
    epd29_SendData((y_end >> 8) & 0xFF);
}

static void epd29_SetMemoryPointer(int x, int y)
{
    epd29_SendCommand(SET_RAM_X_ADDRESS_COUNTER);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    epd29_SendData((x >> 3) & 0xFF);
    epd29_SendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    epd29_SendData(y & 0xFF);
    epd29_SendData((y >> 8) & 0xFF);
    epd29_WaitUntilIdle();
}

static void epd29_SetFrameMemory(const unsigned char *image_buffer)
{
    epd29_SetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    epd29_SetMemoryPointer(0, 0);
    epd29_SendCommand(WRITE_RAM);
    /* send the image data */
    for (int i = 0; i < EPD_WIDTH / 8 * EPD_HEIGHT; i++)
    {
        epd29_SendData(image_buffer[i]);
    }
}

void epd29_ClearFrameMemory(unsigned char color)
{
    epd29_SetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    epd29_SetMemoryPointer(0, 0);
    epd29_SendCommand(WRITE_RAM);
    /* send the color data */
    for (int i = 0; i < EPD_WIDTH / 8 * EPD_HEIGHT; i++)
    {
        epd29_SendData(color);
    }
}

void epd29_DisplayFrame(void)
{
    epd29_SendCommand(DISPLAY_UPDATE_CONTROL_2);
    epd29_SendData(0xC4);
    epd29_SendCommand(MASTER_ACTIVATION);
    epd29_SendCommand(TERMINATE_FRAME_READ_WRITE);
    epd29_WaitUntilIdle();
}

static void epd29_Sleep()
{
    epd29_SendCommand(DEEP_SLEEP_MODE);
    epd29_SendData(0x01);
    // epd29_WaitUntilIdle();
}

void epd29_init()
{
    epd29_Reset();
    epd29_SendCommand(DRIVER_OUTPUT_CONTROL);
    epd29_SendData((EPD_HEIGHT - 1) & 0xFF);
    epd29_SendData(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epd29_SendData(0x00); // GD = 0; SM = 0; TB = 0;
    epd29_SendCommand(BOOSTER_SOFT_START_CONTROL);
    epd29_SendData(0xD7);
    epd29_SendData(0xD6);
    epd29_SendData(0x9D);
    epd29_SendCommand(WRITE_VCOM_REGISTER);
    epd29_SendData(0xA8); // VCOM 7C
    epd29_SendCommand(SET_DUMMY_LINE_PERIOD);
    epd29_SendData(0x1A); // 4 dummy lines per gate
    epd29_SendCommand(SET_GATE_TIME);
    epd29_SendData(0x08); // 2us per line
    epd29_SendCommand(DATA_ENTRY_MODE_SETTING);
    epd29_SendData(0x03); // X increment; Y increment
    epd29_SetLut(lut_partial_update);
}

void epd29_set_handle(spi_device_handle_t *handle, SemaphoreHandle_t *spi_mutex_local)
{
    device_handle = handle;
    spi_mutex = spi_mutex_local;
}