#include "mixed.h"

extern inline float mixed_from_float(float sample);
extern inline float mixed_from_double(double sample);
extern inline float mixed_from_int8(int8_t sample);
extern inline float mixed_from_uint8(uint8_t sample);
extern inline float mixed_from_int16(int16_t sample);
extern inline float mixed_from_uint16(uint16_t sample);
extern inline float mixed_from_int24(int24_t sample);
extern inline float mixed_from_uint24(uint24_t sample);
extern inline float mixed_from_int32(int32_t sample);
extern inline float mixed_from_uint32(uint32_t sample);
extern inline float mixed_to_float(float sample);
extern inline double mixed_to_double(float sample);
extern inline int8_t mixed_to_int8(float sample);
extern inline uint8_t mixed_to_uint8(float sample);
extern inline int16_t mixed_to_int16(float sample);
extern inline uint16_t mixed_to_uint16(float sample);
extern inline int24_t mixed_to_int24(float sample);
extern inline uint24_t mixed_to_uint24(float sample);
extern inline int32_t mixed_to_int32(float sample);
extern inline uint32_t mixed_to_uint32(float sample);
