#include <Arduino.h>

typedef struct CAN_telegram {
  uint32_t id;
  uint8_t len;
  uint8_t buf[8];
} CAN_telegram;

typedef struct OW_switch {
  uint8_t nick;
  uint8_t addr[8];
  uint8_t last_state;
//  CAN_telegram event[2];
} OW_switch;
