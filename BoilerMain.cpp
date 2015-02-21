#include <Timeout.h>
#include <FixNum.h>
#include <BlinkLed.h>

#include "xprint.h"
#include "parse.h"
#include "state_hal.h"
#include "Config.h"

BlinkLed blinkLed(13);

//------- ALL TIME DEFS ------

const long INITIAL_DUMP_INTERVAL = 2 * Timeout::SECOND;
const long PERIODIC_DUMP_INTERVAL = Timeout::MINUTE;
const long PERIODIC_DUMP_SKEW = 5 * Timeout::SECOND;

const long RESTORE_STATE_INTERVAL = Timeout::MINUTE;        // restore after 1 min
const long RESTORE_RECHECK_INTERVAL = 10 * Timeout::SECOND; // recheck every 10 sec if above 60
const int RESTORE_TEMP_LIMIT = 60;                          // restore only below 60 degrees C

//------- DUMP STATE -------

#define HIGHLIGHT_CHAR '*'

boolean firstDump = true; 
Timeout dump(INITIAL_DUMP_INTERVAL);
char dumpLine[] = "[B:0 t00.0;u00000000](c0v0.0a0000b0000d0000)* ";

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
POSITIONS('c', 'v', cPos, cSize)
POSITIONS('v', 'a', vPos, vSize)
POSITIONS('a', 'b', aPos, aSize)
POSITIONS('b', 'd', bPos, bSize)
POSITIONS('d', ')', dPos, dSize)
POSITIONS('u', ']', uptimePos, uptimeSize)

long daystart = 0;
int updays = 0;

inline void prepareDecimal(int x, int pos, byte size, fmt_t fmt = FMT_NONE) {
  formatDecimal(x, &dumpLine[pos], size, fmt);
}

template<typename T, prec_t prec> inline void prepareDecimal(FixNum<T,prec> x, int pos, uint8_t size, fmt_t fmt = (fmt_t)prec) {
  x.format(&dumpLine[pos], size, fmt);
}

#define DUMP_REGULAR               0
#define DUMP_FIRST                 HIGHLIGHT_CHAR
#define DUMP_ON_OFF                'o'
#define DUMP_POWER                 'p'
#define DUMP_QUERY                 '?'

void makeDump(char dumpType) {
  prepareDecimal(getState(), sPos, sSize);
  prepareDecimal(getTemperature(), tPos, tSize);
  
  // prepare other values
  prepareDecimal(config.state.read(), cPos, cSize);
  prepareDecimal(getMinVoltage(), vPos, vSize);
  prepareDecimal(h0, aPos, aSize);
  prepareDecimal(h1, bPos, bSize);
  prepareDecimal(h3, dPos, dSize);

  // prepare uptime
  long time = millis();
  while ((time - daystart) > Timeout::DAY) {
    daystart += Timeout::DAY;
    updays++;
  }
  prepareDecimal(updays, uptimePos, uptimeSize - 6, FMT_ZERO);
  time -= daystart;
  time /= 1000; // convert seconds
  prepareDecimal(time % 60, uptimePos + uptimeSize - 2, 2, FMT_ZERO);
  time /= 60; // minutes
  prepareDecimal(time % 60, uptimePos + uptimeSize - 4, 2, FMT_ZERO);
  time /= 60; // hours
  prepareDecimal((int) time, uptimePos + uptimeSize - 6, 2, FMT_ZERO);

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

void checkUpdateState();

const long CMD_TIMEOUT = 300; // how long to push button 
const long CMD_SETTLE = 1000; // state updates every 0.5s, so after 1s it definitely settles

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
  // forced update of configured state
  checkUpdateState(); // force update of configured stat
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

void checkUpdateState() {
  State state = getState();
  if (state != prevState) {
    prevState = state;
    restoreStateTimeout.disable();
    if (state != STATE_KEEP)
      config.state = state;
    else if (config.state.read() == STATE_OFF)
      config.state = STATE_DP; // assume double power by default when in "keep"
  }
}

void checkRestoreState() {
  if (restoreStateTimeout.check()) {
      if (getTemperature() < RESTORE_TEMP_LIMIT)
        restoreState();
      else
        restoreStateTimeout.reset(RESTORE_RECHECK_INTERVAL);
  }
}

//------- SETUP & MAIN -------

void setup() {
  setupStateHal();
  setupPrint();
  waitPrintln("{B:Boiler started}*");
}

void loop() {
  blinkLed.blink(1000);
  dumpState();
  executeCommand(parseCommand());
  checkUpdateState();
  checkRestoreState();
}
