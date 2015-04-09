#ifndef ARDUINO
#include <stdio.h>
#include <string.h>
#endif

// #########
// Command definitions.
// #########
#define MAGIC1 'a'
#define MAGIC2 'b'
#define MAGIC3 'c'
#define CMD_PING 'p'//0x01;
#define CMD_ERR 'e'
#define CMD_CTRL_USERCTRL 'u'//0x01;
#define CMD_CTRL_LED 'l'
#define CMD_CTRL_DUMP 0x01
#define CMD_CTRL_CALIBRATE_ESC 0x02
#define CMD_CTRL_SET_RTOPTS 0x03
#define CMD_DEBUG 'd'

// #########
// Buffer definitions.
// #########
static char in_buf[PROTOCOL_INBUF_LEN] = {0};
static uint16_t in_buf_counter = 0;

// #########
// Protocol header.
// #########
struct cmd_header
{
  int8_t magic1;
  int8_t magic2;
  int8_t magic3;
  int8_t cmd;
  uint16_t msg_len;
  uint16_t chksm;
};

// #########
// Handle cmd functions.
// #########
static int internal_handle_cmd_ping(const char * data, const uint16_t data_len);
static int internal_handle_cmd_err(const char * data, const uint16_t data_len);
static int internal_handle_cmd_ctrl_uctrl(const char * data, const uint16_t data_len);
static int internal_handle_cmd_ctrl_led(const char * data, const uint16_t data_len);
static int internal_handle_cmd_ctrl_dump(const char * data, const uint16_t data_len);
static int internal_handle_cmd_ctrl_calibrate_esc(const char * data, const uint16_t data_len);
static int internal_handle_cmd_ctrl_set_rtopts(const char * data, const uint16_t data_len);
static int internal_handle_cmd_debug(const char * data, const uint16_t data_len);

// #########
// Protocol functions.
// #########
static unsigned short get_header_chksum(struct cmd_header * head)
{
  unsigned short ret = 0;
  const unsigned short chksm = head->chksm;
  head->chksm = ~0;
  const char * ptr = (char*)head;
  uint16_t i;
  for (i = 0; i < sizeof (struct cmd_header); ++i) {
    ret	^= ~((int16_t)(ptr[i])) << 8;
    ret ^= (int16_t)(ptr[i]);
  }
  head->chksm = chksm;
  return ret;
}

static struct cmd_header make_header(const int8_t cmd, const uint16_t msg_len)
{
  struct cmd_header ret = {
    MAGIC1
    ,MAGIC2
    ,MAGIC3
    ,cmd
    ,msg_len
    ,get_header_chksum(&ret) };
  return ret;
}

static int match_header(struct cmd_header * head)
{
  return (head->magic1 == MAGIC1) && (head->magic2 == MAGIC2) && (head->magic3 == MAGIC3) &&
      (head->chksm == get_header_chksum(head)) &&
      (PROTOCOL_INBUF_LEN >= sizeof (struct cmd_header) + head->msg_len);
}

static void pop_buf(char * in_buf, uint16_t * in_buf_counter, const uint16_t i)
{
  memcpy(in_buf, in_buf + i, *in_buf_counter - i);
  *in_buf_counter -= i;
}

// #########
// Returns 0 if buffer contains a packet.
// #########
static int sniff_header(char * in_buf, uint16_t * in_buf_counter)
{
  struct cmd_header * head;
  uint16_t i;
  int8_t found_header = 0;
  if (*in_buf_counter < sizeof (struct cmd_header))
    return -1; // Header incomplete
  for (i = 0; i <= *in_buf_counter - (int)sizeof (struct cmd_header); ++i) {
    if (match_header((struct cmd_header *)(in_buf + i))) {
      found_header = 1;
      break;
    }
  }
  if (i)
    pop_buf(in_buf, in_buf_counter, i);
  if (found_header) {
    head = (struct cmd_header *)in_buf;
    if (*in_buf_counter < sizeof (struct cmd_header) + head->msg_len)
      return -1; // Msg not yet complete.
    else
      return 0; // Packet OK.
  }
  else
    return -2; // There was trash in the buffer
}

// #########
// Handle incoming buffer.
// #########
static void internal_handle_buf(char * in_buf, uint16_t * in_buf_counter)
{
  int ret;
  int handling_ok;
  while ((ret = sniff_header(in_buf, in_buf_counter)) == 0)
  {
    struct cmd_header * head = (struct cmd_header *)in_buf;
    switch (head->cmd)
    {
    case CMD_PING:
      handling_ok = internal_handle_cmd_ping(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_ERR:
      handling_ok = internal_handle_cmd_err(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_CTRL_USERCTRL:
      handling_ok = internal_handle_cmd_ctrl_uctrl(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_CTRL_LED:
      handling_ok = internal_handle_cmd_ctrl_led(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_CTRL_DUMP:
      handling_ok = internal_handle_cmd_ctrl_dump(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_CTRL_CALIBRATE_ESC:
      handling_ok = internal_handle_cmd_ctrl_calibrate_esc(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_CTRL_SET_RTOPTS:
      handling_ok = internal_handle_cmd_ctrl_set_rtopts(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    case CMD_DEBUG:
      handling_ok = internal_handle_cmd_debug(in_buf + sizeof (struct cmd_header), head->msg_len);
      break;
    default:
      handling_ok = 0;
    }
    pop_buf(in_buf, in_buf_counter, handling_ok ? sizeof (struct cmd_header) + head->msg_len : 3); // If not ok, eat just 3 bytes (size of header magic).
  }
  if (ret == -2) {
    // trash in buffer, should tell the other end to back off for some time.
  }
}

void protocol_handle_buf()
{
  int32_t to_read = protocol_get_available();
  if (to_read > PROTOCOL_INBUF_LEN - in_buf_counter)
    to_read = PROTOCOL_INBUF_LEN - in_buf_counter;
  in_buf_counter += protocol_read_data(in_buf + in_buf_counter, to_read);
  internal_handle_buf(in_buf, &in_buf_counter);
}

// #########
// Send data over rfcomm/serial
// #########
static int send_cmd(const struct cmd_header * head, const uint8_t * msg)
{
  int ret1 = protocol_write_data ((char *)head, sizeof (struct cmd_header));
  if (ret1 < 0) return ret1;
  int ret2 = protocol_write_data ((char *)msg, head->msg_len);
  if (ret2 < 0) return ret2;
  return ret1 + ret2;
}

int protocol_send_cmd_ping(const int16_t count, const int16_t echo)
{
  const struct cmd_header head = make_header(CMD_PING, sizeof(struct protocol_msg_ping));
  const struct protocol_msg_ping msg = {count, echo};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_err(const int8_t code, const char* data)
{
  const struct cmd_header head = make_header(CMD_ERR, sizeof(struct protocol_msg_err));
  struct protocol_msg_err msg = {code, {0}};
  snprintf(msg.dmp, 16, "%s", data);
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_uctrl(const int16_t x, const int16_t y, const int16_t z, const int16_t w)
{
  const struct cmd_header head = make_header(CMD_CTRL_USERCTRL, sizeof(struct protocol_msg_uctrl));
  const struct protocol_msg_uctrl msg = {x, y, z, w};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_ctrl_led(const int8_t value)
{
  const struct cmd_header head = make_header(CMD_CTRL_LED, sizeof(struct protocol_msg_led));
  const struct protocol_msg_led msg = {(char)value};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_ctrl_dump(const int8_t target)
{
  const struct cmd_header head = make_header(CMD_CTRL_DUMP, sizeof(struct protocol_msg_dump));
  const struct protocol_msg_dump msg = {(char)target};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_ctrl_calibrate_esc(const int8_t num, const int8_t pin, const uint16_t min_us, const uint16_t max_us, const float upvx, const float upvy, const float upvz)
{
  const struct cmd_header head = make_header(CMD_CTRL_CALIBRATE_ESC, sizeof(struct protocol_msg_calibrate_esc));
  const struct protocol_msg_calibrate_esc msg = {num, pin, min_us, max_us, upvx, upvy, upvz};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_ctrl_set_rtopts(const uint8_t auto_stabilize)
{
  const struct cmd_header head = make_header(CMD_CTRL_SET_RTOPTS, sizeof(struct protocol_msg_set_rtopts));
  const struct protocol_msg_set_rtopts msg = {auto_stabilize};
  return send_cmd(&head, (uint8_t*)&msg);
}

int protocol_send_cmd_debug(const char* data)
{
  const struct cmd_header head = make_header(CMD_DEBUG, strlen(data) + 1); // Throw in the \0
  return send_cmd(&head, (uint8_t*)data);
}

// #########
// Internal protocol handling
// #########
static int internal_handle_cmd_ping(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_ping* ping = (const struct protocol_msg_ping *)data;
  if (sizeof(*ping) != data_len) return 0;
#ifdef PROTOCOL_CMD_PING_HANDLER
  return PROTOCOL_CMD_PING_HANDLER(ping);
#else
  return 1;
#endif
}

static int internal_handle_cmd_err(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_err* err = (const struct protocol_msg_err *)data;
  if (sizeof(*err) != data_len) return 0;
#ifdef PROTOCOL_CMD_ERR_HANDLER
  return PROTOCOL_CMD_ERR_HANDLER(err);
#else
  return 1;
#endif
}

static int internal_handle_cmd_ctrl_uctrl(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_uctrl* uctrl = (const struct protocol_msg_uctrl *)data;
  if (sizeof(*uctrl) != data_len) return 0;
#ifdef PROTOCOL_CMD_CTRL_UCTRL_HANDLER
  return PROTOCOL_CMD_CTRL_UCTRL_HANDLER(uctrl);
#else
  return 1;
#endif
}

static int internal_handle_cmd_ctrl_led(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_led* led = (const struct protocol_msg_led *)data;
  if (sizeof(*led) != data_len) return 0;
#ifdef PROTOCOL_CMD_CTRL_LED_HANDLER
  return PROTOCOL_CMD_CTRL_LED_HANDLER(led);
#else
  return 1;
#endif
}

static int internal_handle_cmd_ctrl_dump(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_dump* dump = (const struct protocol_msg_dump *)data;
  if (sizeof(*dump) != data_len) return 0;
#ifdef PROTOCOL_CMD_CTRL_DUMP_HANDLER
  return PROTOCOL_CMD_CTRL_DUMP_HANDLER(dump);
#else
  return 1;
#endif
}

static int internal_handle_cmd_ctrl_calibrate_esc(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_calibrate_esc* esc = (const struct protocol_msg_calibrate_esc *)data;
  if (sizeof(*esc) != data_len) return 0;
#ifdef PROTOCOL_CMD_CTRL_CALIBRATE_ESC_HANDLER
  return PROTOCOL_CMD_CTRL_CALIBRATE_ESC_HANDLER(esc);
#else
  return 1;
#endif
}

static int internal_handle_cmd_ctrl_set_rtopts(const char * data, const uint16_t data_len)
{
  const struct protocol_msg_set_rtopts* opts = (const struct protocol_msg_set_rtopts*)data;
  if (sizeof(*opts) != data_len) return 0;
#ifdef PROTOCOL_CMD_CTRL_SET_RTOPTS_HANDLER
  return PROTOCOL_CMD_CTRL_SET_RTOPTS_HANDLER(opts);
#else
  return 1;
#endif
}
static int internal_handle_cmd_debug(const char * data, const uint16_t data_len)
{
  if (strlen(data) + 1 != data_len) return 0; // Data length counts in trailing 0
#ifdef PROTOCOL_CMD_DEBUG_HANDLER
  return PROTOCOL_CMD_DEBUG_HANDLER(data);
#else
  return 1;
#endif
}

