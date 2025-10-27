#ifndef SENSORBOARD_BLE_H
#define SENSORBOARD_BLE_H

#include "main.h"

//UUIDs
#define UUID_SENSORBOARD_SVC 0x517c
#define UUID_SENSORBOARD_CHR_IMU 0x3dbd
#define UUID_SENSORBOARD_CHR_BLINK 0xb7e5
#define UUID_SENSORBOARD_CHR_MAGNETOMETER 0x5793
#define UUID_BATTERY_SERVICE_UUID 0x180F
#define UUID_BATTERY_LEVEL_UUID 0x2A19

//public functions
void sensorboard_ble_start();

#endif