#ifndef CONFIG_H_
#define CONFIG_H_

#include <Arduino.h>
#include <avr/eeprom.h>
#include "state_hal.h"

class Config {
public:
  template<class T> class Byte {
  public:
    Byte();
    T read();
    Byte<T>& operator = (T value);
  private:
    Byte(const Byte<T>& other); // disable copy constructor
    byte _placeholder;
  };
  
  Byte<byte>        _reserved0;
  Byte<State>       state;       // see state_hal.h enum State
};

template<class T> Config::Byte<T>::Byte() {} // default constructor is empty

template<class T> T Config::Byte<T>::read() {
  return (T)eeprom_read_byte((uint8_t*)this);
}

template<class T> Config::Byte<T>& Config::Byte<T>::operator = (T value) {
  eeprom_write_byte((uint8_t*)this, (byte)value);
  return *this;
}

extern Config config;

#endif

