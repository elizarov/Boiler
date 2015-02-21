#ifndef PARSE_H_
#define PARSE_H_

// query
#define CMD_QUERY  '?'
// push buttons
#define CMD_ON_OFF 'O'
#define CMD_POWER  'P'
// switch to defined state
#define CMD_OFF '0'
#define CMD_SP  '1'
#define CMD_DP  '2'

char parseCommand();

#endif /* PARSE_H_ */
