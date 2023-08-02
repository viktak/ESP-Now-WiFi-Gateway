#define __debugSettings

#include <Arduino.h>
#include <rom/rtc.h>
#include <WiFi.h>

#include <ESPmDNS.h>

#include <asyncwebserver.h>
#include <DallasTemperature.h>

#include "main.h"
#include "buttons.h"
#include "common.h"
#include "mqtt.h"
#include "settings.h"
#include "leds.h"
#include "ntp.h"
#include "esp-now.h"
#include "version.h"

bool needsHeartbeat = false;

unsigned long oldMillis = 0;
bool isStartup = true;
bool isAccessPointCreated = false;
bool wifiTimerExpired = false;
bool ntpInitialized = false;

enum CONNECTION_STATE
{
    STATE_CHECK_WIFI_CONNECTION,
    STATE_WIFI_CONNECT,
    STATE_CHECK_INTERNET_CONNECTION,
    STATE_INTERNET_CONNECTED
};

enum CONNECTION_STATE connectionState;

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t *heartbeatTimer;
volatile SemaphoreHandle_t heartbeatTimerSemaphore;

hw_timer_t *wiFiModeTimer;
volatile SemaphoreHandle_t wiFiModeTimerSemaphore;

void IRAM_ATTR heartbeatTimerCallback()
{
    portENTER_CRITICAL_ISR(&timerMux);

    needsHeartbeat = true;

    portEXIT_CRITICAL_ISR(&timerMux);
    xSemaphoreGiveFromISR(heartbeatTimerSemaphore, NULL);
}

void IRAM_ATTR wifiModeTimerCallback()
{
    portENTER_CRITICAL_ISR(&timerMux);

    wifiTimerExpired = true;

    portEXIT_CRITICAL_ISR(&timerMux);
    xSemaphoreGiveFromISR(wiFiModeTimerSemaphore, NULL);
}

void StartWifiTimer()
{
    wiFiModeTimerSemaphore = xSemaphoreCreateBinary();
    wiFiModeTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(wiFiModeTimer, &wifiModeTimerCallback, true);
    timerAlarmWrite(wiFiModeTimer, WIFI_MODE_TIMEOUT * 1000000, true);
    timerAlarmEnable(wiFiModeTimer);
}

void StartHeartbeatTimer()
{
    timerAlarmWrite(heartbeatTimer, heartbeatInterval * 1000000, true);
    timerAlarmEnable(heartbeatTimer);
}

void InitHeartbeatTimer()
{
    heartbeatTimerSemaphore = xSemaphoreCreateBinary();
    heartbeatTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(heartbeatTimer, &heartbeatTimerCallback, true);
}

void setup()
{
    char stack;
    stack_start = &stack;

    SerialMon.begin(DEBUG_SPEED);
    delay(1000);

    String FirmwareVersionString = String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);

    SerialMon.printf("\r\n\n\nBooting ESP node %u...\r\n\n", GetChipID());
    SerialMon.println("==========================================================================================");
    SerialMon.println("   diy.viktak.com");
    SerialMon.println("------------------------------------------------------------------------------------------");

    SerialMon.println("-  Hardware ID:      " + (String)HARDWARE_ID);
    SerialMon.println("-  Hardware version: " + (String)HARDWARE_VERSION);
    SerialMon.println("-  Software ID:      " + (String)FIRMWARE_ID);
    SerialMon.println("-  Software version: " + FirmwareVersionString);
    SerialMon.println();

    InitLEDs();

    SetLED(CONNECTION_STATUS_LED_GPIO, HIGH);

    InitSettings();

    pinMode(BUTTON_MODE_GPIO, INPUT_PULLUP);

    delay(1000);

    // if (!digitalRead(BUTTON_MODE_GPIO)) //  If mode force button is pressed, force WiFi mode
    // {
    //     SerialMon.println("Switching to WiFi mode...");
    //     opMode = OPERATION_MODES::WIFI_SETUP;
    //     SaveSettings();
    //     ESP.restart();
    // }

    // SerialMon.printf("%s mode (#%u) selected.\r\n", GetOperationModeString(), opMode);

    SetRandomSeed();

    InitMQTT();

    InitHeartbeatTimer();
    StartHeartbeatTimer();

    connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;

    SerialMon.println("Setup finished.");
}

void loop()
{

    if (opMode == OPERATION_MODES::WIFI_SETUP)
    {
        if (wifiTimerExpired)
        {
            SerialMon.println("WiFi mode is now over. Restarting in Normal mode....");
            opMode = OPERATION_MODES::NORMAL;
            SaveSettings();
            ESP.restart();
        }

        if (!isAccessPointCreated)
        {
            SerialMon.println("Creating Acces Point...");

            delay(500);

            WiFi.mode(WIFI_AP);
            WiFi.softAP(AccessPointSSID, AccessPointPassword);

            IPAddress myIP = WiFi.softAPIP();
            isAccessPointCreated = true;

            SerialMon.println("Access point created. Use the following information to connect to the ESP device:");

            SerialMon.printf("SSID:\t\t\t%s\r\nPassword:\t\t%s\r\nAccess point address:\t%s\r\n",
                             AccessPointSSID, AccessPointPassword, myIP.toString().c_str());

            InitAsyncWebServer();

            StartWifiTimer();
        }
    }
    else if (opMode == OPERATION_MODES::NORMAL)
    {
        switch (connectionState)
        {

        // Check the WiFi connection
        case STATE_CHECK_WIFI_CONNECTION:

            // Are we connected ?
            if (WiFi.status() != WL_CONNECTED)
            {
                // Wifi is NOT connected
                SetLED(CONNECTION_STATUS_LED_GPIO, HIGH);
                connectionState = CONNECTION_STATE::STATE_WIFI_CONNECT;
            }
            else
            {
                // Wifi is connected so check Internet
                SetLED(CONNECTION_STATUS_LED_GPIO, LOW);
                connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;
            }
            break;

        // No Wifi so attempt WiFi connection
        case STATE_WIFI_CONNECT:
        {
            // Indicate NTP no yet initialized
            ntpInitialized = false;

            SetLED(CONNECTION_STATUS_LED_GPIO, HIGH);
            Serial.printf("Trying to connect to WIFI network: %s", ssid);

            // Set mixed mode
            WiFi.mode(WIFI_MODE_APSTA);

            // Start connection process
            WiFi.setHostname(hostName);
            WiFi.begin(ssid, password);

            // Initialize iteration counter
            uint8_t attempt = 0;

            while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT))
            {
                SetLED(CONNECTION_STATUS_LED_GPIO, LOW);
                Serial.printf(".");
                delay(50);
                SetLED(CONNECTION_STATUS_LED_GPIO, HIGH);
                delay(950);
            }

            if (attempt >= WIFI_CONNECTION_TIMEOUT)
            {
                Serial.println("\r\nCould not connect to WiFi.");
                delay(100);

                opMode = OPERATION_MODES::WIFI_SETUP;

                SaveSettings();
                ESP.restart();

                break;
            }

            SetLED(CONNECTION_STATUS_LED_GPIO, LOW);
            Serial.printf(" Success.\r\n");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            SerialMon.printf("Hostname: %s\r\n", hostName);

            if (MDNS.begin(hostName))
                SerialMon.println("MDNS responder started.");

            String apSSID = "vBridge-" + WiFi.softAPmacAddress().substring(8);
            apSSID.replace(":", "");

            SerialMon.printf("Creating Access Point: %s... ", apSSID.c_str());
            WiFi.softAP(apSSID.c_str(), AccessPointPassword);
            SerialMon.println("Success.");
            SerialMon.printf("Soft AP MAC address: %s\r\n", WiFi.softAPmacAddress().c_str());

            InitAsyncWebServer();
            initEspNow();

            connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;
        }
        break;

        case STATE_CHECK_INTERNET_CONNECTION:

            // Do we have a connection to the Internet ?
            if (checkInternetConnection())
            {
                // We have an Internet connection

                if (!ntpInitialized)
                {
                    // We are connected to the Internet for the first time so set NTP provider
                    InitNTP();

                    ntpInitialized = true;

                    SerialMon.println("Connected to the Internet.");
                }

                connectionState = CONNECTION_STATE::STATE_INTERNET_CONNECTED;
            }
            else
            {
                connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            }
            break;

        case STATE_INTERNET_CONNECTED:

            // ArduinoOTA.handle();

            if (needsHeartbeat)
            {
                SendHeartbeat();
                needsHeartbeat = false;
            }

            NTPloop();

            MQTTLoop();

            // Set next connection state
            connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            break;
        }
    }
}
