#include "leds.h"


void SetLED(const uint8_t gpio, uint8_t val){
    digitalWrite(gpio, val);
}

void InitLEDs()
{
    pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
    SetLED(CONNECTION_STATUS_LED_GPIO, HIGH);

    pinMode(ACTIVITY_LED_GPIO, OUTPUT);
    SetLED(ACTIVITY_LED_GPIO, HIGH);

}