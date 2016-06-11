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
// Misc
#define N_EVENTS 64 							// size of events array
#define N_SWITCHES 64 						// size of switch array
#define DEBUG 0										// 1 for noisy serial
#define LED 13
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
static OW_switch_t switches[N_SWITCHES];
static event_t events[N_EVENTS];


// Initialisation
// Misc
#if DEBUG
  #define DEBUG_PRINT(a) Serial.print(a)
  #define DEBUG_WRITE(a) Serial.write(a)
#else
  #define DEBUG_PRINT(a)
  #define DEBUG_WRITE(a)
#endif /* DEBUG */

uint8_t led = LED;
uint8_t state;
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


static CAN_message_t txmsg,rxmsg;
static uint8_t hex[17] = "0123456789abcdef";

int txCount,rxCount;
unsigned int txTimer,rxTimer;


// Functions -------------------------------------------------------
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

// -------------------------------------------------------------
void setup(void)
{
  // Misc
	// Most clumsy way later to be put in eeprom or s/th
  switches[0].nick = 1;
  switches[0].addr[0] = 0x12,
  switches[0].addr[1] = 0x5b;
  switches[0].addr[2] = 0x27;
  switches[0].addr[3] = 0x50;
  switches[0].addr[4] = 0x0;
  switches[0].addr[5] = 0x0;
  switches[0].addr[6] = 0x0;
  switches[0].addr[7] = 0x26;
  switches[0].event_id[0] = 0x01;
  switches[0].event_id[1] = 0x02;

  events[0].id =0x01;
  events[0].telegram.id = 0x0102DEAD;
  events[0].telegram.len = 2;
  events[0].telegram.buf[0] = 0xDE;
  events[0].telegram.buf[1] = 0xAD;
  events[1].id =0x02;
  events[1].telegram.id = 0x0204BEEF;
  events[1].telegram.len = 4;
  events[1].telegram.buf[0] = 0xDE;
  events[1].telegram.buf[1] = 0xAD;
  events[1].telegram.buf[2] = 0xBE;
  events[1].telegram.buf[3] = 0xEF;

    //{ 0x02040608, 2, { 0xbe, 0xef }}};

  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);

  //CAN bus
  pinMode(CAN_RS_PIN,OUTPUT);
  digitalWrite(CAN_RS_PIN,0);
  CANbus.begin();
  txmsg.ext = 1;
  txmsg.timeout = 100;


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
            delay(100);
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
    if (switches[0].state != readout) {
      tmp = readout ^ switches[0].state;
      switches[0].state = readout;
      action[0] = tmp & 0x04;
      action[1] = tmp & 0x08;
    }
    if (action[0]) {
      Serial.print("pioA toggled");
      digitalWrite(led, !digitalRead(led));
      trig_event = switches[0].event_id[0];
      // might need queuing
      event_idx = 0;
    }
    if (action[1]) {
      Serial.print("pioB toggled");
      digitalWrite(led, !digitalRead(led));
      trig_event= switches[0].event_id[1];
      event_idx = 0;
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
  if ( txmsg.len == 0 && trig_event != 0 ) {
    if ( events[event_idx].id == trig_event ) {
      txmsg.id = events[event_idx].telegram.id;
      txmsg.len = events[event_idx].telegram.len;
      for (uint i=0;i<txmsg.len;i++){
        txmsg.buf[i] = events[event_idx].telegram.buf[i];
      }
    }
    if ( events[event_idx].id == 0) { trig_event = 0; }
    event_idx++;
  }
  // if not time-delayed, read CAN messages and print 1st byte
  if ( !rxTimer ) {
    while ( CANbus.read(rxmsg) ) {
      //hexDump( sizeof(rxmsg), (uint8_t *)&rxmsg );
      DEBUG_WRITE(rxmsg.buf[0]);
      rxCount++;
    }
  }

  // insert a time delay between transmissions
  if ( !txTimer ) {
    // if frames were received, print the count
    if ( rxCount ) {
      //#if DEBUG
      // Serial.write('=');
      //  Serial.print(rxCount);
        Serial.print("GOT=");
        Serial.print(rxmsg.id,HEX);
        for (uint8_t i=0; i<rxmsg.len; i++){
          Serial.print(":");
          Serial.print(rxmsg.buf[i],HEX);
        }
        Serial.print("\n");
        if (rxmsg.id ==0x0102DEAD) {
          state=toggle_Pin(led);
          txmsg.id=0xDEADBEEF;
          txmsg.len=2;
          txmsg.buf[0]=led;
          txmsg.buf[1]=state;
        }
        //#endif
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
    // digitalWrite(led, 0);
    // time delay to force some rx data queue use
    rxTimer = 3;//milliseconds
  }

}
