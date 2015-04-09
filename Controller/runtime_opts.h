#ifndef _RUNTIME_OPTS_
#define _RUNTIME_OPTS_

#include <Arduino.h>

// For bt rfcomm baud rate must be set explicitly via AT commands.

// #########
// Default runtime definitions.
// #########
#define DEFAULT_AUTO_STABILIZE true
#define DEFUALT_SERIAL_BAUD 115200

// #########
// Runtime opts structure
// #########
struct opts {
  boolean auto_stabilize; // false for raw user input.
  const uint32_t serial_baud;
};

#endif
