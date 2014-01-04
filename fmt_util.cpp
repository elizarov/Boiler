#include "fmt_util.h"

const char HEX_CHARS[17] = "0123456789ABCDEF";

// Replaces all digits with '9' on overflow
static void fillOverflow(char* pos, uint8_t size) {
  for (uint8_t i = 0; i < size; i++)
    if (pos[i] >= '0' && pos[i] < '9')
      pos[i] = '9';
}

// Moves to the left and fills with spaces on the right
static void moveLeft(char* pos, uint8_t size, uint8_t actualSize) {
  for (uint8_t i = 0; i < actualSize; i++)
    pos[i] = pos[i + size - actualSize];
  for (uint8_t i = actualSize; i < size; i++)
    pos[i] = ' ';
}

uint8_t formatDecimal(int16_t x, char* pos, uint8_t size, uint8_t fmt) {
  char sc = (fmt & FMT_SPACE) ? ' ' : '+';
  if (x < 0) {
    x = -x;
    sc = '-';
  }
  uint8_t actualSize = 0;
  uint8_t first = (fmt & FMT_PREC) ? (fmt & FMT_PREC) + 1 : 0;
  char* ptr = pos + size;
  for (uint8_t i = 0; i < size; i++) {
    ptr--;
    if (i + 1 == first) {
      *ptr = '.';
      actualSize++;
    } else if ((fmt & FMT_SPACE) && x == 0 && i > first) {
      *ptr = sc;
      if (sc != ' ')
        actualSize++;
      sc = ' '; // fill the rest with spaces
    } else if ((fmt & FMT_SIGN) && i == size - 1) {
      *ptr = sc;
      actualSize++;
    } else {
      *ptr = '0' + x % 10;
      x /= 10;
      actualSize++;
    }
  }
  if (x != 0)
    fillOverflow(pos, size);
  if ((fmt & FMT_LEFT) && actualSize < size)
    moveLeft(pos, size, actualSize);
  return actualSize;
}

uint8_t formatDecimal(int32_t x, char* pos, uint8_t size, uint8_t fmt) {
  char sc = (fmt & FMT_SPACE) ? ' ' : '+';
  if (x < 0) {
    x = -x;
    sc = '-';
  }
  uint8_t actualSize = 0;
  uint8_t first = (fmt & FMT_PREC) ? (fmt & FMT_PREC) + 1 : 0;
  char* ptr = pos + size;
  for (uint8_t i = 0; i < size; i++) {
    ptr--;
    if (i + 1 == first) {
      *ptr = '.';
      actualSize++;
    } else if ((fmt & FMT_SPACE) && x == 0 && i > first) {
      *ptr = sc;
      if (sc != ' ')
        actualSize++;
      sc = ' '; // will the rest with space with this branch  
    } else if ((fmt & FMT_SIGN) && i == size - 1) {
      *ptr = sc;
      actualSize++;
    } else {
      *ptr = '0' + x % 10;
      x /= 10;
      actualSize++;
    }
  }
  if (x != 0)
    fillOverflow(pos, size);
  if ((fmt & FMT_LEFT) && actualSize < size)
    moveLeft(pos, size, actualSize);
  return actualSize;
}
