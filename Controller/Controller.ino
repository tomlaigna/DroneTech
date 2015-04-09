#include <Wire.h>
#include <string.h>
#include "vector3.h"
#include "ctrl.h"
#include "rfcomm.h"
#include "esc.h"

// #########
// Initialize systems.
// #########
void init_all_systems()
{
  init_rfcomm();
  init_ctrl();
  init_ESC();
}

// #########
// Actual runtime routines.
// #########
void update_all_systems()
{
  update_rfcomm();
  update_ctrl();
}

void setup(){
  init_all_systems();
}

void loop(){
  update_all_systems();
}
