#include "RFM95W.h"

// private variables
static spi_device_handle_t *device_handle;
static SemaphoreHandle_t * spi_mutex;

// private functions
static void RFM95W_WriteRegister(RFM95W_LoRaRegister reg, uint8_t register_value);
// static uint8_t RFM95W_ReadRegister(RFM95W_LoRaRegister reg);

void RFM95W_set_handle(spi_device_handle_t *handle, SemaphoreHandle_t * spi_mutex_local)
{
    device_handle = handle;
    spi_mutex = spi_mutex_local;
}

static void RFM95W_WriteRegister(RFM95W_LoRaRegister reg, uint8_t register_value)
{
    int ret;
    uint8_t rxdata[10] = ZEROS;
    uint8_t txdata[10] = ZEROS;
    txdata[0] = (1 << 7) | reg;
    txdata[1] = register_value;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 2); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_RFM95W_SS, 0);    // setting slave select line low
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    if(xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    gpio_set_level(PIN_RFM95W_SS, 1);    // setting slave select line high
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
}

void RFM95W_WriteRegisters(uint8_t *register_value, uint8_t length)
{
    int ret;
    uint8_t rxdata[50] = ZEROS;
    uint8_t txdata[50] = ZEROS;
    // copying bytes
    for (uint8_t i = 0; i < length; i++)
    {
        txdata[i] = register_value[i];
    }

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * (length + 1)); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_RFM95W_SS, 0);    // setting slave select line low
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    if(xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    gpio_set_level(PIN_RFM95W_SS, 1);    // setting slave select line high
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
}

uint8_t RFM95W_ReadRegister(RFM95W_LoRaRegister reg)
{
    int ret;
    uint8_t txdata[10] = ZEROS;
    uint8_t rxdata[10] = ZEROS;
    txdata[0] = (0 << 7) | reg;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 2); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_RFM95W_SS, 0);    // setting slave select line low
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    if(xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    gpio_set_level(PIN_RFM95W_SS, 1);    // setting slave select line high
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms

    return rxdata[1];
}

void RFM95W_ReadRegisters(RFM95W_LoRaRegister reg, uint8_t *buffer, uint8_t bytes_to_read)
{
    int ret;
    uint8_t txdata[270] = ZEROS;
    uint8_t rxdata[270] = ZEROS;
    txdata[0] = (0 << 7) | reg;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * (1 + bytes_to_read)); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_RFM95W_SS, 0);    // setting slave select line low
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    if(xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms
    gpio_set_level(PIN_RFM95W_SS, 1);    // setting slave select line high
    vTaskDelay(10 / portTICK_PERIOD_MS); // waiting 10ms

    memcpy(buffer, &rxdata[1], bytes_to_read);
}

void RFM95W_Reset()
{
    gpio_set_level(PIN_RFM95W_NRST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS); // waiting 100ms
    gpio_set_level(PIN_RFM95W_NRST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS); // waiting 100ms
}

void RFM95W_Init()
{
    RFM95W_Reset();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // setting register
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1, 0x80}, 2);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // setting frequency
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x6, 217, 0, 0}, 4);
    // setting output power limited upto 20dBm, ramp up and down times in FSK as 40us
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x9, 0xff, 0b1001}, 3);
    // setting LNA gain as max, default current
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0xc, 0b00100000}, 2);
    // setting signal bandwidth as 62.5KHz and error coding rate as 4/8, and explicit header mode
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1d, 0b0110 << 4 | 0b100 << 1 | 0b0}, 2);
    // setting spread factor as 12, packet mode
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1e, (0b1100 << 4) | (1 << 2)}, 2);
    // setting preamble length as 50
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x20, 0, 50}, 3);
    // setting in mobile node
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x26, 1 << 3 | 1 << 2}, 2);
    RFM95W_WriteRegisters((uint8_t[]){0x80 | 0x24, 0x00}, 2);

    // setting all FIFO as 0x00
    RFM95W_SetFifoPtr(0x00);
    RFM95W_SetRxPtr(0x00);
    RFM95W_SetTxPtr(0x80);

    // setting DIO to pulse at RxDone
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x40, 0b00}, 2);

    // standby mode
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1, (1 << 7) | 1}, 2);
}

void RFM95W_StandbyMode()
{
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1, (1 << 7) | 1}, 2);
}

void RFM95W_RxMode()
{
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x1, (1 << 7) | 0b101}, 2);
}

void print_spare_info()
{
    ESP_LOGI(__func__, "%02X %02X %02X %02X %02X %02X%02X%02X ", RFM95W_ReadRegister(0x12),
             RFM95W_ReadRegister(0x18),
             RFM95W_ReadRegister(0x12),
             RFM95W_ReadRegister(0x1B),
             RFM95W_ReadRegister(0x1C),
             RFM95W_ReadRegister(0x28),
             RFM95W_ReadRegister(0x29),
             RFM95W_ReadRegister(0x2A));
}

void RFM95W_WaitUntilRxDone()
{
    do
    {
        print_spare_info();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } while (!(RFM95W_ReadRegister(RFM95W_REG_IRQ_FLAGS) & (1 << 6)));
}

void RFM95W_WaitUntilRxDone_Timeout()
{
    uint8_t c = 0;
    do
    {
        print_spare_info();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        c++;
        if (c >= 10)
            break;
    } while (!(RFM95W_ReadRegister(RFM95W_REG_IRQ_FLAGS) & (1 << 6)));
}

void RFM95W_WaitUntilTxDone()
{
    do
    {
        print_spare_info();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } while (!(RFM95W_ReadRegister(RFM95W_REG_IRQ_FLAGS) & (1 << 3)));
}

void RFM95W_SetTxPtr(uint8_t ptr)
{
    RFM95W_WriteRegister(RFM95W_REG_FIFO_TX_BASE_ADDR, ptr);
}

void RFM95W_SetRxPtr(uint8_t ptr)
{
    RFM95W_WriteRegister(RFM95W_REG_FIFO_RX_BASE_ADDR, ptr);
}

void RFM95W_SetFifoPtr(uint8_t ptr)
{
    RFM95W_WriteRegister(RFM95W_REG_FIFO_ADDR_PTR, ptr);
}

void RFM95W_ClearAllInterrupts()
{
    RFM95W_WriteRegisters((uint8_t[]){RFM95W_REG_IRQ_FLAGS | 1 << 7, 0xff}, 2);
}

void RFM95W_Transmit(uint8_t *data, uint8_t length)
{
    uint8_t data_buffer[270] = ZEROS;
    data_buffer[0] = (1 << 7) | 0x0;
    for (uint8_t i = 0; i < length; i++)
    {
        data_buffer[1 + i] = data[i];
    }
    // reseting data pointer to 0x80
    RFM95W_SetTxPtr(0x80);
    RFM95W_SetFifoPtr(0x80);
    RFM95W_WriteRegisters(data_buffer, length + 1);
    RFM95W_WriteRegisters((uint8_t[]){(1 << 7) | 0x22, length}, 2);
    RFM95W_WriteRegister(RFM95W_REG_OPMODE, (1 << 7) | (0b011));
}

void RFM95W_Set_LongRangeMode(RFM95W_LongRangeMode mode)
{
    RFM95W_WriteRegister(RFM95W_REG_OPMODE, (RFM95W_ReadRegister(RFM95W_REG_OPMODE) & 0b01111111) | (mode << 7));
}

void RFM95W_Set_AccessShare(RFM95W_AccessShare share)
{
    RFM95W_WriteRegister(RFM95W_REG_OPMODE, (RFM95W_ReadRegister(RFM95W_REG_OPMODE) & 0b10111111) | (share << 6));
}

void RFM95W_Set_Mode(RFM95W_Mode mode)
{
    RFM95W_WriteRegister(RFM95W_REG_OPMODE, (RFM95W_ReadRegister(RFM95W_REG_OPMODE) & 0b11111100) | (mode));
}

void RFM95W_Set_FrequencyRegisters(uint32_t freq_register)
{
    uint8_t msb = (freq_register >> 16) & 0xff;
    uint8_t midsb = (freq_register >> 8) & 0xff;
    uint8_t lsb = (freq_register) & 0xff;
    RFM95W_WriteRegister(RFM95W_REG_FR_MSB, msb);
    RFM95W_WriteRegister(RFM95W_REG_FR_MID, midsb);
    RFM95W_WriteRegister(RFM95W_REG_FR_LSB, lsb);
}
