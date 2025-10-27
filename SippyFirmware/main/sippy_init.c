#include "sippy_init.h"

// handlers
static spi_device_handle_t lsm6dsl_device;
static adc_oneshot_unit_handle_t adc1_handle;
static SemaphoreHandle_t spi_mutex;

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated)
    {
        ESP_LOGI(__func__, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(__func__, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(__func__, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(__func__, "Invalid arg or no memory");
    }

    return calibrated;
}

void sippy_gpio_init()
{
    const char *TAG = __func__;
    // outputs
    gpio_config_t gp = ZEROS;
    gp.intr_type = GPIO_INTR_DISABLE;
    gp.mode = GPIO_MODE_OUTPUT;
    gp.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gp.pull_up_en = GPIO_PULLUP_DISABLE;
 
 

    // LSM6DSL chip select
    gp.pin_bit_mask = BIT64(PIN_LSM6DSL_SS);
    gpio_config(&gp);
    gpio_set_level(PIN_LSM6DSL_SS, 1); // setting high

    //buzzer
    gp.pin_bit_mask = BIT64(PIN_BUZZER);
    gpio_config(&gp);
    gpio_set_level(PIN_BUZZER, 1); // setting high

    // //WS2812B LED Buffer
    // gp.pin_bit_mask = BIT64(PIN_LED_DOUT);
    // gpio_config(&gp);
    // gpio_set_level(PIN_LED_DOUT, 0); // setting low

    //accel interrupt
    gp.pin_bit_mask = BIT64(PIN_LSM6DSL_INT1);
    gp.mode = GPIO_MODE_INPUT;
    gpio_config(&gp);


    // ADC stuff
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL_0, ADC_ATTEN_DB_0, &adc1_cali_chan0_handle);
    static int adc_raw[2][10];
    static int voltage[2][10];
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw[0][0]));
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHANNEL_0, adc_raw[0][0]);
    if (do_calibration1_chan0)
    {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC_CHANNEL_0, voltage[0][0]);
    }
}

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t sippy_battery_voltage()
{
    const char *TAG = __func__;
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw));
    float raw_ratio = ((adc_raw / 4096.0f) * 2.0f);
    float voltage = (raw_ratio * 4.0f);
    voltage = (voltage > 4.0f) ? 4.0f : voltage;
    float percentage = map(voltage, 3.0f, 4.0f, 0.0f, 100.0f);
    ESP_LOGI(TAG, "Raw ADC reading=%d\traw_ratio=%f\tpercentage=%f", adc_raw, raw_ratio, percentage);
    return (uint8_t)percentage;
}

void sippy_spi_init()
{
    spi_mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(spi_mutex);
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_SPI_MISO,
        .mosi_io_num = PIN_SPI_MOSI,
        .sclk_io_num = PIN_SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 128,
    };

    spi_device_interface_config_t lsm6dsl_interface_config = {
        .clock_speed_hz = 1 * 1000 * 1000, // Clock out at 4MHz
        .mode = 3,                         // SPI mode 3
        .spics_io_num = -1,                // no CS pin
        .queue_size = 8,                   // We want to be able to queue 7 transactions at a time

    };

    // setting up magnetometer
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(SPI2_HOST, &lsm6dsl_interface_config, &lsm6dsl_device);
    ESP_ERROR_CHECK(ret);
}

spi_device_handle_t *get_lsm6dsl_handle()
{
    return &lsm6dsl_device;
}

SemaphoreHandle_t *get_spi_mutex()
{
    return &spi_mutex;
}