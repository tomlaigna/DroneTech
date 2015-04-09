#ifndef _CTRL_H_
#define _CTRL_H_

#include <Arduino.h>
#include <Wire.h>

// ##########
// Interrupt routine stuff.
// ##########
void acc_data_ready();
boolean is_data_ready();

// #########
// Initialize ctrls to default.
// #########
void init_ctrl();

// #########
// ctrl update routine.
// #########
void update_ctrl();

// #########
// Get current user input control vector.
// #########
struct vector4f * get_U();

// #########
// Get current integrated correction vector.
// #########
struct vector4f * get_G();

// #########
// Update engine throttle status.
// #########
void update_Th();

// #########
// Update current throttle vector.
// #########
void update_G(const struct vector4f * corr_vec);

// #########
// Return raw accelerometer data in ints.
// #########
struct vector3i get_raw_acc();

// #########
// Return velocity computed from raw accelerometer data in ints.
// #########
struct vector3i get_raw_acc_vel(const struct vector3i * raw_acc);

// #########
// Compute the correction vector from raw accelerometer data.
// #########
struct vector4f compute_corr();

#endif
