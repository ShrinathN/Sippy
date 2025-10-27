#include "sensorboard_ble_app.h"
#include "sensorboard_init.h"
#include "LSM6DSL.h"
#include "MLX90393.h"

// variables
extern uint16_t conn_handle;
extern uint16_t imu_task_handle;
extern uint16_t mag_task_handle;
extern volatile uint8_t b;

extern ble_uuid_t SENSORBOARD_SVC;
extern ble_uuid_t SENSORBOARD_CHR_IMU;
extern ble_uuid_t SENSORBOARD_CHR_BLINK;
extern ble_uuid_t SENSORBOARD_CHR_MAGNETOMETER;
extern ble_uuid_t BATTERY_SERVICE_UUID;
extern ble_uuid_t BATTERY_LEVEL_UUID;

int device_info_callback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = 0;

    if (ble_uuid_cmp(ctxt->chr->uuid, &BATTERY_LEVEL_UUID) == 0)
    {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            uint8_t battery_level = sensorboard_battery_voltage();
            rc = os_mbuf_append(ctxt->om, &battery_level, sizeof(battery_level));
        }

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int sensorboard_mag_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = 0;

    if (ble_uuid_cmp(ctxt->chr->uuid, &SENSORBOARD_CHR_MAGNETOMETER) == 0)
    {
        // central wants to read
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            MLX90393_Data data = ZEROS;
            MLX90393_Read_Measurements(&data);
            rc = os_mbuf_append(ctxt->om, &data, sizeof(MLX90393_Data));
        }

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int sensorboard_imu_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = 0;

    if (ble_uuid_cmp(ctxt->chr->uuid, &SENSORBOARD_CHR_IMU) == 0)
    {
        // central wants to read
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
        {
            LSM6DSL_Data data = ZEROS;
            LSM6DSL_ReadData(&data);
            rc = os_mbuf_append(ctxt->om, &data, sizeof(LSM6DSL_Data));
        }

        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int sensorboard_blink_test(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint16_t uuid;
    int rc = 0;

    if (ble_uuid_cmp(ctxt->chr->uuid, &SENSORBOARD_CHR_BLINK) == 0)
    {
        // central wants to read
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
        {
            b = 1;
        }
        return 0;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

void imu_report_task(void *arg)
{
    const char *TAG = __func__;
    int rc;
    struct os_mbuf *om;
    while (1)
    {
        LSM6DSL_Data data = ZEROS;
        LSM6DSL_ReadData(&data);
        om = ble_hs_mbuf_from_flat(&data, sizeof(data));
        rc = ble_gatts_notify_custom(conn_handle, imu_task_handle, om);
        if (rc != ESP_OK)
        {
            ESP_LOGI(TAG, "Notify error %d", rc);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void mag_report_task(void *arg)
{
    const char *TAG = __func__;
    int rc;
    struct os_mbuf *om;
    while (1)
    {
        MLX90393_Data data = ZEROS;
        MLX90393_Read_Measurements(&data);
        om = ble_hs_mbuf_from_flat(&data, sizeof(data));
        rc = ble_gatts_notify_custom(conn_handle, mag_task_handle, om);
        if (rc != ESP_OK)
        {
            ESP_LOGI(TAG, "Notify error %d", rc);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}