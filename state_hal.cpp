#include <Arduino.h>

#include "state_hal.h"

#define MEASURE_TIME 250L
#define CACHE_TIME 1000L
#define H0_PIN A0
#define H1_PIN A1

#define KEEP_THRESHOLD 30
#define SP_THRESHOLD 750

#define TEMP_PIN A3
#define TEMP_BASE 100L
#define TEMP_MUL  107L
#define TEMP_ADD  730L

int h0;
int h1;

State lastState;
long lastStateTime = -CACHE_TIME;

State getState() {
  long now = millis();
  if (now - lastStateTime < CACHE_TIME)
    return lastState;
  long h0sum = 0;
  long h1sum = 0;
  int cnt = 0;
  do {
    h0sum += analogRead(H0_PIN);
    h1sum += analogRead(H1_PIN);
    cnt++;
  } while (millis() - now < MEASURE_TIME);
  h0 = h0sum / cnt;
  h1 = h1sum / cnt;
  if (h0 == 0 && h1 == 0)
    lastState = STATE_OFF;
  else if (h0 < KEEP_THRESHOLD && h1 < KEEP_THRESHOLD)
    lastState = STATE_KEEP;  
  else if (h0 > SP_THRESHOLD || h0 < KEEP_THRESHOLD)  
    lastState = STATE_SP;
  else 
    lastState = STATE_DP;
  lastStateTime = millis();
  return lastState;
}

int getTemperature() {
  return (analogRead(TEMP_PIN) * TEMP_MUL + (TEMP_ADD + TEMP_BASE/2)) / TEMP_BASE;
}

