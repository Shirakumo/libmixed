#include "internal.h"

//// Single sample transfer functions
// We assume little endian for all formats.
#define DEF_MIXED_TRANSFER_SAMPLE_FROM(name, datatype)                  \
  static inline void mixed_transfer_sample_from_##name(void *in, size_t is, float *out, size_t os, float volume) { \
    out[os] = mixed_from_##name(((datatype *)in)[is]) * volume;         \
  }

DEF_MIXED_TRANSFER_SAMPLE_FROM(int8, int8_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint8, uint8_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(int16, int16_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint16, uint16_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(int32, int32_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint32, uint32_t)
DEF_MIXED_TRANSFER_SAMPLE_FROM(float, float)
DEF_MIXED_TRANSFER_SAMPLE_FROM(double, double)

extern inline void mixed_transfer_sample_from_int24(void *in, size_t is, float *out, size_t os, float volume) {
  int8_t *data = (int8_t *)in;
  // Read in as uint
  uint32_t temp = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
  // Shift up to fill 32 bit range
  temp = temp << 8;
  // Convert to signed, this is actually not standards compliant.
  int32_t sample = (int32_t)temp;
  // Shift down again to get back to 24 bit range.
  out[os] = mixed_from_int24(sample >> 8) * volume;
}

extern inline void mixed_transfer_sample_from_uint24(void *in, size_t is, float *out, size_t os, float volume) {
  int8_t *data = (int8_t *)in;
  uint24_t sample = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
  out[os] = mixed_from_uint24(sample) * volume;
}

#define DEF_MIXED_TRANSFER_SAMPLE_TO(name, datatype)                    \
  static inline void mixed_transfer_sample_to_##name(float *in, size_t is, void *out, size_t os, float volume){ \
    ((datatype *)out)[os] = mixed_to_##name(in[is]) * volume;           \
  }

DEF_MIXED_TRANSFER_SAMPLE_TO(int8, int8_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(uint8, uint8_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(int16, int16_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(uint16, uint16_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(int32, int32_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(uint32, uint32_t)
DEF_MIXED_TRANSFER_SAMPLE_TO(float, float)
DEF_MIXED_TRANSFER_SAMPLE_TO(double, double)

extern inline void mixed_transfer_sample_to_int24(float *in, size_t is, void *out, size_t os, float volume){
  int24_t sample = mixed_to_int24(in[is]) * volume;
  ((uint8_t *)out)[os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out)[os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out)[os+0] = (sample >>  0) & 0xFF;
}

extern inline void mixed_transfer_sample_to_uint24(float *in, size_t is, void *out, size_t os, float volume){
  uint24_t sample = mixed_to_uint24(in[is]) * volume;
  ((uint8_t *)out)[os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out)[os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out)[os+0] = (sample >>  0) & 0xFF;
}

//// Array transfer functions
#define DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(datatype)             \
  void mixed_transfer_array_from_alternating_##datatype(void *in, float *out, uint8_t stride, size_t samples, float volume) { \
    for(size_t sample=0; sample<samples; ++sample){                     \
      mixed_transfer_sample_from_##datatype(in, sample*stride, out, sample, volume); \
    }                                                                   \
  }

#define DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(datatype)               \
  static inline void mixed_transfer_array_to_alternating_##datatype(float *in, void *out, uint8_t stride, size_t samples, float volume){ \
    for(size_t sample=0; sample<samples; ++sample){                     \
      mixed_transfer_sample_to_##datatype(in, sample, out, sample*stride, volume); \
    }                                                                   \
  }

DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int8)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint8)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int16)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint16)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int24)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint24)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int32)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint32)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(float)
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(double)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int8)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint8)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int16)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint16)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int24)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint24)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int32)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint32)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(float)
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(double)

//// Buffer transfer functions
static mixed_transfer_function_from transfer_array_functions_from[20] =
  { mixed_transfer_array_from_alternating_int8,
    mixed_transfer_array_from_alternating_uint8,
    mixed_transfer_array_from_alternating_int16,
    mixed_transfer_array_from_alternating_uint16,
    mixed_transfer_array_from_alternating_int24,
    mixed_transfer_array_from_alternating_uint24,
    mixed_transfer_array_from_alternating_int32,
    mixed_transfer_array_from_alternating_uint32,
    mixed_transfer_array_from_alternating_float,
    mixed_transfer_array_from_alternating_double,
  };

MIXED_EXPORT mixed_transfer_function_from mixed_translator_from(enum mixed_encoding encoding){
  return transfer_array_functions_from[encoding-1];
}

MIXED_EXPORT int mixed_buffer_from_packed_audio(struct mixed_packed_audio *in, struct mixed_buffer **outs, size_t *samples, float volume){
  uint8_t channels = in->channels;
  void *ind = in->data;
  float *outd[channels];
  for(size_t i=0; i<channels; ++i)
    mixed_buffer_request_write(&outd[i], samples, outs[i]);
  
  mixed_transfer_function_from fun = transfer_array_functions_from[in->encoding-1];
  for(int8_t c=0; c<channels; ++c)
    fun(ind, outd[c], channels, *samples, volume);

  for(size_t i=0; i<channels; ++i)
    mixed_buffer_finish_write(*samples, outs[i]);
  
  return 1;
}

static mixed_transfer_function_to transfer_array_functions_to[20] =
  { mixed_transfer_array_to_alternating_int8,
    mixed_transfer_array_to_alternating_uint8,
    mixed_transfer_array_to_alternating_int16,
    mixed_transfer_array_to_alternating_uint16,
    mixed_transfer_array_to_alternating_int24,
    mixed_transfer_array_to_alternating_uint24,
    mixed_transfer_array_to_alternating_int32,
    mixed_transfer_array_to_alternating_uint32,
    mixed_transfer_array_to_alternating_float,
    mixed_transfer_array_to_alternating_double,
  };

MIXED_EXPORT mixed_transfer_function_to mixed_translator_to(enum mixed_encoding encoding){
  return transfer_array_functions_to[encoding-1];
}

MIXED_EXPORT int mixed_buffer_to_packed_audio(struct mixed_buffer **ins, struct mixed_packed_audio *out, size_t *samples, float volume){
  uint8_t channels = out->channels;
  void *outd = out->data;
  float *ind[channels];
  for(size_t i=0; i<channels; ++i)
    mixed_buffer_request_read(&ind[i], samples, ins[i]);
  
  mixed_transfer_function_to fun = transfer_array_functions_to[out->encoding-1];
  for(int8_t c=0; c<channels; ++c)
    fun(ind[c], outd, channels, *samples, volume);

  for(size_t i=0; i<channels; ++i)
    mixed_buffer_finish_read(*samples, ins[i]);
  
  return 1;
}
