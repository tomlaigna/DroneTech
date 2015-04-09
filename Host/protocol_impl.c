#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include "protocol_impl.h"

// #################
// Then include protocol c file to be compiled into this unit
// #################
#include "../Common/protocol.c"

// #################
// Global bt socket file descriptor for IO.
// #################
int fd = -1;
extern struct opts exec_arguments;

// #################
// Send command to fd.
// #################
int32_t protocol_write_data(const char *dst, const uint32_t size)
{
  return write(fd, dst, size);
}

int32_t protocol_read_data(char *src, const uint32_t size)
{
  return read(fd, src, size);
}

int32_t protocol_get_available()
{
  // Todo: use some poll or something.
  return 64;
}

int protocol_handle_cmd_ping(const struct protocol_msg_ping * ping) {
  return 1;
}

int protocol_handle_cmd_debug(const char * msg) {
  printf ("recv: %s\n", msg);
  return 1;
}

// #################
// Tries to open already open comm, else tries to acquire bt socket.
// #################
int select_comm(const char * portname, unsigned long long baud, const char * bt_addr)
{
  fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    printf ("error %d opening %s: %s\n", errno, portname, strerror (errno));
    if (errno == 2) { // Error no such file
      fd = get_rfcomm_socket(bt_addr, 1);
      if (fd < 0) {
        printf ("error %d opening rfcomm %s\n", errno, strerror (errno));
      }
    } else {
      return -1;
    }
  } else { // rfcomm already up
    set_interface_attribs (fd, baud, 0);// set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
  }
  return 0;
}

// #################
// BT socket initialization code.
// #################
int get_rfcomm_socket(const char* dest_addr, uint8_t channel)
{
  struct sockaddr_rc addr = { 0 };
  int s, status;
  //char dest[18] = "00:12:06:15:70:93";

  // allocate a socket
  s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  // set the connection parameters (who to connect to)
  addr.rc_family = AF_BLUETOOTH;
  addr.rc_channel = channel;
  str2ba( dest_addr, &addr.rc_bdaddr );

  // connect to server
  status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

  if( status < 0 ) perror("uh oh");

  return s;
}

// #################
// tty IO related code.
// #################
int set_interface_attribs(int fd, int speed, int parity)
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf ("error %d from tcgetattr", errno);
    return -1;
  }

  cfsetospeed (&tty, speed);
  cfsetispeed (&tty, speed);

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  tty.c_iflag &= ~IGNBRK;         // disable break processing
  tty.c_lflag = 0;                // no signaling chars, no echo,
                                  // no canonical processing
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;            // read doesn't block
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                  // enable reading
  tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
  tty.c_cflag |= parity;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
  {
    printf ("error %d from tcsetattr", errno);
    return -1;
  }
  return 0;
}

void set_blocking(int fd, int should_block)
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
  {
    printf ("error %d from tggetattr", errno);
    return;
  }

  tty.c_cc[VMIN]  = should_block ? 1 : 0;
  tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    printf ("error %d setting term attributes", errno);
}
