#include "settings.h"

#include <SPIFFS.h>

#include "common.h"

#define SETTINGS_NAME "now-bridge"

////////////////////////////////////////////////////////////////////
/// Deafult values
////////////////////////////////////////////////////////////////////

//  System
#define DEFAULT_ADMIN_USERNAME "admin"
#define DEFAULT_ADMIN_PASSWORD "admin"
#define DEFAULT_APP_FRIENDLY_NAME "ESP-Now Sensor Bridge"
#define DEFAULT_AP_PASSWORD "esp12345678"

#define DEFAULT_TIMEZONE 13

#define DEFAULT_HEARTBEAT_INTERVAL 300 //  seconds

//  WiFi
#define DEFAULT_SSID "ESP"
#define DEFAULT_PASSWORD "ESP"

//  MQTT
#define DEFAULT_MQTT_SERVER "192.168.1.99"
#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_TOPIC "esp-now-bridge"
#define DEFAULT_MQTT_USERNAME "user"
#define DEFAULT_MQTT_PASSWORD "password"

//  Operation
#define DEFAULT_OPERATION_MODE 0

Preferences prefs;

//  System
u_char FailedBootAttempts;

char adminPassword[32];

char ssid[32];
char password[32];

char AccessPointSSID[32];
char AccessPointPassword[32];

char friendlyName[32];

signed char timeZone;

u_int heartbeatInterval;

//  MQTT
char mqttServer[64];
uint16_t mqttPort;
char mqttTopic[64];
char mqttUserName[64];
char mqttPassword[64];

//  Operation
OPERATION_MODES opMode;

//  Calculated settings
char hostName[32];
char mqttRootTopic[32];
char mqttStateTopic[128];
char mqttCommandTopic[128];
char mqttHeartbeatTopic[128];
char mqttTelemetryTopic[128];

void PrintSettings()
{
    SerialMon.println("====================================== App settings ======================================");
    SerialMon.printf("App name\t\t%s\r\nAdmin password\t\t%s\r\nAP SSID\t\t\t%s\r\nAP Password\t\t%s\r\nTimezone\t\t%i\r\n"
                     "MQTT Server\t\t%s\r\nMQTT Port\t\t%u\r\nMQTT TOPIC\t\t%s\r\nMQTT USER\t\t%s\r\nMQTT PASSWORD\t\t%s\r\n"
                     "SSID\t\t\t%s\r\nPassword\t\t%s\r\nHearbeat interval\t%u\r\n"
                     "Operation mode\t\t%s\r\n",
                     friendlyName, adminPassword, AccessPointSSID, AccessPointPassword, timeZone,
                     mqttServer, mqttPort, mqttTopic, mqttUserName, mqttPassword, ssid, password,
                     heartbeatInterval,
                     GetOperationModeString());
    SerialMon.println("==========================================================================================");
}

void SaveSettings()
{
    prefs.begin(SETTINGS_NAME, false);

    //  System
    prefs.putUChar("FAILED_BOOT_ATT", FailedBootAttempts);
    prefs.putChar("TIMEZONE", timeZone);
    prefs.putUInt("HEARTBEAT_INTL", heartbeatInterval);
    prefs.putString("APP_NAME", friendlyName);

    prefs.putString("ADMIN_PASSWORD", adminPassword);

    prefs.putString("AP_SSID", AccessPointSSID);
    prefs.putString("AP_PASSWORD", AccessPointPassword);

    prefs.putString("SSID", ssid);
    prefs.putString("PASSWORD", password);

    //  MQTT
    prefs.putString("MQTT_SERVER", mqttServer);
    prefs.putUShort("MQTT_PORT", mqttPort);
    prefs.putString("MQTT_TOPIC", mqttTopic);
    prefs.putString("MQTT_USER", mqttUserName);
    prefs.putString("MQTT_PASSWORD", mqttPassword);

    //  Operation
    prefs.putUChar("OP_MODE", opMode);

    prefs.end();
    delay(100);

    Serial.println("Settings saved.");
}

void LoadSettings(bool LoadDefaults)
{
    char DEFAULT_AP_SSID[32];

    char c[ESP_ACCESS_POINT_NAME_SIZE];
    uint64_t chipid = ESP.getEfuseMac();

    sprintf(c, "ESP-%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
    strcpy(DEFAULT_AP_SSID, c);

    prefs.begin(SETTINGS_NAME, false);

    if (LoadDefaults)
        prefs.clear(); //  clear all settings

    //  System
    FailedBootAttempts = prefs.getUChar("FAILED_BOOT_ATT", 0) + 1;
    timeZone = prefs.getChar("TIMEZONE", DEFAULT_TIMEZONE);
    heartbeatInterval = prefs.getUInt("HEARTBEAT_INTL", DEFAULT_HEARTBEAT_INTERVAL);
    strcpy(friendlyName, (prefs.getString("APP_NAME", DEFAULT_APP_FRIENDLY_NAME).c_str()));

    strcpy(adminPassword, (prefs.getString("ADMIN_PASSWORD", DEFAULT_ADMIN_PASSWORD).c_str()));

    strcpy(AccessPointSSID, (prefs.getString("AP_SSID", DEFAULT_AP_SSID).c_str()));
    strcpy(AccessPointPassword, (prefs.getString("AP_PASSWORD", DEFAULT_AP_PASSWORD).c_str()));

    strcpy(ssid, (prefs.getString("SSID", DEFAULT_SSID).c_str()));
    strcpy(password, (prefs.getString("PASSWORD", DEFAULT_PASSWORD).c_str()));

    //  MQTT
    strcpy(mqttServer, (prefs.getString("MQTT_SERVER", DEFAULT_MQTT_SERVER).c_str()));
    mqttPort = prefs.getUShort("MQTT_PORT", DEFAULT_MQTT_PORT);
    strcpy(mqttTopic, (prefs.getString("MQTT_TOPIC").c_str()));
    strcpy(mqttUserName, (prefs.getString("MQTT_USER", DEFAULT_MQTT_USERNAME).c_str()));
    strcpy(mqttPassword, (prefs.getString("MQTT_PASSWORD", DEFAULT_MQTT_PASSWORD).c_str()));

    //  Operation
    // opMode = OPERATION_MODES::WIFI_SETUP;
    opMode = static_cast<OPERATION_MODES>(prefs.getUChar("OP_MODE", DEFAULT_OPERATION_MODE));

    prefs.end();

    //  Calculated settings

    sprintf(hostName, "vBridge-%s", GetDeviceMAC().substring(6).c_str());

    sprintf(mqttRootTopic, (MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + mqttTopic).c_str());
    sprintf(mqttStateTopic, ((String)mqttRootTopic + "/state").c_str());
    sprintf(mqttCommandTopic, ((String)mqttRootTopic + "/cmnd").c_str());
    sprintf(mqttHeartbeatTopic, ((String)mqttRootTopic + "/heartbeat").c_str());
    sprintf(mqttTelemetryTopic, ((String)mqttRootTopic + "/tele").c_str());
}

void ClearNVS()
{
    nvs_flash_erase(); // erase the NVS partition and...
    nvs_flash_init();  // initialize the NVS partition.
}

void InitSettings()
{
    //  Internal file system
    if (!SPIFFS.begin(false))
    {
        SerialMon.println("Error: Failed to initialize the internal filesystem!");
    }
    if (!SPIFFS.begin(true))
    {
        SerialMon.println("Error: Failed to format the internal filesystem!");
        delay(3000);
        ESP.restart();
    }

    LoadSettings();
    PrintSettings();
}
