#include "xprint.h"
#include "Timeout.h"

const long INITIAL_PRINT_INTERVAL = 1000L; // wait 1 s before first print to get XBee time to initialize & join
const long PRINT_INTERVAL         = 250L;  // wait 250 ms between prints 

Timeout printTimeout(INITIAL_PRINT_INTERVAL);

void setupPrint() {
  Serial.begin(57600);  
}

void waitPrint() {
  while (!printTimeout.check()); // just wait...
  printTimeout.reset(PRINT_INTERVAL);
}

void waitPrintln(const char* s) {
  waitPrint();
  Serial.println(s);
}

void printOn_P(Print& out, PGM_P str) {
  while (1) {
    char ch = pgm_read_byte_near(str++);
    if (!ch)
      return;
    out.write(ch);
  }
}

void print_P(PGM_P str) { 
  printOn_P(Serial, str); 
}

