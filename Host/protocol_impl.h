#ifndef _PROTOCOL_IMPL_H_
#define _PROTOCOL_IMPL_H_

// #################
// First include protocol header, define handlers
// #################
#include "../Common/protocol.h"

// #########
// Handle cmd function declarations.
// #########
int protocol_handle_cmd_ping(const struct protocol_msg_ping * ping);
int protocol_handle_cmd_debug(const char * msg);
#define PROTOCOL_CMD_PING_HANDLER protocol_handle_cmd_ping
#define PROTOCOL_CMD_DEBUG_HANDLER protocol_handle_cmd_debug

// #########
// Definitions.
// #########
#define PROTOCOL_INBUF_LEN 512
extern int fd;

int select_comm(const char * portname, int baud, const char *bt_addr);
int get_rfcomm_socket(const char* dest_addr, uint8_t channel);
int set_interface_attribs(int fd, int speed, int parity);
void set_blocking(int fd, int should_block);

#endif
