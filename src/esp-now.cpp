#include "esp-now.h"

#include "mqtt.h"
#include "leds.h"

struct_message myData;

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    SetLED(ACTIVITY_LED_GPIO, LOW);

    memcpy(&myData, data, sizeof(myData));

    Serial.printf("%u bytes of data received:\r\n", len);
    Serial.printf("Ordinal:\t%u\r\n", (uint32_t)myData.ordinal);
    Serial.printf("Node ID:\t%u\r\n", myData.nodeID);
    Serial.printf("Sensor ID:\t%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\r\n", myData.deviceAddress[0], myData.deviceAddress[1], myData.deviceAddress[2], myData.deviceAddress[3], myData.deviceAddress[4], myData.deviceAddress[5], myData.deviceAddress[6], myData.deviceAddress[07]);
    Serial.printf("Temperature:\t%f\r\n", myData.sensorValue);
    Serial.printf("Failures:\t%u\r\n", (uint32_t)myData.failuresCount);
    Serial.printf("Battery level:\t%fV\r\n", myData.batteryLevel);

    SendMeasurement(myData.ordinal, myData.nodeID, myData.deviceAddress, myData.sensorValue, myData.failuresCount, myData.batteryLevel);

        SetLED(ACTIVITY_LED_GPIO, HIGH);

}

void initEspNow()
{

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP NOW failed to initialize");
        while (1)
            ;
    }

    esp_now_register_recv_cb(onReceive);

    Serial.println("ESP-Now initialized.");
}
