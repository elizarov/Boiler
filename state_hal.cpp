#include <Arduino.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "state_hal.h"

// ----------------------------------------------------------------
// Continuous analog measurement via interrupt

const uint8_t H0 = 0;
const uint8_t H1 = 1;
const uint8_t TEMP = 2;
const uint8_t VOLTAGE0 = 3;
const uint8_t VOLTAGE_LAST = 9;
const uint8_t MEASURE_COUNT = 10;

uint8_t measure;

// Note: A2 is also connected, but not used (not clear what it is)
const uint8_t inputs[MEASURE_COUNT] = { 0, 1, 3, 0x0e };

const uint32_t TEMP_BASE = 100L;
const uint32_t TEMP_MUL = 107L;
const uint32_t TEMP_ADD = 730L;

uint32_t h0sum;
uint16_t h0cnt;
uint16_t h0avg;

uint32_t h1sum;
uint16_t h1cnt;
uint16_t h1avg;

uint16_t tLast;
voltage_t vMin;

// each measurement takes 13 clock cycles (125kHz) ~= 9.6 KHz conversion speed.
// we have 10 values to measure in a look, each at ~ 960Hz
// we'll update atate at ~ 500 ms intervals

const uint16_t AVG_CNT = 480;

// we gather ~57k measurements per minute. 
// We'll use 16 bit min queue for minVoltage (for slightly > 1 min)

const uint8_t V_QUEUE_LEN = 16;
voltage_t vQueue[2 * V_QUEUE_LEN];
uint16_t vQueueCnt;

void startMeasure() {
  uint8_t input = measure < VOLTAGE0 ? inputs[measure] : inputs[VOLTAGE0];
  ADMUX = input | _BV(REFS0); // external cap on AREF pin
  // Assume F_CPU = 16MHz and set prescaler to 128 (125KHz clock)
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

void setupStateHal() {
  memset(&vQueue, 0xfe, sizeof(vQueue)); // 0xfe is valid, but higher that any other value
  startMeasure();
}

inline void updateVQueue(voltage_t v) {
  uint16_t cnt = vQueueCnt++;
  for (uint8_t i = 0; i < V_QUEUE_LEN; i++) {
    vQueue[2 * i + (cnt & 1)] = v;
    v = min(vQueue[2 * i], vQueue[2 * i + 1]);    
    cnt >>= 1;
  }
  vMin = v;
}

ISR(ADC_vect) {
  switch (measure) {
    case H0: 
      h0sum += ADC;
      if (++h0cnt >= AVG_CNT) {
        h0avg = h0sum / h0cnt;
        h0sum = 0;
        h0cnt = 0;
      }
      break;
    case H1: 
      h1sum += ADC;
      if (++h1cnt >= AVG_CNT) {
        h1avg = h1sum / h1cnt;
        h1sum = 0;
        h1cnt = 0;
      }
      break;
    case TEMP:
      tLast = ADC;
      break;
    case VOLTAGE_LAST:    
      updateVQueue(fixnum16_1(11) * 1023 / int16_t(ADC));
      break;
    default:
      break; // discard measurement  
  }
  if (++measure == MEASURE_COUNT)
    measure = 0;
  startMeasure();
}

// ----------------------------------------------------------------
// Measurement interpretation

const int KEEP_THRESHOLD = 30;
const int SP_THRESHOLD = 750;

int h0;
int h1;
int h3;

State getState() {
  cli();
  h0 = h0avg;
  h1 = h1avg;
  sei();
  if (h0 == 0 && h1 == 0)
    return STATE_OFF;
  else if (h0 < KEEP_THRESHOLD && h1 < KEEP_THRESHOLD)
    return STATE_KEEP;  
  else if (h0 > SP_THRESHOLD || h0 < KEEP_THRESHOLD)  
    return STATE_SP;
  else 
    return STATE_DP;
}

temp_t getTemperature() {
  cli();
  h3 = tLast;
  sei();
  return temp_t((h3 * TEMP_MUL + (TEMP_ADD + TEMP_BASE/2)) / TEMP_BASE);
}

voltage_t getMinVoltage() {
  return vMin;
}
