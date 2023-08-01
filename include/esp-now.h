#ifndef ESP_NOW_H
#define ESP_NOW_H

#include <Arduino.h>
#include <esp_now.h>

#include <DallasTemperature.h>

typedef struct struct_message
{
    uint64_t ordinal;
    uint32_t nodeID;
    DeviceAddress deviceAddress;
    float sensorValue;
    uint64_t failuresCount;
    float batteryLevel;

} struct_message;

void initEspNow();

#endif