#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "mqtt.h"
#include "common.h"
#include "version.h"
#include "settings.h"
#include "main.h"
#include "esp-now.h"

#define MQTT_BUFFER_SIZE 1024 * 5
#define MQTT_KEEP_ALIVE_PERIOD 60 //  seconds

WiFiClient client;
PubSubClient PSclient(client);

void ConnectToMQTTBroker()
{
    if (!PSclient.connected())
    {
        SerialMon.printf("Connecting to MQTT broker %s... ", mqttServer);

        char msg[100];
        sprintf(msg, "{\"System\":{\"Hostname\":\"%s\",\"Status\":\"offline\"}}", hostName);

        if (PSclient.connect(hostName, "", mqttPassword, mqttStateTopic, 0, true, msg))
        {
            SerialMon.println(" Success.");
            sprintf(msg, "{\"System\":{\"Hostname\":\"%s\",\"Status\":\"online\"}}", hostName);

            PSclient.publish(mqttStateTopic, msg, true);

            //  Listening to commands
            PSclient.subscribe(mqttCommandTopic, 0);
        }
        else
        {
            SerialMon.println(" failed. Check broker settings, credentials, access.");
        }
    }
}

void SendDataToBroker(const char *topic, const char data[], bool retained)
{
    if (PSclient.connected())
    {
        PSclient.publish(topic, data, retained);
    }
}

void SendMeasurement(const uint64_t ordinal, const uint32_t nodeID, const DeviceAddress deviceAddress, const float sensorValue, const uint64_t failuresCount, const float batteryLevel)
{
    char da[24];

    sprintf(da, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", deviceAddress[0], deviceAddress[1], deviceAddress[2], deviceAddress[3], deviceAddress[4], deviceAddress[5], deviceAddress[6], deviceAddress[7]);

    char buffer[192];
    StaticJsonDocument<192> doc;

    JsonObject Measurement = doc.createNestedObject("Measurement");
    Measurement["Ordinal"] = ordinal;
    Measurement["Hostname"] = nodeID;
    Measurement["SensorID"] = da;
    Measurement["Failures"] = failuresCount;
    Measurement["Value"] = sensorValue;
    Measurement["Battery"] = batteryLevel;

    serializeJson(doc, buffer);

    SendDataToBroker(mqttTelemetryTopic, buffer, false);
}

void SendHeartbeat()
{
    char buffer[MQTT_BUFFER_SIZE];
    StaticJsonDocument<768> doc;

    JsonObject System = doc.createNestedObject("System");
    System["Hostname"] = hostName;
    System["Mode"] = GetOperationModeString();
    System["UpTime"] = TimeIntervalToString(millis() / 1000);
    System["CPU0_ResetReason"] = GetResetReasonString(rtc_get_reset_reason(0));
    System["CPU1_ResetReason"] = GetResetReasonString(rtc_get_reset_reason(1));

    serializeJson(doc, buffer);

    SendDataToBroker(mqttHeartbeatTopic, buffer, false);
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, payload, len);

    if (error)
    {
        Serial.print("Error decoding JSON data: ");
        Serial.println(error.c_str());
        return;
    }

#ifdef __debugSettings
    SerialMon.print("Message received in [");
    SerialMon.print(topic);
    SerialMon.println("]: ");

    serializeJsonPretty(doc, SerialMon);
    SerialMon.println();
#endif

    const char *command = doc["command"];
    if (command != NULL)
    {
    }
}

void InitMQTT()
{
    PSclient.setServer(mqttServer, mqttPort);
    PSclient.setKeepAlive(MQTT_KEEP_ALIVE_PERIOD);
    PSclient.setCallback(mqttCallback);
    if (!PSclient.setBufferSize(MQTT_BUFFER_SIZE))
    {
        SerialMon.printf("Could not resize MQTT buffer to %u.\r\n", MQTT_BUFFER_SIZE);
    }
}

void MQTTLoop()
{

    if (PSclient.connected())
    {
        PSclient.loop();
    }
    else
    {
        ConnectToMQTTBroker();
    }
}