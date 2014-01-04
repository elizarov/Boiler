#include <Arduino.h>

#include "parse.h"

#define PARSE_ANY      0
#define PARSE_ATTN     1      // Attention char '!' received, wait for 'B'
#define PARSE_WCMD     2      // '!B' was read, wait for command char

byte parseState = PARSE_ANY;
byte parseArg;

inline char parseChar(char ch) {
  switch (parseState) {
  case PARSE_ANY:
    if (ch == '!')
      parseState = PARSE_ATTN;
    break;
  case PARSE_ATTN:
    parseState = (ch == 'B') ? PARSE_WCMD : PARSE_ANY;
    break;
  case PARSE_WCMD:
    switch (ch) {
    case CMD_ON_OFF: case CMD_POWER: case CMD_QUERY:
      parseState = PARSE_ANY;
      return ch; // command for external processing
    default:
      parseState = PARSE_ANY;
    }
    break;
  }
  return 0;
}

char parseCommand() {
  while (Serial.available()) {
    char cmd = parseChar(Serial.read());
    if (cmd != 0)
      return cmd;
  }
  return 0;
}
