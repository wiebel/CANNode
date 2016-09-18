#include <Arduino.h>

// -------------------------------------------------------------
// CANNode for Teensy 3.1/3.2
//
// by wiebel
//

#include <Metro.h>
#include <FlexCAN.h>
#include <OneWire.h>
#include "CANNode.h"

// Configuration
#define NODE_ID 0x01
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



// Metro ticks in ms
#define METRO_CAN_tick 1
#define METRO_OW_read_tick 20
#define METRO_OW_search_tick 10000
// CAN bus
#define CAN_speed 125000
#define CAN_RS_PIN 2							//
// OneWire
#define OW_pin 14

#define TEST_ADDR "125B275000000026"

#define DS2406_FAMILY       0x12
#define DS2406_WRITE_STATUS 0x55
#define DS2406_READ_STATUS  0xaa
#define DS2406_CHANNEL_ACCESS  0xf5
#define DS2406_CHANNEL_CONTROL1 0x4c // 01001100 - Read Both Pins
#define DS2406_CHANNEL_CONTROL2 0xff // for future dev
#define SKIP_ROM 0xCC
#define PIO_A 0x20
#define PIO_B 0x40

#define DS2406_BUF_LEN 10
// Definitions
static uint8_t node_id PROGMEM= { NODE_ID };
static OW_switch_t switches[N_SWITCHES] PROGMEM={
//  nick, addr[8], event_tag[sw1, sw2]
 { 1, { 0x12, 0x5b, 0x27, 0x50, 0x0, 0x0, 0x0, 0x26 }, { 1, 10 } },
 { 0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }, { 0, 0 } }
};
static uint8_t switches_state[N_SWITCHES];

static outputs_t outputs[N_OUTPUTS] PROGMEM={
  { GPIO, 0},
  { GPIO, 1},
  { GPIO, 23},
  { GPIO, 22},
  { GPIO, 17},
  { GPIO, 16},
  { GPIO, 9},
  { GPIO, 10},
  { NOP, 0xFF}
};
static uint8_t outputs_state[N_OUTPUTS];
//  ID:
// 3 bit Prio, 2bit TYPE, 8bit DST, 8bit SRC, 8bit CMD/SEQ
// Type: 0: CMD, 1: FIRST, 2: CONT, 3: LAST
// CMD:
static event_t tx_events[N_EVENTS] PROGMEM={
// |    --- ID ---    |
// tag, prio, dst, cmd, data
{ 1, 0x03, 0xff, 0x03, 0x01},
{ 2, 0x03, 0xff, 0x03, 0x02},
{ 3, 0x03, 0xff, 0x03, 0x03},
{ 4, 0x03, 0xff, 0x03, 0x04},
{ 5, 0x03, 0xff, 0x03, 0x05},
{ 6, 0x03, 0xff, 0x03, 0x06},
{ 7, 0x03, 0xff, 0x03, 0x07},
{ 8, 0x03, 0xff, 0x03, 0x08},
{ 9, 0x03, 0xff, 0x03, 0x09},
{ 10, 0x03, 0xff, 0x03, 0x01},
{ 10, 0x03, 0xff, 0x03, 0x02},
{ 10, 0x03, 0xff, 0x03, 0x03},
{ 10, 0x03, 0xff, 0x03, 0x04},
{ 10, 0x03, 0xff, 0x03, 0x05},
{ 10, 0x03, 0xff, 0x03, 0x06},
{ 10, 0x03, 0xff, 0x03, 0x07},
{ 10, 0x03, 0xff, 0x03, 0x08},

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

// Initialisation
// Misc
#if DEBUG
  #define DEBUG_PRINT(a) Serial.print(a)
  #define DEBUG_PRINTLN(a) Serial.println(a)
  #define DEBUG_WRITE(a) Serial.write(a)
#else
  #define DEBUG_PRINT(a)
  #define DEBUG_PRINTLN(a)
  #define DEBUG_WRITE(a)
#endif /* DEBUG */

uint8_t led = LED;
uint8_t state;
uint8_t pin_state;
// Metro
Metro METRO_CAN = Metro(METRO_CAN_tick);
Metro METRO_OW_read = Metro(METRO_OW_read_tick);
Metro METRO_OW_search = Metro(METRO_OW_search_tick);
// CAN bus
FlexCAN CANbus(CAN_speed);
// OneWire
uint8_t addr[8];
uint8_t buffer[DS2406_BUF_LEN];
uint8_t readout,trig_event,event_idx,tmp;
OneWire OW_1(OW_pin);

telegram_comp_t mesg_comp;

static CAN_message_t txmsg,rxmsg;
static uint8_t hex[17] = "0123456789abcdef";

int txCount,rxCount;
unsigned int txTimer,rxTimer;


// Functions ------------------------------^-------------------------
void PrintBytes(uint8_t* addr, uint8_t count, bool newline=0) {
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(addr[i]>>4, HEX);
    Serial.print(addr[i]&0x0f, HEX);
  }
  if (newline)
    Serial.println();
}
void DEBUG_Bytes(uint8_t* addr, uint8_t count, bool newline=0) {
  #if DEBUG
    for (uint8_t i = 0; i < count; i++) {
      Serial.print(addr[i]>>4, HEX);
      Serial.print(addr[i]&0x0f, HEX);
  }
    if (newline)
      Serial.println();
  #endif
}
void print_OW_Device(uint8_t *addr) {
    for(int i = 0; i < 8; i++) {
        if(i != 0) {
            Serial.print(":");
        }
        Serial.print(addr[i], HEX);
    }
}
uint8_t read_DS2406(uint8_t* addr) {
	  OW_1.reset();
    OW_1.select(addr);
    OW_1.write(DS2406_CHANNEL_ACCESS,1);
    OW_1.write(DS2406_CHANNEL_CONTROL1,1);
    OW_1.write(DS2406_CHANNEL_CONTROL2,1);
    tmp = OW_1.read();
    OW_1.reset();
    #if DEBUG
	   PrintBytes(&tmp,1,1);
    #endif /* DEBUG */
    return tmp;
}
uint8_t toggle_Pin(uint8_t pin){
  digitalWrite(pin, !digitalRead(pin));
  return digitalRead(pin);
}

uint32_t forgeid(event_t event){
  return (event.prio<<26)+(event.dst<<16)+(NODE_ID<<8)+event.cmd;
}

telegram_comp_t parse_CAN(CAN_message_t mesg){
  telegram_comp_t tmp;
  tmp.prio=mesg.id>>26;
  tmp.frametype=(mesg.id>>24) & 0x03;
  tmp.dst=(mesg.id>>16) & 0xFF;
  tmp.src=(mesg.id>>8) & 0xFF;
  tmp.cmd=mesg.id & 0xFF;
  tmp.length=mesg.len;
  for (uint8_t i = 0; i < mesg.len; i++) { tmp.buf[i] = mesg.buf[i]; }
  return tmp;
}
void send_event (uint8_t trig_event){
        Serial.print(F("Sending event: "));
        Serial.println(trig_event);

  for (uint8_t e_idx = 0; tx_events[e_idx].tag != 0; e_idx++)
    if ( tx_events[e_idx].tag == trig_event ) {
      txmsg.id = forgeid(tx_events[e_idx]);
      txmsg.len = 1;
//      txmsg.timeout = 100;
      txmsg.buf[0]= tx_events[e_idx].data;
      CANbus.write(txmsg);
        Serial.print(F("Sending to CAN ID: "));
        Serial.print(txmsg.id);
        Serial.print(F("DATA: "));
        Serial.println(txmsg.buf[0]);
      txmsg.len = 0;
    }
  }
void take_action (action_type type, uint8_t tag ){
for (uint8_t i = 0; action_map[i].tag != 0 ; i++) {
  if ( action_map[i].tag == tag ) {
    switch ( type ) {
      case OFF:
        digitalWrite(outputs[action_map[i].outputs_idx].address,LOW);
        Serial.print(F("Switching OFF Output: "));
        Serial.println(action_map[i].outputs_idx);
        break;
      case ON:
        digitalWrite(outputs[action_map[i].outputs_idx].address,HIGH);
        Serial.print(F("Switching ON Output: "));
        Serial.println(action_map[i].outputs_idx);
        break;
      case TOGGLE:
        pin_state = toggle_Pin(outputs[action_map[i].outputs_idx].address);
        Serial.print(F("Toggeling Output: "));
        Serial.print(action_map[i].outputs_idx);
        Serial.print(F("to new state: "));
        Serial.println(pin_state);
        break;
      case VALUE:
        Serial.println(F("TBD"));
        break;
      }
    }
  }
}

// -------------------------------------------------------------
void setup(void)
{

  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);

  //CAN bus
  pinMode(CAN_RS_PIN,OUTPUT);
  digitalWrite(CAN_RS_PIN,0);
  CANbus.begin();
  txmsg.ext = 1;
  txmsg.timeout = 100;

  // outputs
  for (size_t i = 0; outputs[i].type != NOP; i++) {
    if (outputs[i].type == GPIO || outputs[i].type == PWM){
      pinMode(outputs[i].address, OUTPUT);
    }
  }

  delay(100);
  Serial.println(F("Hello Teensy 3.2 CANode awakes."));
  METRO_CAN.reset();
  METRO_OW_read.reset();
  METRO_OW_search.reset();
}


// -------------------------------------------------------------
void loop(void)
{
  // service software timers based on Metro tick
  if ( METRO_OW_search.check() ) {
    OW_1.reset_search();
    while(OW_1.search(addr) == 1) {
        if ( OneWire::crc8( addr, 7) != addr[7]) {
            Serial.print("CRC is not valid!\n");
            delay(10);
            return;
        }
        Serial.print("Found a device: ");
        print_OW_Device(addr);
        Serial.println("");
        }
  }
  if (METRO_OW_read.check() ) {
    bool action[2];
     readout = read_DS2406(switches[0].addr);
    if (switches_state[0] != readout) {
      tmp = readout ^ switches_state[0];
      switches_state[0] = readout;
      action[0] = tmp & 0x04;
      action[1] = tmp & 0x08;
    }
    if (action[0]) {
      Serial.println("pioA toggled");
      send_event(switches[0].event_tag[0]);
    }
    if (action[1]) {
      Serial.println("pioB toggled");
      send_event(switches[0].event_tag[1]);
    }
  }
  if ( METRO_CAN.check() ) {
    if ( txTimer ) {
      --txTimer;
    }
    if ( rxTimer ) {
      --rxTimer;
    }
  }
    while ( CANbus.read(rxmsg) ) {
      //hexDump( sizeof(rxmsg), (uint8_t *)&rxmsg );
      DEBUG_WRITE(rxmsg.buf[0]);
      rxCount++;
        Serial.print("GOT=");
        Serial.print(rxmsg.id,HEX);
        for (uint8_t i=0; i<rxmsg.len; i++){
          Serial.print(":");
          Serial.print(rxmsg.buf[i],HEX);
        }
        Serial.println("\n");
        mesg_comp= parse_CAN(rxmsg);
        if (mesg_comp.dst == 0xFF || mesg_comp.dst == NODE_ID ) {

        switch (mesg_comp.cmd) {
          // OFF
          case 0x00 :
            take_action(OFF, mesg_comp.buf[0]);
          break;
          // ON
          case 0x01 :
            take_action(ON, mesg_comp.buf[0]);
          break;
          // VALUE
          case 0x02 :
          break;
          // TOGGLE
          case 0x03:
            take_action(TOGGLE, mesg_comp.buf[0]);
          break;
        }
      }
      rxCount = 0;
    }
    txTimer = 100;//milliseconds
    if (txmsg.len != 0){
      Serial.print("PUT=");
      Serial.print(txmsg.id,HEX);
        for (uint8_t i=0; i<txmsg.len; i++){
          Serial.print(":");
          Serial.print(txmsg.buf[i],HEX);
        }
        Serial.print("\n");
      CANbus.write(txmsg);
      txmsg.buf[0]++;
      txmsg.len = 0;
    }

}
