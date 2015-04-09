#ifndef _VECTOR3_H_
#define _VECTOR3_H_

// #########
// vector3 types
// #########
struct vector3i
{
  int xi;
  int yi;
  int zi;
};
struct vector3f
{
  float xf;
  float yf;
  float zf;
};
struct vector4f
{
  float xf;
  float yf;
  float zf;
  float wf;
};
void add_vec(struct vector3f * v1, const struct vector3f * v2);
void add_vec(struct vector4f * v1, const struct vector4f * v2);
void mul_vec(struct vector3f * v1, const float val);
void mul_vec(struct vector4f * v1, const float val);
float get_mod(const struct vector3f * v);
float get_proj_angle(const struct vector3f * v1, const struct vector3f * v2);
void constrain_vec(struct vector3f *v, float lim);
void constrain_vec(struct vector4f *v, float lim);

#endif
