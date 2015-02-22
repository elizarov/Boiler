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
char dumpLine[] = "[B:0 t00.0 c0;u00000000](v0.0a0000b0000d0000)* ";

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
POSITIONS('t', ' ', tPos, tSize)
POSITIONS('c', ';', cPos, cSize)
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
#define DUMP_CHANGED               'c'
#define DUMP_RESTORED              'r'
#define DUMP_OFF                   '0'
#define DUMP_SP                    '1'
#define DUMP_DP                    '2'
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

void checkUpdateState(bool force);

const long CMD_TIMEOUT = 300; // how long to push button 
const long CMD_SETTLE = 1600; // state updates ~2 times perscond, plus one change is skipped

void executeHardwareCommand(char cmd) {
  byte pin;
  switch (cmd) {
  case CMD_ON_OFF:
    pin = 10;
    break;
  case CMD_POWER:
    pin = 11;
    break;
  default:
    return;
  }  
  pinMode(pin, OUTPUT);
  delay(CMD_TIMEOUT);
  pinMode(pin, INPUT);
  delay(CMD_SETTLE); // let it settle onto new state
  // forced update of configured state
  checkUpdateState(true); // force update of configured stat
}

bool changeState(State to) {
  State cur = getState();
  if (cur == to)
    return false;
  if (cur == STATE_OFF || to == STATE_OFF)
    executeHardwareCommand(CMD_ON_OFF);
  cur = getState();
  if (cur == STATE_SP && to == STATE_DP || cur == STATE_DP && to == STATE_SP)
    executeHardwareCommand(CMD_POWER);
  return true;  
}

void executeCommand(char cmd) {
  char dumpType;
  switch (cmd) {
  case CMD_QUERY:
    dumpType = DUMP_QUERY;
    break;
  case CMD_ON_OFF:
    executeHardwareCommand(CMD_ON_OFF);
    dumpType = DUMP_ON_OFF;
    break;
  case CMD_POWER:
    executeHardwareCommand(CMD_POWER);
    dumpType = DUMP_POWER;
    break;
  case CMD_OFF:
    config.state = STATE_OFF;
    changeState(STATE_OFF);
    dumpType = DUMP_OFF;
    break;    
  case CMD_SP:
    config.state = STATE_SP;
    changeState(STATE_SP);
    dumpType = DUMP_SP;
    break;    
  case CMD_DP:
    config.state = STATE_DP;
    changeState(STATE_DP);
    dumpType = DUMP_DP;
    break;    
  default:
    return;
  }  
  makeDump(dumpType);
}

//------- STATE UPDATE -------

Timeout restoreStateTimeout(RESTORE_STATE_INTERVAL);
State prevState = STATE_OFF;

void restoreState() {
  State cfg = config.state.read();
  if (cfg == STATE_SP || cfg == STATE_DP)
    if (changeState(cfg))
      makeDump(DUMP_RESTORED);
}

void checkUpdateState(bool force) {
  State state = getState();
  if (state == prevState)
    return;
  prevState = state;
  restoreStateTimeout.disable();
  // special logic to figure out "keep" state (don't know if DP or SP)
  if (state == STATE_KEEP) {
    if (config.state.read() != STATE_OFF)
      return; // some other state is configured -- don't touch it
    state = STATE_DP; // assume double power by default when in "keep"
  }  
  if (state != config.state.read()) {
    // new state if different from configured -- update configuration
    config.state = state;
    if (!force)
      makeDump(DUMP_CHANGED);
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
  checkUpdateState(false);
  checkRestoreState();
}
