#ifndef TIMEOUT_H_
#define TIMEOUT_H_

#include <Arduino.h>

/**
 * Simple timeout class that "fires" once by returning true from its "check" method when
 * previously spcified time interval passes. Auto-repeat is not supported. Call "reset".
 */
class Timeout {
  private:
    unsigned long _time;
  public:
    static const unsigned long SECOND = 1000UL;
    static const unsigned long MINUTE = 60 * SECOND;
    static const unsigned long HOUR = 60 * MINUTE;
    static const unsigned long DAY = 24 * HOUR;
    
    Timeout();
    Timeout(unsigned long interval);
    boolean check(); 
    boolean enabled();
    void disable();
    void reset(unsigned long interval);
};

inline Timeout::Timeout() {}

inline Timeout::Timeout(unsigned long interval) { 
  reset(interval);
}

inline boolean Timeout::enabled() {
  return _time != 0;
}

inline void Timeout::disable() {
  _time = 0;
}

#endif

