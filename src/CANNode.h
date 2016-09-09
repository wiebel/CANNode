#include <Arduino.h>

enum out_type { GPIO, PWM, OW, I2C, SPI, WS2811, DMX };
enum event_type { LOCAL, SEND};

enum action_type { OFF, ON, TOGGLE, VALUE };
enum telegram_type { ALERT, EVENT, NOTIFY, INFO };

typedef struct CAN_telegram_t {
  uint32_t id;
  uint8_t len;
  uint8_t buf[8];
} CAN_telegram_t;

typedef struct OW_switch_t {
  uint8_t nick;
  uint8_t addr[8];
  uint8_t event_tag[2];
} OW_switch_t;

typedef struct event_t {
  uint8_t tag;
  uint8_t prio;
  uint8_t dst;
  uint8_t cmd;
  uint8_t target_id;
//  uint8_t data[2];
//  CAN_telegram_t telegram;
} event_t;

typedef struct telegram_dict_t {
  CAN_telegram_t telegram;
  uint8_t tag;
} telegram_dict_ttelegram_dict_t;

typedef struct action_t {
  uint8_t id;
  uint8_t tag;
  uint8_t outputs_id;
} action_t;

typedef struct outputs_t {
  uint8_t id;
  out_type type;
  uint8_t address;
} outputs_t;
