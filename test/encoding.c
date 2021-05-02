#define __TEST_SUITE encoding
#include "tester.h"

define_test(int8, {
    is(mixed_to_int8(+2.0), INT8_MAX);
    is(mixed_to_int8(+1.0), INT8_MAX);
    is(mixed_to_int8( 0.0), 0);
    is(mixed_to_int8(-1.0), INT8_MIN);
    is(mixed_to_int8(-2.0), INT8_MIN);
    is_f(mixed_from_int8(INT8_MAX), 1.0);
    is_f(mixed_from_int8(INT8_MIN), -1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      int8_t sample = (rand() % INT8_MAX*2)-INT8_MAX;
      is(mixed_to_int8(mixed_from_int8(sample)), sample);
    }
  cleanup:;
  });

define_test(uint8, {
    is(mixed_to_uint8(+2.0), UINT8_MAX);
    is(mixed_to_uint8(+1.0), UINT8_MAX);
    is(mixed_to_uint8(-1.0), 0);
    is(mixed_to_uint8(-2.0), 0);
    is(mixed_from_uint8(0), -1.0);
    is(mixed_from_uint8(UINT8_MAX), +1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      uint8_t sample = rand() % UINT8_MAX;
      is_ui(mixed_to_uint8(mixed_from_uint8(sample)), sample);
    }
  cleanup:;
  });

define_test(int16, {
    is(mixed_to_int16(+2.0), INT16_MAX);
    is(mixed_to_int16(+1.0), INT16_MAX);
    is(mixed_to_int16( 0.0), 0);
    is(mixed_to_int16(-1.0), INT16_MIN);
    is(mixed_to_int16(-2.0), INT16_MIN);
    is_f(mixed_from_int16(INT16_MAX), 1.0);
    is_f(mixed_from_int16(INT16_MIN), -1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      int16_t sample = (rand() % INT16_MAX*2)-INT16_MAX;
      is_a(mixed_to_int16(mixed_from_int16(sample)), sample, 1);
    }
  cleanup:;
  });

define_test(uint16, {
    is(mixed_to_uint16(+2.0), UINT16_MAX);
    is(mixed_to_uint16(+1.0), UINT16_MAX);
    is(mixed_to_uint16(-1.0), 0);
    is(mixed_to_uint16(-2.0), 0);
    is(mixed_from_uint16(0), -1.0);
    is(mixed_from_uint16(UINT16_MAX), +1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      uint16_t sample = rand() % UINT16_MAX;
      is_a(mixed_to_uint16(mixed_from_uint16(sample)), sample, 1);
    }
  cleanup:;
  });

define_test(int24, {
    is(mixed_to_int24(+2.0), INT24_MAX);
    is(mixed_to_int24(+1.0), INT24_MAX);
    is(mixed_to_int24( 0.0), 0);
    is(mixed_to_int24(-1.0), INT24_MIN);
    is(mixed_to_int24(-2.0), INT24_MIN);
    is_f(mixed_from_int24(INT24_MAX), 1.0);
    is_f(mixed_from_int24(INT24_MIN), -1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      int24_t sample = (rand() % INT24_MAX*2)-INT24_MAX;
      is_a(mixed_to_int24(mixed_from_int24(sample)), sample, 1);
    }
  cleanup:;
  });

define_test(uint24, {
    is(mixed_to_uint24(+2.0), UINT24_MAX);
    is(mixed_to_uint24(+1.0), UINT24_MAX);
    is(mixed_to_uint24(-1.0), 0);
    is(mixed_to_uint24(-2.0), 0);
    is(mixed_from_uint24(0), -1.0);
    is(mixed_from_uint24(UINT24_MAX), +1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      uint24_t sample = rand() % UINT24_MAX;
      is_ui(mixed_to_uint24(mixed_from_uint24(sample)), sample);
    }
  cleanup:;
  });

define_test(int32, {
    is(mixed_to_int32(+2.0), INT32_MAX);
    is(mixed_to_int32(+1.0), INT32_MAX);
    is(mixed_to_int32(0.0), 0);
    is(mixed_to_int32(-1.0), INT32_MIN);
    is(mixed_to_int32(-2.0), INT32_MIN);
    is_f(mixed_from_int32(INT32_MAX), 1.0);
    is_f(mixed_from_int32(INT32_MIN), -1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      int32_t sample = (rand() % INT32_MAX*2)-INT32_MAX;
      is_a(mixed_to_int32(mixed_from_int32(sample)), sample, 150);
    }
  cleanup:;
  });

define_test(uint32, {
    is(mixed_to_uint32(+2.0), UINT32_MAX);
    is(mixed_to_uint32(+1.0), UINT32_MAX);
    is(mixed_to_uint32(-1.0), 0);
    is(mixed_to_uint32(-2.0), 0);
    is(mixed_from_uint32(0), -1.0);
    is(mixed_from_uint32(UINT32_MAX), +1.0);
    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      uint32_t sample = rand() % UINT32_MAX;
      is_a(mixed_to_uint32(mixed_from_uint32(sample)), sample, 150);
    }
  cleanup:;
  });

#undef __TEST_SUITE
