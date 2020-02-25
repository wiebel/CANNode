#include "CANNode.h"

// Node 02 - Werkstatt
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
//  nick, addr[8], event_tag[pioA_FALL, pioA_RISE, pioB_FALL, pioB_RISE]
// { 255, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0, 0, 0 } },
 { 21, { 0x12, 0x86, 0xB4, 0x54, 0x0, 0x0, 0x0, 0x5F }, { 23, 23, 37, 37 } }, // Flur
 { 22, { 0x12, 0xCE, 0x6E, 0xCA, 0x0, 0x0, 0x0, 0x9C }, { 22, 22, 21, 21 } }, // WZ
// { 22, { 0x12, 0xC7, 0x2F, 0xCF, 0x0, 0x0, 0x0, 0xAF }, { 22, 22, 21, 21 } }, // WZ
// { 22, { 0x12, 0x5E, 0xFF, 0x55, 0x0, 0x0, 0x0, 0x2C }, { 21, 21, 22, 22 } }, // WZ
 { 0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0, 0, 0 } } // END MARK
};
static uint8_t switches_state[N_SWITCHES];
static outputs_t outputs[N_OUTPUTS] PROGMEM={
// type, address(PIN), initial value, inverted
  { GPIO, 18, 0, true },  	// 0
  { GPIO, 19, 0, true },   	// 1
  { GPIO, 16, 0, true },  	// 2
  { GPIO, 17, 0, true },  	// 3
  { GPIO, 22, 0, true },  	// 4
  { GPIO, 23, 0, true },  	// 5
  { GPIO, 1,  0, true },   	// 6
  { GPIO, 0,  0, true },   	// 7
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
{ 21, 0x03, 0x02, TOGGLE, 0x01},
{ 22, 0x03, 0x02, TOGGLE, 0x02},
{ 23, 0x03, 0x02, TOGGLE, 0x03},
{ 13, 0x03, 0x01, TOGGLE, 0x03},
{ 14, 0x03, 0x01, TOGGLE, 0x04},
{ 5, 0x03, 0x01, TOGGLE, 0x05},
{ 6, 0x03, 0x01, TOGGLE, 0x06},
{ 37, 0x03, 0x03, TOGGLE, 0x07},
{ 8, 0x03, 0x01, TOGGLE, 0x08},
{ 11, 0x03, 0x01, ON, 0x01},
{ 12, 0x03, 0x01, ON, 0x02},
{ 210, 0x03, 0xff, OFF, 0x06},
{ 211, 0x03, 0xff, ON, 0x06},
{ 220, 0x03, 0xff, OFF, 0x07},
{ 221, 0x03, 0xff, ON, 0x07},
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
  {210, 210},
  {211, 211},
  {220, 220},
  {221, 221},
};
