#ifndef STATE_HAL_H_
#define STATE_HAL_H_

#include <FixNum.h>

enum State {
   STATE_OFF,
   STATE_SP,
   STATE_DP,
   STATE_KEEP
};

typedef fixnum16_1 temp_t;
typedef fixnum8_1 voltage_t;

extern int h0; // reported as a
extern int h1; // reported as b
extern int h3; // reported as c

void setupStateHal();
State getState();
temp_t getTemperature();
voltage_t getMinVoltage();

#endif /* STATE_HAL_H_ */
