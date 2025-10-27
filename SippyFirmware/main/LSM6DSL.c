#include "LSM6DSL.h"

// private variables
static spi_device_handle_t *device_handle;
static SemaphoreHandle_t *spi_mutex;

// private functions
static void LSM6DSL_ReadRegisters(LSM6DSL_Register reg, uint8_t *buffer, uint8_t length);
static uint8_t LSM6DSL_ReadRegister(LSM6DSL_Register reg);
static void LSM6DSL_WriteRegister(LSM6DSL_Register reg, uint8_t register_value);

void LSM6DSL_set_handle(spi_device_handle_t *handle, SemaphoreHandle_t *spi_mutex_local)
{
    device_handle = handle;
    spi_mutex = spi_mutex_local;
}

void LSM6DSL_Reset()
{
    LSM6DSL_WriteRegister(LSM6DSL_REG_CTRL3_C, 1 << 0);
    while (LSM6DSL_ReadRegister(LSM6DSL_REG_CTRL3_C) & (1 << 0))
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void display_data(LSM6DSL_Data_Raw *d)
{
    int16_t acc_raw_x = (int16_t)d->accel_x;
    int16_t acc_raw_y = (int16_t)d->accel_y;
    int16_t acc_raw_z = (int16_t)d->accel_z;
    int16_t gyro_raw_x = (int16_t)d->gyro_x;
    int16_t gyro_raw_y = (int16_t)d->gyro_y;
    int16_t gyro_raw_z = (int16_t)d->gyro_z;

    float accel_x = ((float)acc_raw_x / 32768.0f) * 2.0f; // ±2g range
    float accel_y = ((float)acc_raw_y / 32768.0f) * 2.0f;
    float accel_z = ((float)acc_raw_z / 32768.0f) * 2.0f;

    float gyro_x = ((float)gyro_raw_x / 32768.0f) * 250.0f; // ±250 dps range
    float gyro_y = ((float)gyro_raw_y / 32768.0f) * 250.0f;
    float gyro_z = ((float)gyro_raw_z / 32768.0f) * 250.0f;

    ESP_LOGI(__func__, "%fm/s² %fm/s² %fm/s² %f°/s %f°/s %f°/s",
             accel_x, accel_y, accel_z,
             gyro_x, gyro_y, gyro_z);
}

static void raw_reading_to_float(LSM6DSL_Data_Raw *input_data, LSM6DSL_Data *output)
{
    int16_t acc_raw_x = (int16_t)input_data->accel_x;
    int16_t acc_raw_y = (int16_t)input_data->accel_y;
    int16_t acc_raw_z = (int16_t)input_data->accel_z;
    int16_t gyro_raw_x = (int16_t)input_data->gyro_x;
    int16_t gyro_raw_y = (int16_t)input_data->gyro_y;
    int16_t gyro_raw_z = (int16_t)input_data->gyro_z;
    float accel_x = ((float)acc_raw_x / 32768.0f) * 2.0f; // ±2g range
    float accel_y = ((float)acc_raw_y / 32768.0f) * 2.0f;
    float accel_z = ((float)acc_raw_z / 32768.0f) * 2.0f;
    float gyro_x = ((float)gyro_raw_x / 32768.0f) * 250.0f; // ±250 dps range
    float gyro_y = ((float)gyro_raw_y / 32768.0f) * 250.0f;
    float gyro_z = ((float)gyro_raw_z / 32768.0f) * 250.0f;

    output->accel_x = accel_x;
    output->accel_y = accel_y;
    output->accel_z = accel_z;
    output->gyro_x = gyro_x;
    output->gyro_y = gyro_y;
    output->gyro_z = gyro_z;
}

void LSM6DSL_ReadData(LSM6DSL_Data *data_output)
{
    uint8_t buffer[12];
    LSM6DSL_Data_Raw data = ZEROS;
    LSM6DSL_ReadRegisters(LSM6DSL_REG_OUTX_L_G, buffer, 12);

    // Gyro data comes first (little-endian)
    data.gyro_x = (buffer[1] << 8 | buffer[0]);
    data.gyro_y = (buffer[3] << 8 | buffer[2]);
    data.gyro_z = (buffer[5] << 8 | buffer[4]);

    // Accel data comes second
    data.accel_x = (buffer[7] << 8 | buffer[6]);
    data.accel_y = (buffer[9] << 8 | buffer[8]);
    data.accel_z = (buffer[11] << 8 | buffer[10]);

    display_data(&data);
    raw_reading_to_float(&data, data_output);
}


void LSM6DSL_Init()
{
    LSM6DSL_Reset();
    LSM6DSL_WriteRegister(LSM6DSL_REG_CTRL1_XL, (LSM6DSL_ACCEL_ODR_4 << 4) | (LSM6DSL_ACCEL_FS_2G << 2));  // 204Hz, 2G max
    LSM6DSL_WriteRegister(LSM6DSL_REG_CTRL2_G, (LSM6DSL_GYRO_ODR_5 << 4) | (LSM6DSL_GYRO_FS_250DPS << 1)); // 208Hz, 200DPS
}

static void LSM6DSL_WriteRegister(LSM6DSL_Register reg, uint8_t register_value)
{
    int ret;
    uint8_t rxdata[10] = ZEROS;
    uint8_t txdata[10] = ZEROS;
    txdata[0] = (0 << 7) | reg;
    txdata[1] = register_value;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 2); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_LSM6DSL_SS, 0); // setting slave select line low

    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }

    gpio_set_level(PIN_LSM6DSL_SS, 1); // setting slave select line high
}

static uint8_t LSM6DSL_ReadRegister(LSM6DSL_Register reg)
{
    int ret;
    uint8_t rxdata[10] = ZEROS;
    uint8_t txdata[10] = ZEROS;
    txdata[0] = (1 << 7) | reg;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * 2); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_LSM6DSL_SS, 0); // setting slave select line low

    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }

    gpio_set_level(PIN_LSM6DSL_SS, 1); // setting slave select line high

    return rxdata[1];
}

static void LSM6DSL_ReadRegisters(LSM6DSL_Register reg, uint8_t *buffer, uint8_t length)
{
    int ret;
    uint8_t rxdata[50] = ZEROS;
    uint8_t txdata[50] = ZEROS;
    txdata[0] = (1 << 7) | reg;

    // transaction stuff
    spi_transaction_t transaction = ZEROS;
    transaction.tx_buffer = txdata;
    transaction.rx_buffer = rxdata;
    transaction.length = (8 * (length + 1)); // length in bits

    // doing the transaction finally
    gpio_set_level(PIN_LSM6DSL_SS, 0); // setting slave select line low
    if (xSemaphoreTake(*spi_mutex, portMAX_DELAY) == pdTRUE)
    {
        ret = spi_device_polling_transmit(*device_handle, &transaction);
        xSemaphoreGive(*spi_mutex);
    }
    gpio_set_level(PIN_LSM6DSL_SS, 1); // setting slave select line high

    for (uint8_t i = 0; i < length; i++)
    {
        buffer[i] = rxdata[i + 1];
    }
}