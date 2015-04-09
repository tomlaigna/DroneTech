#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <stdint.h>
#endif

// #########
// Command protocol data definitions.
// #########
struct protocol_msg_ping
{
  int16_t count;
  int16_t echo;
};
struct protocol_msg_err
{
  int8_t code;
  char dmp[16];
};
struct protocol_msg_uctrl
{
  int16_t x, y, z; // throttle
  int16_t w; // rotation
};
struct protocol_msg_led
{
  int8_t value;
};
struct protocol_msg_dump
{
  int8_t target;
};
struct protocol_msg_calibrate_esc
{
  int8_t num, pin;
  uint16_t min_us, max_us;
  float upvx, upvy, upvz;
};
struct protocol_msg_set_rtopts
{
  uint8_t auto_stabilize;
};

// #########
// Handle incoming packets.
// #########
void protocol_handle_buf();

// #########
// Send/read data over rfcomm/serial.
// #########
int32_t protocol_write_data(const char * src, const uint32_t size);
int32_t protocol_read_data(char * dst, const uint32_t size);
int32_t protocol_get_available();

int protocol_send_cmd_ping(const int16_t count, const int16_t echo);
int protocol_send_cmd_err(const int8_t code, const char* data);
int protocol_send_cmd_ctrl_uctrl(const int16_t x, const int16_t y, const int16_t z, const int16_t w);
int protocol_send_cmd_ctrl_led(const int8_t value);
int protocol_send_cmd_ctrl_dump(const int8_t target);
int protocol_send_cmd_ctrl_calibrate_esc(const int8_t num, const int8_t pin, const uint16_t min_us, const uint16_t max_us, const float upvx, const float upvy, const float upvz);
int protocol_send_cmd_ctrl_set_rtopts(const uint8_t auto_stabilize);
int protocol_send_cmd_debug(const char* data);

#endif
