#include "MLX90393.h"

// private variables
static spi_device_handle_t *device_handle;
static SemaphoreHandle_t *spi_mutex;

// private functions
static void MLX90393_Set_Bits_Register(MLX90393_Register addr, uint16_t mask, uint16_t data);
static void MLX90393_Write_Register(MLX90393_Register addr, uint16_t data);
static uint16_t MLX90393_Read_Register(MLX90393_Register reg);
static void MLX90393_Set_Hallconf(MLX90393_Hallconf hcfg);
static void MLX90393_Set_Gain(MLX90393_Gain gain);
static void MLX90393_Set_Z_Axis_Measurement(MLX90393_Z_Measurement ax);
static void MLX90393_Set_Burst_Data_Rate(MLX90393_Burst_Data_Rate bdr);
static void MLX90393_Set_Burst_Sel(uint8_t sel);
static void MLX90393_Set_Temperature_Compensation(uint8_t comp);
static void MLX90393_Set_OSR(uint8_t osr);
static void MLX90393_Set_Dig_Filt(uint8_t dig);
static void MLX90393_Set_Res_XYZ(uint8_t xyz);
static void MLX90393_Set_Config();
static void MLX90393_Reset();
static void MLX90393_Send_Command(MLX90393_Command com);

// function definition

static void MLX90393_Set_Bits_Register(MLX90393_Register addr, uint16_t mask, uint16_t data)
{
    uint16_t reg_val = MLX90393_Read_Register(addr);
    reg_val &= ~(mask);
    reg_val |= data;
    MLX90393_Write_Register(addr, reg_val);
}

static void MLX90393_Write_Register(MLX90393_Register addr, uint16_t data)
{
    int ret;
    uint8_t rxdata[10] = ZEROS;
    uint8_t txdata[10] = ZEROS;
    txdata[0] = MLX90393_COMMAND_WRITE_REGISTER;
    txdata[1] = (data & 0xFF00) >> 8;
    txdata[2] = (data & 0x00FF);
    txdata[3] = addr << 2;
    txdata[4] = 0x00;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 5); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_MLX90393ELW_SS, 0); // setting slave select line low
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_MLX90393ELW_SS, 1); // setting slave select line high
}

static uint16_t MLX90393_Read_Register(MLX90393_Register reg)
{
    int ret;
    uint8_t txdata[10] = ZEROS;
    uint8_t rxdata[10] = ZEROS;
    txdata[0] = MLX90393_COMMAND_READ_REGISTER;
    txdata[1] = reg << 2;
    txdata[2] = 0x00;
    txdata[3] = 0x00;
    txdata[4] = 0x00;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 5); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_MLX90393ELW_SS, 0); // setting slave select line low
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_MLX90393ELW_SS, 1); // setting slave select line high

    ESP_LOGI(__func__, "Rx Data = %02X %02X %02X %02X %02X", rxdata[0], rxdata[1], rxdata[2], rxdata[3], rxdata[4]);

    return (rxdata[3] << 8) | rxdata[4];
}

void MLX90393_set_handle(spi_device_handle_t *handle, SemaphoreHandle_t *spi_mutex_local)
{
    device_handle = handle;
    spi_mutex = spi_mutex_local;
}

void MLX90393_Init()
{
    MLX90393_Reset();
    MLX90393_Set_Config();
}

static void MLX90393_Set_Hallconf(MLX90393_Hallconf hcfg)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x00, (0b1111 << 0), (hcfg));
}

static void MLX90393_Set_Gain(MLX90393_Gain gain)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x00, (0b111 << 4), (gain << 4));
}

static void MLX90393_Set_Z_Axis_Measurement(MLX90393_Z_Measurement ax)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x00, (0b1 << 7), (ax << 7));
}

static void MLX90393_Set_Burst_Data_Rate(MLX90393_Burst_Data_Rate bdr)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x01, (0b111111 << 0), (bdr << 0));
}

static void MLX90393_Set_Burst_Sel(uint8_t sel)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x01, (0b1111 << 7), (sel << 7));
}

static void MLX90393_Set_Temperature_Compensation(uint8_t comp)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x01, (0b1 << 10), (comp << 10));
}

static void MLX90393_Set_OSR(uint8_t osr)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x02, (0b11 << 0), (osr << 0));
}

static void MLX90393_Set_Dig_Filt(uint8_t dig)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x02, (0b111 << 2), (dig << 2));
}

static void MLX90393_Set_Res_XYZ(uint8_t xyz)
{
    MLX90393_Set_Bits_Register(MLX90393_REGISTER_0x02, (0b111111 << 5), (xyz << 5));
}

static void MLX90393_Set_Config()
{
    MLX90393_Set_Hallconf(MLX90393_HALLCONF_12);
    MLX90393_Set_Gain(MLX90393_GAIN_7);
    MLX90393_Set_Z_Axis_Measurement(MLX90393_Z_AXIS_ENABLE);
    MLX90393_Set_Burst_Data_Rate(MLX90393_BURST_DATA_RATE_40MS);
    MLX90393_Set_Burst_Sel(0b1111);
    MLX90393_Set_Temperature_Compensation(1);
    MLX90393_Set_Dig_Filt(3);
    MLX90393_Set_Res_XYZ(0b010101);
    MLX90393_Send_Command(MLX90393_COMMAND_START_BURST_MODE);
}

void MLX90393_Read_Measurements(MLX90393_Data *output)
{
    int ret;
    uint8_t txdata[15] = ZEROS;
    uint8_t rxdata[15] = ZEROS;
    MLX90393_Data_Raw raw_data = ZEROS;
    txdata[0] = MLX90393_COMMAND_READ_MEASUREMENT | 0x0f;
    txdata[1] = 0x00;
    txdata[2] = 0x00;
    txdata[3] = 0x00;
    txdata[4] = 0x00;
    txdata[5] = 0x00;
    txdata[6] = 0x00;
    txdata[7] = 0x00;
    txdata[8] = 0x00;
    txdata[9] = 0x00;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 10); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_MLX90393ELW_SS, 0); // setting slave select line low
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_MLX90393ELW_SS, 1); // setting slave select line high

    // putting data
    raw_data.status = rxdata[1];
    raw_data.temperature = (rxdata[2] << 8) | rxdata[3];
    raw_data.x = (rxdata[4] << 8) | rxdata[5];
    raw_data.y = (rxdata[6] << 8) | rxdata[7];
    raw_data.z = (rxdata[8] << 8) | rxdata[9];

    // putting in actual output struct
    output->status = raw_data.status;
    output->temperature = ((float)raw_data.temperature * 0.0977f) - 75.0f;
    output->x = (float)raw_data.x * 0.3; //uT
    output->y = (float)raw_data.y * 0.3; //uT
    output->z = (float)raw_data.z * 0.484; //uT
}

static void MLX90393_Reset()
{
    MLX90393_Send_Command(MLX90393_COMMAND_EXIT_MODE);
    MLX90393_Send_Command(MLX90393_COMMAND_RESET);
}

static void MLX90393_Send_Command(MLX90393_Command com)
{
    int ret;
    uint8_t txdata[10] = ZEROS;
    uint8_t rxdata[10] = ZEROS;
    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;

    if (com == MLX90393_COMMAND_EXIT_MODE)
    {
        txdata[0] = MLX90393_COMMAND_EXIT_MODE;
        txdata[1] = 0;
        transaction.length = (8 * 2); // length in bits
    }

    if (com == MLX90393_COMMAND_RESET)
    {
        txdata[0] = MLX90393_COMMAND_RESET;
        transaction.length = (8 * 1); // length in bits
    }

    if (com == MLX90393_COMMAND_START_BURST_MODE)
    {
        txdata[0] = MLX90393_COMMAND_START_BURST_MODE;
        transaction.length = (8 * 1); // length in bits
    }

    // making the transaction
    gpio_set_level(PIN_MLX90393ELW_SS, 0); // setting slave select line low
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_MLX90393ELW_SS, 1); // setting slave select line high

    ESP_LOGI("WEW", "RX[0] = %02X RX[1] = %02X\n", rxdata[0], rxdata[1]);

    assert(ret == ESP_OK);
}
