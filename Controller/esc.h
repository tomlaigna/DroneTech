#ifndef _ESC_H_
#define _ESC_H_

#include "vector3.h"

// #########
// Definitions.
// #########
#define ESC_COUNT 4

// #########
// ESC control structure.
// #########
struct ESC
{
  uint16_t min_us; // min microseconds to write for 0% power.
  uint16_t max_us; // max microseconds to write for 100% power.
  uint16_t last_us; // last written value.
  int pin; // ESC control pin.
  struct vector3f upv; // Unit position vector for the throttle physical position.
};

// #########
// Initialize ESC to default.
// #########
void init_ESC();

// #########
// ESC data.
// #########
struct ESC * get_ESC(int num);

// #########
// Initialize ESC data.
// #########
void initialize_ESC(int num, int pin, uint16_t min_us, uint16_t max_us, const struct vector3f * unit_pos_vec);

// #########
// Apply actual throttle status to ESC.
// #########
void apply_throttle_ESC(float val, struct ESC * esc);

#endif
