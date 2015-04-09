#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <termios.h>
#include <ctype.h>

#include "protocol_impl.h"

#define STR_USAGE   \
  "usage:\n"        \
  "-h           - help\n"\
  "-p <port>    - portname (/dev/ttyUSB0, /dev/rfcomm0, ...)\n" \
  "-b <baud>    - baud rate (9600, 115200, ...)\n"\
  "-B <bt_addr> - bluetooth address (00:12:06:15:70:93, ...)"

#define DEFAULT_COMM_PORT "/dev/rfcomm0"
#define DEFAULT_BT_ADDR "00:12:06:15:70:93"
#define DEFAULT_BAUD B115200 // cfsetospeed macro definition

pthread_mutex_t lock;

struct opts {
  char * port;
  char * bt_addr;
  int baud;
} exec_arguments = {
  DEFAULT_COMM_PORT,
  DEFAULT_BT_ADDR,
  DEFAULT_BAUD
};

void get_exec_arguments(int argc, char ** argv)
{
  opterr = 0;
  int c;
  while ((c = getopt (argc, argv, "p:B:b:h")) != -1) {
    switch (c)
      {
      case 'p':
        exec_arguments.port = optarg;
        printf("opt port: %s\n", exec_arguments.port);
        break;
      case 'B':
      {
        const unsigned long long baud = strtoull(optarg, NULL, 10);
        if (baud == 9600)
          exec_arguments.baud = B9600;
        else if (baud == 38400)
          exec_arguments.baud = B38400;
        else if (baud == 115200)
          exec_arguments.baud = B115200;
        else
          fprintf(stderr, "illegal baud rate %llu\n", baud);
        printf("opt baud: %llu\n", (unsigned long long)exec_arguments.baud);
      }
        break;
      case 'b':
        exec_arguments.bt_addr = optarg;
        printf("opt bt_addr: %s\n", exec_arguments.bt_addr);
        break;
      case 'h':
        puts(STR_USAGE);
        exit(0);
      case '?':
        if (optopt == 'p' || optopt == 'b' || optopt == 'B')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
      default:
        puts(STR_USAGE);
        exit(1);
      }
  }

  printf("\nportname: %s\nbaudrate: %d\nbt_addr: %s\n\n",
         exec_arguments.port,
         exec_arguments.baud,
         exec_arguments.bt_addr);
}

// #################
// Periodically send pings.
// #################
void * sender_thread()
{
  int led = 0;
  int counter = 0;
  while (1) {
    protocol_send_cmd_ping(counter++, 0);
    if (counter % 20 == 0) // flash LED
      protocol_send_cmd_ctrl_led(led++ % 2);
    protocol_send_cmd_ctrl_dump('a');
    protocol_send_cmd_ctrl_dump('u');
    protocol_send_cmd_ctrl_led(led++ % 2);
    usleep(20000);
  }
  return NULL;
}

// #################
// Program exit handler.
// #################
void sigintkill(int sigint)
{
  pthread_mutex_destroy(&lock);
  if (fd)
    close (fd);
  exit(0);
}

int main(int argc, char ** argv)
{
  signal (SIGINT, sigintkill);
  signal (SIGSEGV, sigintkill);
  pthread_mutex_init(&lock, NULL);

  get_exec_arguments(argc, argv);

  if (-1 == select_comm(exec_arguments.port,
                        exec_arguments.baud,
                        exec_arguments.bt_addr))
  {
    fprintf (stderr, "error selecting commport\n");
    return 1;
  }

  pthread_t thread;
  if (pthread_create(&thread, NULL, sender_thread, NULL) < 0)
  {
    fprintf (stderr, "error creating thread %d\n", errno);
  }

  while (1) {
    protocol_handle_buf();
  }
  
  sigintkill(SIGINT);  
  return 0;
}
