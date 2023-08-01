#include "common.h"
#include "version.h"

#include <TimeLib.h>

char *stack_start;

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

void DateTimeToString(char *dest)
{
    sprintf(dest, "%u-%02u-%02u %02u:%02u:%02u", year(), month(), day(), hour(), minute(), second());
}

String TimeIntervalToString(const time_t time)
{

    String myTime = "";
    char s[2];

    //  hours
    itoa((time / 3600), s, DEC);
    myTime += s;
    myTime += ":";

    //  minutes
    if (minute(time) < 10)
        myTime += "0";

    itoa(minute(time), s, DEC);
    myTime += s;
    myTime += ":";

    //  seconds
    if (second(time) < 10)
        myTime += "0";

    itoa(second(time), s, DEC);
    myTime += s;
    return myTime;
}

const char *GetOperationModeString()
{
    switch (opMode)
    {
    case 0:
        return "WiFi setup";
        break;
    case 1:
        return "Normal";
        break;

    default:
        return "Unknown";
        break;
    }
}

String GetDeviceMAC()
{
    String s = WiFi.macAddress();

    for (size_t i = 0; i < 5; i++)
        s.remove(14 - i * 3, 1);

    return s;
}

uint32_t GetChipID()
{
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return chipId;
}

int8_t GetPrintableCardID(char *dest, const char *source)
{
    return sprintf(dest, "%02X%02X%02X%02X%02X%02X%02X", source[0], source[1], source[2], source[3], source[4], source[5], source[6]);
}

int8_t GetPrintableCardUID(char *dest, const char *source)
{
    return sprintf(dest, "%02X%02X%02X%02X", source[0], source[1], source[2], source[3]);
}

const char *GetResetReasonString(RESET_REASON reason)
{
    switch (reason)
    {
    case 1:
        return ("Vbat power on reset");
        break;
    case 3:
        return ("Software reset digital core");
        break;
    case 4:
        return ("Legacy watch dog reset digital core");
        break;
    case 5:
        return ("Deep Sleep reset digital core");
        break;
    case 6:
        return ("Reset by SLC module, reset digital core");
        break;
    case 7:
        return ("Timer Group0 Watch dog reset digital core");
        break;
    case 8:
        return ("Timer Group1 Watch dog reset digital core");
        break;
    case 9:
        return ("RTC Watch dog Reset digital core");
        break;
    case 10:
        return ("Instrusion tested to reset CPU");
        break;
    case 11:
        return ("Time Group reset CPU");
        break;
    case 12:
        return ("Software reset CPU");
        break;
    case 13:
        return ("RTC Watch dog Reset CPU");
        break;
    case 14:
        return ("for APP CPU, reset by PRO CPU");
        break;
    case 15:
        return ("Reset when the vdd voltage is not stable");
        break;
    case 16:
        return ("RTC Watch dog reset digital core and rtc module");
        break;
    default:
        return ("NO_MEAN");
    }
}

String GetFirmwareVersionString()
{
    return String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);
}

boolean checkInternetConnection()
{
    IPAddress timeServerIP;
    int result = WiFi.hostByName(INTERNET_SERVER_NAME, timeServerIP);
    return (result == 1);
}

uint32_t stack_size()
{
    char stack;
    return (uint32_t)stack_start - (uint32_t)&stack;
}