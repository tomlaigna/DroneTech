#include "vector3.h"
#include "runtime_opts.h"
#include "ctrl.h"
#include "esc.h"

// #########
// Definitions.
// #########
static const int MPU=0x68;  // I2C address of the MPU-6050
static const int SAMPLE_RATE=50;
static const int GRAVITY=16384;
static const float ALPHA=10.0f / (float)GRAVITY / (float)SAMPLE_RATE;
static const float BETA=1.0f / (float)GRAVITY;
static const float DAMPING=0.9f;
static const struct vector3i Ac0 = {0, 0, GRAVITY}; // Stationary acceleration.
static boolean data_ready = false;
extern struct opts global_runtime_opts;

void acc_data_ready()
{
  data_ready = true;
}

boolean is_data_ready()
{
  return data_ready;
}

static void setup_wire()
{
  Wire.begin();
}

static void send_wire(const int16_t addr, const int16_t reg, const int16_t data)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);  // register
  Wire.write(data); // data
  Wire.endTransmission(true);
}

static void send_wire(const int16_t addr, const int16_t reg)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);  // register
  Wire.endTransmission(true);
}

static void setup_acc()
{
  // The MPU will tell us when its data is ready for reading.
  attachInterrupt(0, &acc_data_ready, FALLING);
  send_wire(MPU,
            0x6B, // PWR_MGMT_1 register
            0);   // set to zero (wakes up the MPU-6050)
}

void init_ctrl()
{
  setup_wire();
  setup_acc();
}

void update_ctrl()
{
  struct vector4f ctrl_vec = *get_U();
  if (global_runtime_opts.auto_stabilize) {
    if (is_data_ready()) {
      const struct vector4f corr_vec = compute_corr();
      add_vec(&ctrl_vec, &corr_vec);
    }
  }
  update_G(&ctrl_vec);
  update_Th();
}

struct vector4f * get_U()
{
  static struct vector4f U = {0.0f,0.0f,0.0f,0.0f};
  return &U;
}

struct vector4f * get_G()
{
  static struct vector4f G = {0.0f,0.0f,0.0f,0.0f};
  return &G;
}

void update_Th()
{
  float E[ESC_COUNT] = { 0 };
  static const struct vector4f* G = get_G();
  
  // Todo - add angular correction
  const struct vector3f Throttle = {G->xf, G->yf, G->zf < 0 ? 0.0f : G->zf};
  for (int i = 0; i < ESC_COUNT; ++i) {
    struct ESC * curr_esc = get_ESC(i);
    E[i] = get_mod(&Throttle) * (1 - sin(M_PI/2 - get_proj_angle(&Throttle, &curr_esc->upv))) / 2;
    apply_throttle_ESC(E[i], curr_esc);
  }
}

void update_G(const struct vector4f * corr_vec)
{
  struct vector4f* G = get_G();
  mul_vec(G, DAMPING);
  add_vec(G, corr_vec);
  constrain_vec(G, 2.0f);
}

struct vector3i get_raw_acc()
{
  struct vector3i ret;
  send_wire(MPU,
            0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.requestFrom(MPU,6,true);  // request a total of 6 registers
  ret.xi=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  ret.yi=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  ret.zi=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  
  // Todo - add angular correction
  // Currently not using gyroscope or temperature data.
/*  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)  */
  return ret;
}

struct vector3i get_raw_acc_vel(const struct vector3i * raw_acc)
{
  struct vector3i ret;
  static struct vector3i raw_vel_old = {0,0,0};
  
  ret.xi = raw_acc->xi - raw_vel_old.xi;
  ret.yi = raw_acc->yi - raw_vel_old.yi;
  ret.zi = raw_acc->zi - raw_vel_old.zi;
  
  raw_vel_old = *raw_acc;
  return ret;
}

struct vector4f compute_corr()
{
  struct vector4f ret;
  const struct vector3i raw_acc = get_raw_acc();
  const struct vector3i raw_acc_vel = get_raw_acc_vel(&raw_acc);
  ret.xf = (float) (ALPHA * (Ac0.xi - raw_acc.xi) - BETA * raw_acc_vel.xi);
  ret.yf = (float) (ALPHA * (Ac0.yi - raw_acc.yi) - BETA * raw_acc_vel.yi);
  ret.zf = (float) (ALPHA * (Ac0.zi - raw_acc.zi) - BETA * raw_acc_vel.zi);
  
  // Todo - add angular correction
  ret.wf = 0.0f; 
  
  return ret;
}
