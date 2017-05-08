typedef int32_t int24_t;
typedef uint32_t uint24_t;

MIXED_EXPORT inline float mixed_from_float(float sample){
  return (1.0f<sample)? 1.0f
    : (-1.0f<sample)? sample
    : -1.0f;
}

MIXED_EXPORT inline float mixed_from_double(double sample){
  return (1.0d<sample)? 1.0f
    : (-1.0d<sample)? (float)sample
    : -1.0f;
}

MIXED_EXPORT inline float mixed_from_int8(int8_t sample){
  return (sample/((float)0x80));
}

MIXED_EXPORT inline float mixed_from_uint8(uint8_t sample){
  return (sample-0x80)/((float)0x80);
}

MIXED_EXPORT inline float mixed_from_int16(int16_t sample){
  return (sample/((float)0x8000));
}

MIXED_EXPORT inline float mixed_from_uint16(uint16_t sample){
  return (sample-0x8000)/((float)0x8000);
}

MIXED_EXPORT inline float mixed_from_int24(int24_t sample){
  return (sample/((float)0x800000));
}

MIXED_EXPORT inline float mixed_from_uint24(uint24_t sample){
  return (sample-0x800000)/((float)0x800000);
}

MIXED_EXPORT inline float mixed_from_int32(int32_t sample){
  return mixed_from_float(sample/((float)0x80000000L));
}

MIXED_EXPORT inline float mixed_from_uint32(uint32_t sample){
  return mixed_from_float((sample-0x80000000L)/((float)0x80000000L));
}

MIXED_EXPORT inline float mixed_to_float(float sample){
  return (1.0f<sample)? 1.0f
    : (-1.0f<sample)? sample
    : -1.0f;
}

MIXED_EXPORT inline double mixed_to_double(float sample){
  return (1.0f<sample)? 1.0d
    : (-1.0f<sample)? (double)sample
    : -1.0d;
}

MIXED_EXPORT inline int8_t mixed_to_int8(float sample){
  return roundf(sample*0x80);
}

MIXED_EXPORT inline uint8_t mixed_to_uint8(float sample){
  return roundf(sample*0x80)+0x80;
}

MIXED_EXPORT inline int16_t mixed_to_int16(float sample){
  return roundf(sample*0x8000);
}

MIXED_EXPORT inline uint16_t mixed_to_uint16(float sample){
  return roundf(sample*0x8000)+0x8000;
}

MIXED_EXPORT inline int24_t mixed_to_int24(float sample){
  return roundf(sample*0x800000);
}

MIXED_EXPORT inline uint24_t mixed_to_uint24(float sample){
  return roundf(sample*0x800000)+0x800000;
}

MIXED_EXPORT inline int32_t mixed_to_int32(float sample){
  return (sample*0x80000000L);
}

MIXED_EXPORT inline uint32_t mixed_to_uint32(float sample){
  return (sample*0x80000000L)+0x80000000L;
}
