#ifndef STATE_HAL_H_
#define STATE_HAL_H_

enum State {
   STATE_OFF,
   STATE_SP,
   STATE_DP,
   STATE_KEEP
};

extern int h0;
extern int h1;

State getState();
int getTemperature();

#endif /* STATE_HAL_H_ */
