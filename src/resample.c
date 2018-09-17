#include "internal.h"

static inline size_t clamp(size_t l, size_t x, size_t u){
  return (l < x)? ((x < u)? x : u) : l;
}

MIXED_EXPORT int mixed_resample_nearest(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples){
  float *out_data = out->data;
  float *in_data = in->data;
  for(size_t o=0; o<out_samples; ++o){
    size_t i = (o * in_samplerate) / out_samplerate;
    out_data[o] = in_data[i];
  }
  return 1;
}

MIXED_EXPORT int mixed_resample_linear(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples){
  float *out_data = out->data;
  float *in_data = in->data;

  float ratio = in_samplerate / ((float)out_samplerate);
  float i = 0;
  for(size_t o=0; o<out_samples; ++o, i+=ratio){
    int ii = clamp(0, (int)i, in->size-2);
    float t = i-ii;
    
    out_data[o] = in_data[ii]+(in_data[ii+1]-in_data[ii])*t;
  }
  return 1;
}

MIXED_EXPORT int mixed_resample_cubic(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples){
  float *out_data = out->data;
  float *in_data = in->data;
  
  size_t in_samples = out_samples * in_samplerate / out_samplerate;
  for(size_t o=0; o<out_samples; ++o){
    float p = (o * in_samplerate) / ((float)out_samplerate);
    size_t i = (size_t)p;
    float t = p-i;
    
    // Primitive cubic hermite spline
    float A = in_data[clamp(0, i-1, in_samples)];
    float B = in_data[clamp(0, i+0, in_samples)];
    float C = in_data[clamp(0, i+1, in_samples)];
    float D = in_data[clamp(0, i+2, in_samples)];

    float a = -A/2.0f + (3.0f*B)/2.0f - (3.0f*C)/2.0f + D/2.0f;
    float b = A - (5.0f*B)/2.0f + 2.0f*C - D / 2.0f;
    float c = -A/2.0f + C/2.0f;
    float d = B;
 
    out_data[o] = a*t*t*t + b*t*t + c*t + d;
  }
  return 1;
}
