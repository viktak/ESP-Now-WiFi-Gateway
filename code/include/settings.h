#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

////////////////////////////////////////////////////////////////////

enum OPERATION_MODES
{
    WIFI_SETUP,
    NORMAL
};

////////////////////////////////////////////////////////////////////

//  System
extern u_char FailedBootAttempts;

extern char adminPassword[32];

extern char ssid[32];
extern char password[32];

extern char AccessPointSSID[32];
extern char AccessPointPassword[32];

extern char friendlyName[32];

extern signed char timeZone;

extern u_int heartbeatInterval;

//  Wifi


//  MQTT
extern char mqttServer[64];
extern uint16_t mqttPort;
extern char mqttTopic[64];
extern char mqttUserName[64];
extern char mqttPassword[64];

//  Operation
extern OPERATION_MODES opMode;

//  Calculated settings
extern char hostName[32];
extern char mqttRootTopic[32];
extern char mqttStateTopic[128];
extern char mqttCommandTopic[128];
extern char mqttHeartbeatTopic[128];
extern char mqttTelemetryTopic[128];

//  functions

void PrintSettings();
void LoadSettings(bool LoadDefaults = false);
void SaveSettings();

void ClearNVS();
void InitSettings();

#endif