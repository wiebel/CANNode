#include <Arduino.h>

typedef struct CAN_telegram_t {
  uint32_t id;
  uint8_t len;
  uint8_t buf[8];
} CAN_telegram_t;

typedef struct OW_switch_t {
  uint8_t nick;
  uint8_t addr[8];
  uint8_t state;
  uint8_t event_id[2];
} OW_switch_t;

typedef struct event_t {
  uint8_t id;
  CAN_telegram_t telegram;
} event_t;
