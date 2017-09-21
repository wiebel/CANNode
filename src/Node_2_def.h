#include "CANNode.h"
// Configuration
#define NODE_ID 0x02

// Misc

#define N_EVENTS 64 							// size of events array
#define N_SWITCHES 64 						// size of switch array
#define N_OUTPUTS 64
#define N_ACTIONS 64

#define DEBUG 0										// 1 for noisy serial
#define ONBOARD_LED 13

#define LED_PIN     20
#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define NUM_LEDS    4

#define BRIGHTNESS  255
//#define TEMP Tungsten100W
#define TEMP Candle

// OneWire
#define OW_pin 14

// Definitions
static uint8_t node_id PROGMEM= { NODE_ID };
static OW_switch_t switches[N_SWITCHES] PROGMEM={
//  nick, addr[8], event_tag[pioA_FALL, pioA_RISE, pioB_FALL, pioB_RISE]
 { 255, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0, 0, 0 } },
 { 11, { 0x12, 0xF2, 0x2A, 0x66, 0x0, 0x0, 0x0, 0x41 }, { 1, 1, 1, 1 } },
 { 12, { 0x12, 0x37, 0x8A, 0x4F, 0x0, 0x0, 0x0, 0xE5 }, { 2, 2, 2, 2 } },
 { 21, { 0x12, 0x86, 0xB4, 0x54, 0x0, 0x0, 0x0, 0x5F }, { 3, 3, 4, 4 } },
 { 22, { 0x12, 0x84, 0xAD, 0x4F, 0x0, 0x0, 0x0, 0x12 }, { 5, 5, 6, 6 } },
 { 31, { 0x12, 0x88, 0xDD, 0x53, 0x0, 0x0, 0x0, 0x28 }, { 210, 211, 220, 221 } }, // Roldenschalter (A - UP, B - DOWN)
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
  { GPIO, 19,  255, true },   	// 6
  { GPIO, 18, 0, true },  	// 7
  { LED, 21, 0, false },  	// 8
  { NOP, 0xFF, 0, 0 },
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
  {10, 8},
  {210, 210},
  {211, 211},
  {220, 220},
  {221, 221},
};
