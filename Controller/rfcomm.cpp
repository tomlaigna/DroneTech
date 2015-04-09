#include "vector3.h"
#include "runtime_opts.h"
#include "rfcomm.h"
#include "ctrl.h"
#include "esc.h"

// #################
// First include protocol header, define handlers
// #################
#include "/home/tom/Work/DroneTech/Common/protocol.h"

// #########
// Definitions.
// #########
#define PROTOCOL_INBUF_LEN 256
#define PROTOCOL_OUTBUF_LEN 256
static char out_buf[PROTOCOL_OUTBUF_LEN] = {0};
extern struct opts global_runtime_opts;

// #########
// Handle cmd function declarations.
// #########
static int protocol_handle_cmd_ping(const struct protocol_msg_ping * ping);
static int protocol_handle_cmd_ctrl_uctrl(const struct protocol_msg_uctrl *uctrl);
static int protocol_handle_cmd_ctrl_led(const struct protocol_msg_led *led);
static int protocol_handle_cmd_ctrl_dump(const struct protocol_msg_dump * dump);
static int protocol_handle_cmd_ctrl_calibrate_esc(const struct protocol_msg_calibrate_esc * esc);
static int protocol_handle_cmd_ctrl_set_rtopts(const struct protocol_msg_set_rtopts *opts);
#define PROTOCOL_CMD_PING_HANDLER protocol_handle_cmd_ping
#define PROTOCOL_CMD_CTRL_UCTRL_HANDLER protocol_handle_cmd_ctrl_uctrl
#define PROTOCOL_CMD_CTRL_LED_HANDLER protocol_handle_cmd_ctrl_led
#define PROTOCOL_CMD_CTRL_DUMP_HANDLER protocol_handle_cmd_ctrl_dump
#define PROTOCOL_CMD_CTRL_CALIBRATE_ESC_HANDLER protocol_handle_cmd_ctrl_calibrate_esc
#define PROTOCOL_CMD_CTRL_SET_RTOPTS_HANDLER protocol_handle_cmd_ctrl_set_rtopts

// #################
// Then include protocol c file to be compiled into this unit
// #################
// For some unholy reason, this has to be a full path.
#include "/home/tom/Work/DroneTech/Common/protocol.c"

int32_t protocol_write_data(const char *src, const uint32_t size)
{
  return Serial.write((const uint8_t *)src, size);
}

int32_t protocol_read_data(char *dst, const uint32_t size)
{
  return Serial.readBytes(dst, size);
}

void init_rfcomm()
{
  Serial.begin(global_runtime_opts.serial_baud);
}

int32_t protocol_get_available()
{
  return Serial.available();
}

void update_rfcomm()
{
  protocol_handle_buf();
}

// #########
// Handle cmd function definitions.
// #########
static int protocol_handle_cmd_ping(const struct protocol_msg_ping* ping)
{
  protocol_send_cmd_ping(ping->count, 1);
  return 1;
}

static int protocol_handle_cmd_ctrl_uctrl(const struct protocol_msg_uctrl * uctrl)
{
  struct vector4f* user_ctrl = get_U();
  // Translate to the normalized control sphere
  user_ctrl->xf = uctrl->x;
  user_ctrl->yf = uctrl->y;
  user_ctrl->zf = uctrl->z;
  user_ctrl->wf = uctrl->w;
  return 1;
}

static int protocol_handle_cmd_ctrl_led(const struct protocol_msg_led * led)
{
  if (led->value) digitalWrite(13, HIGH);
  else digitalWrite(13, LOW);
  return 1;
}

static int protocol_handle_cmd_ctrl_dump(const struct protocol_msg_dump* dump)
{
  switch (dump->target)
  {
    case 'a':
    {
      const struct vector3i acc = get_raw_acc();
      snprintf(out_buf, PROTOCOL_OUTBUF_LEN, "acc x=%d,y=%d,z=%d", acc.xi, acc.yi, acc.zi);
    }
      break;
    case 'u':
    {
      const struct vector4f* uctrl = get_U();
      snprintf(out_buf, PROTOCOL_OUTBUF_LEN, "uctrl x=%.3f,y=%.3f,z=%.3f,w=%.3f",
               (double)uctrl->xf, (double)uctrl->yf, (double)uctrl->zf, (double)uctrl->wf);
    }
      break;
    default:
      return 0;
  }
  protocol_send_cmd_debug(out_buf);
  return 1;
}

static int protocol_handle_cmd_ctrl_calibrate_esc(const protocol_msg_calibrate_esc *esc)
{
  const struct vector3f upv = {esc->upvx, esc->upvy, esc->upvz};
  initialize_ESC(  (int)esc->num, (int)esc->pin,
                   esc->min_us, esc->max_us,
                   &upv  );
  snprintf(out_buf, PROTOCOL_OUTBUF_LEN, "cal ESC num=%i,pin=%i,min_us=%i,max_us=%i,upv={%.3f,%.3f,%.3f}",
           (int)esc->num, (int)esc->pin, esc->min_us, esc->max_us,
           (double)upv.xf, (double)upv.yf, (double)upv.zf);
  protocol_send_cmd_debug(out_buf);
  return 1;
}

static int protocol_handle_cmd_ctrl_set_rtopts(const struct protocol_msg_set_rtopts * opts)
{
  global_runtime_opts.auto_stabilize = opts->auto_stabilize;
  return 1;
}
