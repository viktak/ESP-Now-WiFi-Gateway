#include <Arduino.h>
#include <Update.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "asyncwebserver.h"
#include "common.h"
#include "version.h"
#include "settings.h"

const char *ADMIN_USERNAME = "admin";

AsyncWebServer server(80);

size_t updateSize = 0;
bool shouldReboot = false;

//  Helper functions - not used in production
void PrintParameters(AsyncWebServerRequest *request)
{
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        if (p->isFile())
        { // p->isPost() is also true
            SerialMon.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        }
        else if (p->isPost())
        {
            SerialMon.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
        else
        {
            SerialMon.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
    }
}

void PrintHeaders(AsyncWebServerRequest *request)
{
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
        AsyncWebHeader *h = request->getHeader(i);
        SerialMon.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
}

// Root/Status
String StatusTemplateProcessor(const String &var)
{
    //  System information
    if (var == "chipid")
        return AccessPointSSID;
    if (var == "hardwareid")
        return HARDWARE_ID;
    if (var == "hardwareversion")
        return HARDWARE_VERSION;
    if (var == "softwareid")
        return FIRMWARE_ID;
    if (var == "firmwareversion")
        return GetFirmwareVersionString();
    if (var == "chipid")
        return AccessPointSSID;
    if (var == "currenttime")
    {
        char dateTime[32];
        DateTimeToString(dateTime);
        return dateTime;
    }

    //  Network
    switch (WiFi.getMode())
    {
    case WIFI_MODE_AP:
        if (var == "ssid")
            return "";
        if (var == "channel")
            return "";
        if (var == "macaddress")
            return "";
        if (var == "ipaddress")
            return "";
        break;
    case WIFI_MODE_APSTA:
        if (var == "ssid")
            return ssid;
        if (var == "channel")
            return (String)WiFi.channel();

        if (var == "macaddress")
            return WiFi.BSSIDstr();
        if (var == "ipaddress")
            return WiFi.localIP().toString();
        break;
    default:
        break;
    }

    String apSSID = "vBridge-" + WiFi.softAPmacAddress().substring(8);
    apSSID.replace(":", "");

    if (var == "apssid")
        return apSSID;
    if (var == "apmacaddress")
        return WiFi.softAPmacAddress();

    //  MQTT
    if (var == "mqtt-server")
        return mqttServer;
    if (var == "mqtt-port")
        return ((String)mqttPort).c_str();
    if (var == "mqtt-topic")
        return mqttRootTopic;
    if (var == "mqtt-username")
        return mqttUserName;

    return String();
}

//  MQTT
String MQTTSettingsTemplateProcessor(const String &var)
{
    if (var == "mqtt-servername")
        return mqttServer;
    if (var == "mqtt-port")
        return ((String)mqttPort).c_str();
    if (var == "mqtt-username")
        return mqttUserName;
    if (var == "mqtt-password")
        return mqttPassword;
    if (var == "mqtt-topic")
        return mqttTopic;
    if (var == "HeartbeatInterval")
        return ((String)heartbeatInterval).c_str();

    return String();
}

void POSTMQTTSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("txtServerName", true))
    {
        p = request->getParam("txtServerName", true);
        sprintf(mqttServer, "%s", p->value().c_str());
    }

    if (request->hasParam("numPort", true))
    {
        p = request->getParam("numPort", true);
        mqttPort = atoi(p->value().c_str());
    }

    if (request->hasParam("txtUserName", true))
    {
        p = request->getParam("txtUserName", true);
        sprintf(mqttUserName, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(mqttPassword, "%s", p->value().c_str());
    }

    if (request->hasParam("txtTopic", true))
    {
        p = request->getParam("txtTopic", true);
        sprintf(mqttTopic, "%s", p->value().c_str());
    }

    if (request->hasParam("numHeartbeatInterval", true))
    {
        p = request->getParam("numHeartbeatInterval", true);
        heartbeatInterval = atoi(p->value().c_str());
    }

    SaveSettings();
}

//  Network
String NetworkSettingsTemplateProcessor(const String &var)
{
    if (var == "networklist")
    {
        byte numberOfNetworks = WiFi.scanNetworks();
        String wifiList = "";
        for (size_t i = 0; i < numberOfNetworks; i++)
        {
            wifiList += "<option ";
            wifiList += "name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "<option>";
        }

        return wifiList;
    }

    return String();
}

void POSTNetworkSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    if (request->hasParam("lstNetworks", true))
    {
        p = request->getParam("lstNetworks", true);
        sprintf(ssid, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(password, "%s", p->value().c_str());
    }

    SaveSettings();
}

//  Tools
String ToolsTemplateProcessor(const String &var)
{
    return String();
}

/// Init
void InitAsyncWebServer()
{
    //  Bootstrap
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript"); });

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/bootstrap.min.css", "text/css"); });

    server.on("/menu.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/menu.png", "image/png"); });

    //  Logout
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->requestAuthentication();
                  request->redirect("/"); });

    //  Status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/status.html", String(), false, StatusTemplateProcessor); });

    server.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/status.html", String(), false, StatusTemplateProcessor); });

    //  MQTT
    server.on("/mqtt-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, adminPassword))
                    return request->requestAuthentication();
                request->send(SPIFFS, "/mqtt-settings.html", String(), false, MQTTSettingsTemplateProcessor); });

    server.on("/mqtt-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  PrintParameters(request);
                  POSTMQTTSettings(request);

                  ESP.restart();

                  request->redirect("/mqtt-settings.html"); });

    //  Network
    server.on("/network-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, adminPassword))
                    return request->requestAuthentication();
                request->send(SPIFFS, "/network-settings.html", String(), false, NetworkSettingsTemplateProcessor); });

    server.on("/network-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  PrintParameters(request);
                  POSTNetworkSettings(request);

                  ESP.restart();

                  request->redirect("/network-settings.html"); });

    //  Tools
    server.on("/tools.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, adminPassword))
                        return request->requestAuthentication();
                    request->send(SPIFFS, "/tools.html", String(), false, ToolsTemplateProcessor); });

    server.on("/mode-wifi", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  if (!request->authenticate(ADMIN_USERNAME, adminPassword))
                      return request->requestAuthentication();

                  opMode = OPERATION_MODES::WIFI_SETUP;
                  SaveSettings();
                  ESP.restart(); });

    server.on("/mode-normal", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                  if (!request->authenticate(ADMIN_USERNAME, adminPassword))
                      return request->requestAuthentication();

                  opMode = OPERATION_MODES::NORMAL;
                  SaveSettings();
                  ESP.restart(); });

    AsyncElegantOTA.begin(&server, ADMIN_USERNAME, adminPassword);

    server.begin();
    SerialMon.println("AsyncWebServer started.");
}
