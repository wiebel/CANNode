#include <Arduino.h>
#ifndef CANNODE_TYPES
#define CANNODE_TYPES
enum out_type { GPIO, PWM, OW, I2C, SPI, WS2811, DMX, NOP };
enum event_type { LOCAL, SEND};

enum action_type { OFF, ON, VALUE, TOGGLE };
enum telegram_type { ALERT, EVENT, NOTIFY, INFO };

typedef struct CAN_telegram_t {
  uint32_t id;
  uint8_t length;
  uint8_t buf[8];
} CAN_telegram_t;

typedef struct telegram_comp_t {
  uint8_t prio;
  uint8_t frametype;
  uint8_t dst;
  uint8_t src;
  uint8_t cmd;
  uint8_t length;
  uint8_t buf[8];
} telegram_comp_t;

typedef struct OW_switch_t {
  uint8_t nick;
  uint8_t addr[8];
  uint8_t event_tag[4];
} OW_switch_t;

typedef struct event_t {
  uint8_t tag;
  uint8_t prio;
  uint8_t dst;
  uint8_t cmd;
  uint8_t data;
//  uint8_t data[2];
//  CAN_telegram_t telegram;
} event_t;

typedef struct action_t {
  uint8_t tag;
  uint8_t outputs_idx;
} action_t;

typedef struct outputs_t {
  out_type type;
  uint8_t address;
  uint8_t init;
  bool invert;
} outputs_t;
#endif
