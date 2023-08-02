#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>

#include "esp-now.h"
#include "main.h"

void SendHeartbeat();

// void SendMeasurement(const struct_message myData);
void SendMeasurement(const uint64_t ordinal, const uint32_t nodeID, const DeviceAddress deviceAddress, const float sensorValue, const uint64_t failuresCount, const float batteryLevel);
void InitMQTT();

void MQTTLoop();

#endif