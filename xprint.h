#ifndef PRINT_H
#define PRINT_H

#include <Arduino.h>
#include <avr/pgmspace.h>

void setupPrint();

void waitPrint();
void waitPrintln(const char* s);

void printOn_P(Print& out, PGM_P str);
void print_P(PGM_P str);

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734 
#ifdef PROGMEM 
#undef PROGMEM 
#define PROGMEM __attribute__((section(".progmem.data"))) 
#endif

#define printOn_C(out, str) { static const char _s[] PROGMEM = str; printOn_P(out, &_s[0]); }
#define print_C(str)        { static const char _s[] PROGMEM = str; print_P(&_s[0]); }

template<typename T> inline void print(const T& val) {
  Serial.print(val);
}

template<typename T> inline void print(const T& val, int base) {
  Serial.print(val, base);
} 

#endif

