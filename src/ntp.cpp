#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define DEBUG_NTPClient
#include <NTPClient.h>

#include "ntp.h"
#include "common.h"

WiFiUDP ntpUDP;

#ifdef __debugSettings
char timeServer[] = "192.168.123.2";
#else
char timeServer[] = "pool.ntp.org";
#endif

NTPClient timeClient(ntpUDP, timeServer, 0, NTP_UPDATE_INTERVAL_MS);

void InitNTP()
{
    timeClient.begin();
}

void NTPloop()
{
    timeClient.update();
    if (timeClient.isTimeSet())
    {
        setTime(timeClient.getEpochTime());
    }
}
