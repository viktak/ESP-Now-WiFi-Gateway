#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <rom/rtc.h>

#include <WiFi.h>
#include <rom/rtc.h>

#include "settings.h"

#define __debugSettings
#define SerialMon Serial

#define MQTT_CUSTOMER "viktak"
#define MQTT_PROJECT "spiti"

#define INTERNET_SERVER_NAME "diy.viktak.com"
#define ESP_ACCESS_POINT_NAME_SIZE 63

static const int32_t DEBUG_SPEED = 921600;
static const String HARDWARE_ID = "ESP32-dev";
static const String HARDWARE_VERSION = "1.0";
static const String FIRMWARE_ID = "ESP-Now Sensor Bridge";

extern char *stack_start;

void SetRandomSeed();
void DateTimeToString(char *dest);
String TimeIntervalToString(const time_t time);
String GetDeviceMAC();
uint32_t GetChipID();
int8_t GetPrintableCardID(char *dest, const char *source);
int8_t GetPrintableCardUID(char *dest, const char *source);
String GetFirmwareVersionString();
const char *GetOperationModeString();
boolean checkInternetConnection();

const char *GetResetReasonString(RESET_REASON reason);

uint32_t stack_size();

#endif