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

// For Node definition:

#include "Node_3_def.h"

// Metro ticks in ms
#define METRO_CAN_tick 1
#define METRO_OW_read_tick 20
#define METRO_OW_search_tick 10000
// CAN bus
//#define CAN_speed 125000
#define CAN_speed 125000
#define CAN_RS_PIN 2							//
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
bool action[2];
// Metro
Metro METRO_CAN = Metro(METRO_CAN_tick);
Metro METRO_OW_read = Metro(METRO_OW_read_tick);
Metro METRO_OW_search = Metro(METRO_OW_search_tick);
// CAN bus
FlexCAN CANbus(CAN_speed);
// FLEXCAN0_MCR &= ~FLEXCAN_MCR_SRX_DIS;
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
    //OW_1.reset();
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
        digitalWrite(outputs[action_map[i].outputs_idx].address,LOW ^ outputs[action_map[i].outputs_idx].invert);
        Serial.print(F("Switching OFF Output: "));
        Serial.println(action_map[i].outputs_idx);
        break;
      case ON:
        digitalWrite(outputs[action_map[i].outputs_idx].address, HIGH ^ outputs[action_map[i].outputs_idx].invert);
        Serial.print(F("Switching ON Output: "));
        Serial.println(action_map[i].outputs_idx);
        break;
      case TOGGLE:
        pin_state = toggle_Pin(outputs[action_map[i].outputs_idx].address);
        Serial.print(F("Toggeling Output: "));
        Serial.print(action_map[i].outputs_idx);
        Serial.print(F(" to new state: "));
        Serial.println(pin_state ^ outputs[action_map[i].outputs_idx].invert);
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
  OW_1.reset_search();

  //CAN bus
  pinMode(CAN_RS_PIN,OUTPUT);
  digitalWrite(CAN_RS_PIN,0);
  CANbus.begin();
  txmsg.ext = 1;
  txmsg.timeout = 100;

  // outputs
  for (size_t i = 0; outputs[i].type != NOP; i++) {
    if (outputs[i].type == GPIO || outputs[i].type == PWM){
      //digitalWrite(outputs[i].address, LOW ^ outputs[action_map[i].outputs_idx].invert);
      pinMode(outputs[i].address, OUTPUT);
      if (outputs[i].init != 0 ) {
        digitalWrite(outputs[i].address, HIGH ^ outputs[i].invert );
      }
      else {
        digitalWrite(outputs[i].address, LOW ^ outputs[i].invert );

      }
    }
  }

for (uint8_t s_idx = 0; switches[s_idx].nick != 0; ++s_idx ){
   switches_state[s_idx] = read_DS2406(switches[s_idx].addr);
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
        bool new_owd=1;
        for (uint8_t s_idx = 0; switches[s_idx].nick != 0; ++s_idx ){
          bool cmp = 1;
          for ( uint8_t i = 0; i < 8; i++){
            if ( switches[s_idx].addr[i] != addr[i] ){
              cmp = 0;
              break;
            }
          }
            if ( cmp ) {
              new_owd=0;
              break;
            }
          }
        if (new_owd) {
          Serial.print("Found a device: ");
          print_OW_Device(addr);
          Serial.println();
        }

      }
  }
  if (METRO_OW_read.check() ) {
    for (uint8_t s_idx = 0; switches[s_idx].nick != 0; ++s_idx ){
      readout = read_DS2406(switches[s_idx].addr);
      //if ((switches_state[s_idx] != readout) && (readout & 1<<7)) {
      if (switches_state[s_idx] != readout) {
        tmp = readout ^ switches_state[s_idx];
        switches_state[s_idx] = readout;
        action[0] = tmp & 0x08;
        action[1] = tmp & 0x04;
      }
      if (action[0]) {
        Serial.print("pioA of switch ");
        Serial.print(switches[s_idx].nick);
        if (readout & 0x08) {
          Serial.println(F(" is now OFF"));
          send_event(switches[s_idx].event_tag[0]);
        } else {
          Serial.println(F(" is now ON"));
          send_event(switches[s_idx].event_tag[1]);
        }
        action[0] = 0;
      }
      if (action[1]) {
        Serial.print("pioB of switch ");
        Serial.print(switches[s_idx].nick);
        if (readout & 0x04) {
          Serial.println(F(" is now OFF"));
          send_event(switches[s_idx].event_tag[2]);
        } else {
          Serial.println(F(" is now ON"));
          send_event(switches[s_idx].event_tag[3]);
        }
        action[1] = 0;
      }
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
        Serial.println();
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
