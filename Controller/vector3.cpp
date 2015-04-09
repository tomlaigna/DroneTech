#include <Arduino.h>
#include <math.h>
#include "vector3.h"

// #########
// vector3 types
// #########
void add_vec(struct vector3f * v1, const struct vector3f * v2)
{
  v1->xf += v2->xf; v1->yf += v2->yf; v1->zf += v2->zf;
}
void add_vec(struct vector4f * v1, const struct vector4f * v2)
{
  v1->xf += v2->xf; v1->yf += v2->yf; v1->zf += v2->zf; v1->wf += v2->wf;
}
void mul_vec(struct vector3f * v1, const float val)
{
  v1->xf *= val; v1->yf *= val; v1->zf *= val;
}
void mul_vec(struct vector4f * v1, const float val)
{
  v1->xf *= val; v1->yf *= val; v1->zf *= val; v1->wf *= val;
}
float get_mod(const struct vector3f * v)
{
  float mod = sqrt(pow(v->xf,2) + pow(v->yf,2) + pow(v->zf,2));
  return mod;
}
float get_proj_angle(const struct vector3f * v1, const struct vector3f * v2)
{
  float dot = v1->xf * v2->xf + v1->yf * v2->yf + v1->zf * v2->zf;
  float mod1 = sqrt(pow(v1->xf,2) + pow(v1->yf,2) + pow(v1->zf,2));
  float mod2 = sqrt(pow(v2->xf,2) + pow(v2->yf,2) + pow(v2->zf,2));
  return acos(dot / mod1 / mod2);
}
void constrain_vec(struct vector3f *v, float lim)
{
  if (float mod = get_mod(v) > lim)
    mul_vec(v, lim / mod);
}
void constrain_vec(struct vector4f *v, float lim)
{
  struct vector3f* v3 = (struct vector3f *)v;
  if (float mod = get_mod(v3) > lim)
    mul_vec(v, lim / mod);
  v->zf = constrain(v->zf, -lim, lim);
}
