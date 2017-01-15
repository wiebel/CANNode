// #include "CANNode.h"
// Configuration
#define NODE_ID 0x02

// Misc

#define N_EVENTS 64 							// size of events array
#define N_SWITCHES 64 						// size of switch array
#define N_OUTPUTS 64
#define N_ACTIONS 64

#define DEBUG 0										// 1 for noisy serial
#define LED 13

#define RELAY1 0
#define RELAY2 1
#define RELAY3 23
#define RELAY4 22
#define RELAY5 17
#define RELAY6 16
#define RELAY7 9
#define RELAY8 10

// OneWire
#define OW_pin 14

// Definitions
static uint8_t node_id PROGMEM= { NODE_ID };
static OW_switch_t switches[N_SWITCHES] PROGMEM={
//  nick, addr[8], event_tag[sw1, sw2]
 { 255, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0, 0, 0 } },
 { 0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0, 0, 0 } }
};
static uint8_t switches_state[N_SWITCHES];
static outputs_t outputs[N_OUTPUTS] PROGMEM={
// type, address(PIN), initial value, inverted
  { GPIO, 0,  0, true },   	// 0 
  { GPIO, 1,  0, true },   	// 1 
  { GPIO, 23, 255, true },  	// 2
  { GPIO, 22, 0, true },  	// 3
  { GPIO, 17, 0, true },  	// 4
  { GPIO, 16, 0, true },  	// 5
  { GPIO, 9,  255, true },   	// 6
  { GPIO, 10, 0, true },  	// 7
  { NOP, 0xFF, 0, 0 }
};
static uint8_t outputs_state[N_OUTPUTS];
//  ID:
// 3 bit Prio, 2bit TYPE, 8bit DST, 8bit SRC, 8bit CMD/SEQ
// Type: 0: CMD, 1: FIRST, 2: CONT, 3: LAST
// CMD:
static event_t tx_events[N_EVENTS] PROGMEM={
// |    --- ID ---    |
// tag, prio, dst, cmd, data
{ 1, 0x03, 0x01, OFF, 0x01},
{ 2, 0x03, 0x01, OFF, 0x02},
{ 3, 0x03, 0x01, TOGGLE, 0x03},
{ 4, 0x03, 0x01, TOGGLE, 0x04},
{ 5, 0x03, 0x01, TOGGLE, 0x05},
{ 6, 0x03, 0x01, TOGGLE, 0x06},
{ 7, 0x03, 0x01, TOGGLE, 0x07},
{ 8, 0x03, 0x01, TOGGLE, 0x08},
{ 11, 0x03, 0x01, ON, 0x01},
{ 12, 0x03, 0x01, ON, 0x02},
{ 255, 0x03, 0xff, OFF, 0x09},
{ 254, 0x03, 0xff, ON, 0x09},
{ 10, 0x03, 0xff, TOGGLE, 0x01},
{ 10, 0x03, 0xff, TOGGLE, 0x02},
{ 10, 0x03, 0xff, TOGGLE, 0x03},
{ 10, 0x03, 0xff, TOGGLE, 0x04},
{ 10, 0x03, 0xff, TOGGLE, 0x05},
{ 10, 0x03, 0xff, TOGGLE, 0x06},
{ 10, 0x03, 0xff, TOGGLE, 0x07},
{ 10, 0x03, 0xff, TOGGLE, 0x08},

//  { 0x01, 0x0CFF0103, 1, { 0xDE, 0xAD } },
//  { 0x01, 0x0102DEAD, 2, { 0xDE, 0xAD } },
//  { 0x02, 0x0204BEEF, 4, { 0xDE, 0xAD, 0xBE, 0xEF } }
{ 0, 0, 0, 0, 0}
};

static action_t action_map[N_ACTIONS] PROGMEM={
//  tag, output_idx
  {1, 0},
  {2, 1},
  {3, 2},
  {4, 3},
  {5, 4},
  {6, 5},
  {7, 6},
  {8, 7},
  {9, 0},{9, 1},{9, 2},{9, 3},{9, 4},{9, 5},{9, 6},{9, 7},
};
