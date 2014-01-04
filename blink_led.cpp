#include <Arduino.h>
#include "blink_led.h"

const int BLINK_LED_PIN = LED_BUILTIN;

unsigned long blinkSwitchTime;
boolean blinkLedState = false;

void blinkLed(unsigned int time) {
  unsigned long now = millis();
  if (now - blinkSwitchTime > time) {  
    blinkLedState = !blinkLedState;
    blinkSwitchTime = now;
    digitalWrite(BLINK_LED_PIN, blinkLedState);
  }
}


