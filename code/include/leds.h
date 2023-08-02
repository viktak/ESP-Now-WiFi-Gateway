#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

#define CONNECTION_STATUS_LED_GPIO 2
#define ACTIVITY_LED_GPIO 33

void SetLED(const uint8_t gpio, uint8_t val);
void InitLEDs();

#endif