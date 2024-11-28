#include <math.h>

#define INT24_MAX 8388607
#define INT24_MIN -8388608
#define UINT24_MAX 16777215
typedef int32_t int24_t;
typedef uint32_t uint24_t;

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_float(float sample){
  return (1.0f<sample)? 1.0f
    : (-1.0f<sample)? sample
    : -1.0f;
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_double(double sample){
  return (1.0<sample)? 1.0f
    : (-1.0<sample)? (float)sample
    : -1.0f;
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_int8(int8_t sample){
  return (sample < 0)
    ? -(sample/(float)INT8_MIN)
    : +(sample/(float)INT8_MAX);
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_uint8(uint8_t sample){
  return ((float)sample)/((float)UINT8_MAX/2)-1;
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_int16(int16_t sample){
  return (sample < 0)
    ? -(sample/(float)INT16_MIN)
    : +(sample/(float)INT16_MAX);
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_uint16(uint16_t sample){
  return ((float)sample)/((float)UINT16_MAX/2)-1;
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_int24(int24_t sample){
  return (sample < 0)
    ? -(sample/(float)INT24_MIN)
    : +(sample/(float)INT24_MAX);
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_uint24(uint24_t sample){
  return ((double)sample)/((double)UINT24_MAX/2)-1;
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_int32(int32_t sample){
  return (sample < 0)
    ? -(sample/(double)INT32_MIN)
    : +(sample/(double)INT32_MAX);
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_from_uint32(uint32_t sample){
  return mixed_from_double(((double)sample-0x80000000L)/((double)0x80000000L));
}

__attribute__((always_inline))
MIXED_EXPORT inline float mixed_to_float(float sample){
  return (1.0f<=sample)? 1.0f
    : (-1.0f<=sample)? sample
    : -1.0f;
}

__attribute__((always_inline))
MIXED_EXPORT inline double mixed_to_double(float sample){
  return (1.0f<=sample)? 1.0
    : (-1.0f<=sample)? (double)sample
    : -1.0;
}

__attribute__((always_inline))
MIXED_EXPORT inline int8_t mixed_to_int8(float sample){
  return (1.0f<=sample)? INT8_MAX
    : (-1.0f<=sample)? sample*0x80
    : INT8_MIN;
}

__attribute__((always_inline))
MIXED_EXPORT inline uint8_t mixed_to_uint8(float sample){
  return (1.0f<=sample)? UINT8_MAX
    : (-1.0f<=sample)? (sample+1)*0x80
    : 0;
}

__attribute__((always_inline))
MIXED_EXPORT inline int16_t mixed_to_int16(float sample){
  return (1.0f<=sample)? INT16_MAX
    : (-1.0f<=sample)? sample*0x8000
    : INT16_MIN;
}

__attribute__((always_inline))
MIXED_EXPORT inline uint16_t mixed_to_uint16(float sample){
  return (1.0f<=sample)? UINT16_MAX
    : (-1.0f<=sample)? (sample+1)*0x8000
    : 0;
}

__attribute__((always_inline))
MIXED_EXPORT inline int24_t mixed_to_int24(float sample){
  return (1.0f<=sample)? INT24_MAX
    : (-1.0f<=sample)? sample*0x800000
    : INT24_MIN;
}

__attribute__((always_inline))
MIXED_EXPORT inline uint24_t mixed_to_uint24(float sample){
  return (1.0f<=sample)? UINT24_MAX
    : (-1.0f<=sample)? round((sample+1)*((float)UINT24_MAX/2))
    : 0;
}

__attribute__((always_inline))
MIXED_EXPORT inline int32_t mixed_to_int32(float sample){
  return (1.0f<=sample)? INT32_MAX
    : (-1.0f<=sample)? sample*0x80000000L
    : INT32_MIN;
}

__attribute__((always_inline))
MIXED_EXPORT inline uint32_t mixed_to_uint32(float sample){
  return (1.0f<=sample)? UINT32_MAX
    : (-1.0f<=sample)? (sample+1)*0x80000000L
    : 0;
}
