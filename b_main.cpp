#include "Timeout.h"
#include "xprint.h"
#include "fmt_util.h"
#include "blink_led.h"
#include "parse.h"
#include "state_hal.h"
#include "Config.h"

//------- ALL TIME DEFS ------

#define INITIAL_DUMP_INTERVAL 2000L  // 2 sec
#define PERIODIC_DUMP_INTERVAL 60000L // 1 min
#define PERIODIC_DUMP_SKEW 5000L      // 5 sec 

#define RESTORE_STATE_INTERVAL   60000L   // restore after 1 min
#define RESTORE_TEMP_LIMIT       60       // restore only below 60 degrees C
#define RESTORE_RECHECK_INTERVAL 10000L   // recheck every 10 sec if above 60

//------- DUMP STATE -------

#define HIGHLIGHT_CHAR '*'

boolean firstDump = true; 
Timeout dump(INITIAL_DUMP_INTERVAL);
char dumpLine[] = "[B:0 t00.0;u00000000](a0000b0000c0000d0000)* ";

byte indexOf(byte start, char c) {
  for (byte i = start; dumpLine[i] != 0; i++)
    if (dumpLine[i] == c)
      return i;
  return 0;
}

#define POSITIONS0(P0,C2,POS,SIZE)                 \
        byte POS = P0;                             \
      	byte SIZE = indexOf(POS, C2) - POS;

#define POSITIONS(C1,C2,POS,SIZE)                  \
        POSITIONS0(indexOf(0, C1) + 1,C2,POS,SIZE)

byte highlightPos = indexOf(0, HIGHLIGHT_CHAR);

POSITIONS(':', ' ', sPos, sSize)
POSITIONS('t', ';', tPos, tSize)
POSITIONS('a', 'b', aPos, aSize)
POSITIONS('b', 'c', bPos, bSize)
POSITIONS('c', 'd', cPos, cSize)
POSITIONS('d', ')', dPos, dSize)
POSITIONS('u', ']', uptimePos, uptimeSize)

#define DAY_LENGTH_MS (24 * 60 * 60000L)

long daystart = 0;
int updays = 0;

inline void prepareDecimal(int x, int pos, byte size, byte fmt = 0) {
  formatDecimal(x, &dumpLine[pos], size, fmt);
}

#define DUMP_REGULAR               0
#define DUMP_FIRST                 HIGHLIGHT_CHAR
#define DUMP_ON_OFF                'o'
#define DUMP_POWER                 'p'
#define DUMP_QUERY                 '?'

void makeDump(char dumpType) {
  prepareDecimal(getState(), sPos, sSize);
  prepareDecimal(getTemperature(), tPos, tSize, 1);
  
  // prepare values
  prepareDecimal(h0, aPos, aSize);
  prepareDecimal(h1, bPos, bSize);
  prepareDecimal(analogRead(A2), cPos, cSize);
  prepareDecimal(analogRead(A3), dPos, dSize);

  // prepare uptime
  long time = millis();
  while ((time - daystart) > DAY_LENGTH_MS) {
    daystart += DAY_LENGTH_MS;
    updays++;
  }
  prepareDecimal(updays, uptimePos, uptimeSize - 6);
  time -= daystart;
  time /= 1000; // convert seconds
  prepareDecimal(time % 60, uptimePos + uptimeSize - 2, 2);
  time /= 60; // minutes
  prepareDecimal(time % 60, uptimePos + uptimeSize - 4, 2);
  time /= 60; // hours
  prepareDecimal((int) time, uptimePos + uptimeSize - 6, 2);

  // print
  if (dumpType == DUMP_REGULAR) {
    dumpLine[highlightPos] = 0;
  } else {
    byte i = highlightPos;
    dumpLine[i++] = dumpType;
    if (dumpType != HIGHLIGHT_CHAR)
      dumpLine[i++] = HIGHLIGHT_CHAR; // must end with highlight (signal) char
    dumpLine[i++] = 0; // and the very last char must be zero
  }
  waitPrintln(dumpLine);
  dump.reset(PERIODIC_DUMP_INTERVAL + random(-PERIODIC_DUMP_SKEW, PERIODIC_DUMP_SKEW));
  firstDump = false;
}

inline void dumpState() {
  if (dump.check())
    makeDump(firstDump ? DUMP_FIRST : DUMP_REGULAR);
}

//------- EXECUTE COMMAND -------

#define CMD_TIMEOUT 300 
#define CMD_SETTLE 1000 

void executeCommand(char cmd) {
  byte pin = 0;
  char dumpType;
  switch (cmd) {
  case CMD_QUERY:
    dumpType = DUMP_QUERY;
    break;
  case CMD_ON_OFF:
    pin = 10;
    dumpType = DUMP_ON_OFF;
    break;
  case CMD_POWER:
    pin = 11;
    dumpType = DUMP_POWER;
    break;
  default:
    return;
  }  
  if (pin != 0) {
    pinMode(pin, OUTPUT);
    delay(CMD_TIMEOUT);
    pinMode(pin, INPUT);
    delay(CMD_SETTLE); // let it settle onto new state
  }
  makeDump(dumpType);
}

//------- STATE UPDATE -------

Timeout restoreStateTimeout(RESTORE_STATE_INTERVAL);
State prevState = STATE_OFF;

void restoreState() {
  State cur = getState();
  State cfg = config.state.read();
  if (cur != cfg && (cfg == STATE_SP || cfg == STATE_DP)) {
    if (cur == STATE_OFF)
      executeCommand(CMD_ON_OFF);
    cur = getState();
    if (cur == STATE_SP && cfg == STATE_DP || cur == STATE_DP && cfg == STATE_SP)
      executeCommand(CMD_POWER);
  }
}

void checkState() {
  State state = getState();
  if (state != prevState) {
    prevState = state;
    restoreStateTimeout.disable();
    if (state != STATE_KEEP)
      config.state = state;
    else if (config.state.read() == STATE_OFF)
      config.state = STATE_DP; // assume double power by default when in "keep"
  }
  if (restoreStateTimeout.check()) {
      if (getTemperature() < RESTORE_TEMP_LIMIT)
        restoreState();
      else
        restoreStateTimeout.reset(RESTORE_RECHECK_INTERVAL);
  }
}

//------- SETUP & MAIN -------

void setup() {
  setupPrint();
  waitPrintln("{B:Boiler started}*");
}

void loop() {
  blinkLed(1000);
  dumpState();
  executeCommand(parseCommand());
  checkState();
}
