#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>

#define FMT_PREC     0x0f  // define number precision in lower bits
#define FMT_SIGN     0x10  // print sign at the first position 
#define FMT_SPACE    0x20  // fill with spaces (with zeroes by default)
#define FMT_LEFT     0x40  // left-align the result (use in combination with FMT_SPACE)

extern const char HEX_CHARS[];

extern uint8_t formatDecimal(int16_t x, char* pos, uint8_t size, uint8_t fmt = 0);
extern uint8_t formatDecimal(int32_t x, char* pos, uint8_t size, uint8_t fmt = 0);

#endif

