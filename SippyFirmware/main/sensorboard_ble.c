#include "sensorboard_ble.h"
#include "sensorboard_ble_app.h"

const char *device_name = "SensorBoard";

// UUIDs
const ble_uuid16_t SENSORBOARD_SVC = BLE_UUID16_INIT(UUID_SENSORBOARD_SVC);
const ble_uuid16_t SENSORBOARD_CHR_IMU = BLE_UUID16_INIT(UUID_SENSORBOARD_CHR_IMU);
const ble_uuid16_t SENSORBOARD_CHR_BLINK = BLE_UUID16_INIT(UUID_SENSORBOARD_CHR_BLINK);
const ble_uuid16_t SENSORBOARD_CHR_MAGNETOMETER = BLE_UUID16_INIT(UUID_SENSORBOARD_CHR_MAGNETOMETER);
const ble_uuid16_t BATTERY_SERVICE_UUID = BLE_UUID16_INIT(UUID_BATTERY_SERVICE_UUID);
const ble_uuid16_t BATTERY_LEVEL_UUID = BLE_UUID16_INIT(UUID_BATTERY_LEVEL_UUID);

// variables
uint8_t blehr_addr_type;
uint16_t conn_handle;
// handlers
uint16_t imu_task_handle;
uint16_t mag_task_handle;

// private functions
static void sensorboard_sync_func(void);
static void sensorboard_reset_func(int reason);
static int sensorboard_gap_handler(struct ble_gap_event *event, void *arg);
static void sensorboard_advertise(void);

// GATT service
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // sensorboard main service
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = (const ble_uuid_t *)&SENSORBOARD_SVC,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                // can subscribe to this IMU sampling notification
                .uuid = (const ble_uuid_t *)&SENSORBOARD_CHR_IMU,
                .access_cb = sensorboard_imu_read,
                .val_handle = &imu_task_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
            },
            {
                // just blinks the LED
                .uuid = (const ble_uuid_t *)&SENSORBOARD_CHR_BLINK,
                .access_cb = sensorboard_blink_test,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                // reads magnetometer readings
                .uuid = (const ble_uuid_t *)&SENSORBOARD_CHR_MAGNETOMETER,
                .access_cb = sensorboard_mag_read,
                .val_handle = &mag_task_handle,
                .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,
            },
            {
                0, /* No more characteristics in this service */
            },
        }},
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, // primary
     .uuid = (const ble_uuid_t *)&BATTERY_SERVICE_UUID,
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             .uuid = (const ble_uuid_t *)&BATTERY_LEVEL_UUID,
             .access_cb = device_info_callback,
             .flags = BLE_GATT_CHR_F_READ,
         },
         {
             0,
         },
     }},
    {
        0, /* No more services */
    },
};

static int sensorboard_gatt_service_init()
{

    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}

static int sensorboard_gap_handler(struct ble_gap_event *event, void *arg)
{
    const char *TAG = __func__;
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed */
        ESP_LOGI(TAG, "connection %s; status=%d\n", event->connect.status == 0 ? "established" : "failed", event->connect.status);

        if (event->connect.status != 0)
        {
            /* Connection failed; resume advertising */
            sensorboard_advertise();
            conn_handle = 0;
        }
        else
        {
            conn_handle = event->connect.conn_handle;
        }

        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnect; reason=%d\n", event->disconnect.reason);
        conn_handle = BLE_HS_CONN_HANDLE_NONE; /* reset conn_handle */

        /* Connection terminated; resume advertising */
        sensorboard_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "adv complete\n");
        sensorboard_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "Something subscribed! REASON->%d NOT->%d CONNHAN->%d Att->%d Ind->%d", event->subscribe.reason, event->subscribe.cur_notify, event->subscribe.conn_handle, event->subscribe.attr_handle, event->subscribe.cur_indicate);
        if (event->subscribe.attr_handle == imu_task_handle)
        {
            uint8_t current_notify_state = event->subscribe.cur_notify;
            static TaskHandle_t imu_task_handle = NULL;
            if (current_notify_state)
            {
                xTaskCreate(imu_report_task, "imu_report_task", 4096, NULL, tskIDLE_PRIORITY, &imu_task_handle);
            }
            else
            {
                vTaskDelete(imu_task_handle);
            }
        }
        else if (event->subscribe.attr_handle == mag_task_handle)
        {
            uint8_t current_notify_state = event->subscribe.cur_notify;
            static TaskHandle_t mag_task_handle = NULL;
            if (current_notify_state)
            {
                xTaskCreate(mag_report_task, "mag_report_task", 4096, NULL, tskIDLE_PRIORITY, &mag_task_handle);
            }
            else
            {
                vTaskDelete(mag_task_handle);
            }
        }
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d mtu=%d\n",
                 event->mtu.conn_handle,
                 event->mtu.value);
        break;
    }

    return 0;
}

static void sensorboard_advertise()
{
    const char *TAG = __func__;
    uint8_t buff[] = "\0\0WTF!!\0";
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    fields.appearance_is_present = 1;
    fields.appearance = 0x0540;
    // fields.mfg_data = buff;
    // fields.mfg_data_len = 9;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGI(TAG, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(blehr_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, sensorboard_gap_handler, NULL);
    if (rc != 0)
    {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

static void sensorboard_sync_func(void)
{
    const char *TAG = __func__;
    ESP_LOGI(TAG, "Handler called!");

    int rc;
    rc = ble_hs_id_infer_auto(0, &blehr_addr_type);
    assert(rc == 0);

    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(blehr_addr_type, addr_val, NULL);

    // ESP_LOGI(TAG, "Device Address: %02X:%02X:%02X:%02X:%02X", addr_val[5], addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);
    ESP_LOGI(TAG, "\n");

    /* Begin advertising */
    sensorboard_advertise();
}

static void sensorboard_reset_func(int reason)
{
    const char *TAG = __func__;
    ESP_LOGI(TAG, "Handler called!");
}

static void sensorboard_nimble_task(void *param)
{
    nimble_port_run();

    nimble_port_freertos_deinit();
}

void sensorboard_ble_start()
{
    const char *TAG = __func__;
    esp_err_t ret;
    int rc;
    ret = nimble_port_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to init nimble %d \n", ret);
        return;
    }

    // setting the callback functions
    ble_hs_cfg.sync_cb = sensorboard_sync_func;
    ble_hs_cfg.reset_cb = sensorboard_reset_func;

    rc = sensorboard_gatt_service_init();
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);

    /* Start the task */
    nimble_port_freertos_init(sensorboard_nimble_task);
}