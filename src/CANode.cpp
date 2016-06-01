#include <Arduino.h>

// -------------------------------------------------------------
// CANtest for Teensy 3.1
// by teachop
//
// This test is talking to a single other echo-node on the bus.
// 6 frames are transmitted and rx frames are counted.
// Tx and rx are done in a way to force some driver buffering.
// Serial is used to print the ongoing status.
//

#include <Metro.h>
#include <FlexCAN.h>
#include <OneWire.h>
#include "CANode.h"

// Configurations
// Misc
#define DEBUG 0
#define LED 13
// Metro
#define METRO_CAN_tick 1 // 20 millisecond
#define METRO_OW_read_tick 20 // 20 millisecond
#define METRO_OW_search_tick 1000 // 1000 millisecond
// CAN bus
#define CAN_speed 125000
#define CAN_RS_PIN 2
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
static OW_switch_t switches;



// Initialisation
// Misc
#if DEBUG
  #define DEBUG_PRINT(a) Serial.print(a)
  #define DEBUG_WRITE(a) Serial.write(a)
#else
  #define DEBUG_PRINT(a)
  #define DEBUG_WRITE(a)
#endif /* DEBUG */

int led = LED;
// Metro
Metro METRO_CAN = Metro(METRO_CAN_tick);
Metro METRO_OW_read = Metro(METRO_OW_read_tick);
Metro METRO_OW_search = Metro(METRO_OW_search_tick);
// CAN bus
FlexCAN CANbus(CAN_speed);
// OneWire
uint8_t addr[8];
uint8_t buffer[DS2406_BUF_LEN];
uint8_t readout,tmp;
OneWire OW_1(OW_pin);




static CAN_message_t msg,rxmsg;
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
    // We're using a temporary switch object here only because we're
    // discovering them dynamically.  Depending on your app, you may
    // already know the serial number of the device you intend to
    // control and will use it there.
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
// -------------------------------------------------------------
void setup(void)
{
  // Misc
  switches.nick = 1;
  switches.addr[0] = 0x12,
  switches.addr[1] = 0x5b;
  switches.addr[2] = 0x27;
  switches.addr[3] = 0x50;
  switches.addr[4] = 0x0;
  switches.addr[5] = 0x0;
  switches.addr[6] = 0x0;
  switches.addr[7] = 0x26;
  switches.state = 0x60;
  //my_sw.event[0] =  { 0x01020304, 2, { 0xde, 0xad }};

    //{ 0x02040608, 2, { 0xbe, 0xef }}};

  Serial.begin(115200);
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);

  //CAN bus
  pinMode(CAN_RS_PIN,OUTPUT);
  digitalWrite(CAN_RS_PIN,0);
  CANbus.begin();

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
    //Serial.print("Beginning Search...\n");
    OW_1.reset_search();
    // Assume we can't find something until we prove otherwise.
//    bool foundDevice = false;

    while(OW_1.search(addr) == 1) {
        if ( OneWire::crc8( addr, 7) != addr[7]) {
            Serial.print("CRC is not valid!\n");
            delay(100);
            return;
        }

        Serial.print("Found a device: ");
        print_OW_Device(addr);
        Serial.println("");


        // At this point, we know we have a DS2406.  Blink it.
        //if(addr[0] == DS2406_FAMILY) {
//            foundDevice = true;
            // digitalWrite(13, HIGH);
//            tmp = read_DS2406(addr);
//          bool pioA = tmp & 0x04;
//            bool pioB = tmp & 0x08;
//            digitalWrite(led, pioA)i;
//          }
        }

  }
  if (METRO_OW_read.check() ) {
    bool action[2];
     readout = read_DS2406(switches.addr);
    if (switches.state != readout) {
      tmp = readout ^ switches.state;
      switches.state = readout;
      action[0] = tmp & 0x04;
      action[1] = tmp & 0x08;
    }
    if (action[0]) { Serial.print("pioA toggled"); digitalWrite(led, !digitalRead(led));}
    if (action[1]) { Serial.print("pioB toggled"); digitalWrite(led, !digitalRead(led));}
  }



  if ( METRO_CAN.check() ) {
    if ( txTimer ) {
      --txTimer;
    }
    if ( rxTimer ) {
      --rxTimer;
    }
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
      #if DEBUG
        Serial.write('=');
        Serial.print(rxCount);
        Serial.print(",Hello=");
        Serial.print(rxmsg.id,HEX);
      #endif
      rxCount = 0;
    }
    txTimer = 100;//milliseconds
    msg.len = 8;
    msg.id = 0x222;
    for( int idx=0; idx<8; ++idx ) {
      msg.buf[idx] = '0'+idx;
    }
    // send 6 at a time to force tx buffering
    txCount = 16;
    //digitalWrite(led, 1);
//    Serial.println(".");
    while ( txCount-- ) {
      CANbus.write(msg);
      msg.buf[0]++;
    }
    // digitalWrite(led, 0);
    // time delay to force some rx data queue use
    rxTimer = 3;//milliseconds
  }

}
