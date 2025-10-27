#ifndef SENSORBOARD_BLE_APP_H
#define SENSORBOARD_BLE_APP_H

#include "main.h"

void imu_report_task(void *arg);
void mag_report_task(void *arg);
int device_info_callback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int sensorboard_imu_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int sensorboard_blink_test(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int sensorboard_mag_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

#endif