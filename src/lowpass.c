#include "internal.h"

void lowpass_init(uint32_t samplerate, uint32_t cutoff, struct lowpass_data *data){
  float a = tan(M_PI*cutoff/samplerate);
  float a2 = a*a;
  float s2 = sqrt(2);
  data->k = (a2)/(1+s2*a+a2);
  data->a[0] = (2*(a2-1))/(1+s2*a+a2);
  data->a[1] = (1-s2*a+a2)/(1+s2*a+a2);
  data->b[0] = 2;
  data->b[1] = 1;
  data->x[0] = 0;
  data->x[1] = 0;
  data->y[0] = 0;
  data->y[1] = 0;
}

void lowpass_reset(struct lowpass_data *data){
  data->x[0] = 0;
  data->x[1] = 0;
  data->y[0] = 0;
  data->y[1] = 0;
}

float lowpass(float s, struct lowpass_data *data){
  float *x = data->x;
  float *y = data->y;
  float *a = data->a;
  float *b = data->b;
  float k = data->k;
  float out = k*s + k*b[0]*x[0] + k*b[1]*x[1] - a[0]*y[0] - a[1]*y[1];
  x[1] = x[0];
  x[0] = s;
  y[1] = y[0];
  y[0] = out;
  return out;
}
