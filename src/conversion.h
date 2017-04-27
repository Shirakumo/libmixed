inline float mixed_from_int8(int8_t sample){
  return (sample/128.0f);
}

inline float mixed_from_uint8(uint8_t sample){
  return (sample-128)/128.0f;
}

inline float mixed_from_int16(int16_t sample){
  return (sample/32768.0f);
}

inline float mixed_from_uint16(uint16_t sample){
  return (sample-32768)/32768.0f;
}

inline float mixed_from_int24(int24_t sample){
  return (sample/8388608.0f);
}

inline float mixed_from_uint24(uint24_t sample){
  return (sample-8388608L)/8388608.0f;
}

inline float mixed_from_int32(int32_t sample){
  return mixed_from_float(sample/2147483648.0f);
}

inline float mixed_from_uint32(uint32_t sample){
  return mixed_from_float((sample-2147483648L)/2147483648.0f);
}

inline float mixed_from_int64(int64_t sample){
  return mixed_from_int32(sample>>32);
}

inline float mixed_from_uint64(uint64_t sample){
  return mixed_from_uint32(sample>>32);
}

inline float mixed_from_float(float sample){
  return (1.0f<sample)? 1.0f
    : (-1.0f<sample)? sample
    : -1.0f;
}

inline float mixed_from_double(double sample){
  return (1.0d<sample)? 1.0f
    : (-1.0d<sample)? (float)sample
    : -1.0f;
}

inline int8_t mixed_to_int8(float sample){
  return (sample*128);
}

inline uint8_t mixed_to_uint8(float sample){
  return (sample*128)+128;
}

inline int16_t mixed_to_int16(float sample){
  return (sample*32768);
}

inline uint16_t mixed_to_uint16(float sample){
  return (sample*32768)+32768;
}

inline int24_t mixed_to_int24(float sample){
  return (sample*8388608);
}

inline uint24_t mixed_to_uint24(float sample){
  return (sample*8388608)+8388608;
}

inline int32_t mixed_to_int32(float sample){
  return (sample*2147483648L);
}

inline uint32_t mixed_to_uint32(float sample){
  return (sample*2147483648L)+2147483648L;
}

inline int64_t mixed_to_int64(float sample){
  return ((double)sample)*9223372036854775808LL;
}

inline uint64_t mixed_to_uint64(float sample){
  return ((double)sample)*9223372036854775808LL+9223372036854775808LL;
}

inline float mixed_to_float(float sample){
  return (1.0f<sample)? 1.0f
    : (-1.0f<sample)? sample
    : -1.0f;
}

inline double mixed_to_double(float sample){
  return (1.0f<sample)? 1.0d
    : (-1.0f<sample)? (double)sample
    : -1.0d;
}
