#include <Arduino.h>
#include "esc.h"
#include "ctrl.h"

void init_ESC()
{
  const struct vector3f th1 = { 0.707f, 0.707f, 0.0f };
  const struct vector3f th2 = { 0.707f, -0.707f, 0.0f };
  const struct vector3f th3 = { -0.707f, -0.707f, 0.0f };
  const struct vector3f th4 = { -0.707f, 0.707f, 0.0f };
  initialize_ESC(0, 0, 512, 1024, &th1);
  initialize_ESC(1, 1, 512, 1024, &th2);
  initialize_ESC(2, 2, 512, 1024, &th3);
  initialize_ESC(3, 3, 512, 1024, &th4);
}

struct ESC * get_ESC(int num)
{
  static ESC esc[ESC_COUNT] = { 0 };
  return &esc[num];
}

void initialize_ESC(int num, int pin, uint16_t min_us, uint16_t max_us, const struct vector3f * unit_pos_vec)
{
  struct ESC * esc = get_ESC(num);
  esc->min_us = min_us; esc->max_us = max_us; esc->pin = pin;
  esc->upv = *unit_pos_vec;
}

void apply_throttle_ESC(float val, struct ESC * esc)
{
  val = constrain (val, 0.0f, 1.0f);
  val *= (esc->max_us - esc->min_us);
  val += esc->min_us;
  int set_time_us = (int)val;
  // Actually apply throttle value.
  // ...
  esc->last_us = set_time_us;
}
