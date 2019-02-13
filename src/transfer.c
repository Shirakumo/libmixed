#include "internal.h"

//// Single sample transfer functions
// We assume little endian for all formats.
#define DEF_MIXED_TRANSFER_SAMPLE_FROM(name, datatype)                  \
  static inline void mixed_transfer_sample_from_##name(void *in, size_t is, float *out, size_t os, float volume) { \
    out[os] = mixed_from_##name(((datatype *)in)[is]) * volume;         \
  }

DEF_MIXED_TRANSFER_SAMPLE_FROM(int8, int8_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint8, uint8_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(int16, int16_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint16, uint16_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(int32, int32_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint32, uint32_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(float, float);
DEF_MIXED_TRANSFER_SAMPLE_FROM(double, double);

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

DEF_MIXED_TRANSFER_SAMPLE_TO(int8, int8_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint8, uint8_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(int16, int16_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint16, uint16_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(int32, int32_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint32, uint32_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(float, float);
DEF_MIXED_TRANSFER_SAMPLE_TO(double, double);

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
  void mixed_transfer_array_from_alternating_##datatype(void *in, float **outs, uint8_t channels, size_t samples, float volume) { \
    size_t i = 0;                                                       \
    for(uint8_t channel=0; channel<channels; ++channel){                \
      float *out = outs[channel];                                       \
      for(size_t sample=0; sample<samples; ++sample){                   \
        mixed_transfer_sample_from_##datatype(in, sample*channels+channel, out, sample, volume); \
      }                                                                 \
    }                                                                   \
  }

#define DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(datatype)              \
  void mixed_transfer_array_from_sequential_##datatype(void *in, float **outs, uint8_t channels, size_t samples, float volume) { \
    size_t offset = 0;                                                  \
    for(uint8_t channel=0; channel<channels; ++channel){                \
      float *out = outs[channel];                                       \
      for(size_t sample=0; sample<samples; ++sample){                   \
        mixed_transfer_sample_from_##datatype(in, sample+offset, out, sample, volume); \
      }                                                                 \
      offset += samples;                                                \
    }                                                                   \
  }

#define DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(datatype)               \
  static inline void mixed_transfer_array_to_alternating_##datatype(float **ins, void *out, uint8_t channels, size_t samples, float volume){ \
    for(size_t channel=0; channel<channels; ++channel){                 \
      float *in = ins[channel];                                         \
      for(size_t sample=0; sample<samples; ++sample){                   \
        mixed_transfer_sample_to_##datatype(in, sample, out, sample*channels+channel, volume); \
      }                                                                 \
    }                                                                   \
  }

#define DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(datatype)                \
  static inline void mixed_transfer_array_to_sequential_##datatype(float **ins, void *out, uint8_t channels, size_t samples, float volume){ \
    size_t offset = 0;                                                  \
    for(uint8_t channel=0; channel<channels; ++channel){                \
      float *in = ins[channel];                                         \
      for(size_t sample=0; sample<samples; ++sample){                   \
        mixed_transfer_sample_to_##datatype(in, sample, out, sample+offset, volume); \
        ++sample;                                                       \
      }                                                                 \
      offset += samples;                                                \
    }                                                                   \
  }

DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int8);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint8);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int16);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint16);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int24);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint24);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(int32);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(uint32);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(float);
DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(double);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(int8);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(uint8);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(int16);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(uint16);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(int24);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(uint24);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(int32);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(uint32);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(float);
DEF_MIXED_TRANSFER_ARRAY_FROM_SEQUENTIAL(double);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int8);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint8);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int16);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint16);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int24);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint24);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(int32);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(uint32);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(float);
DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(double);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(int8);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(uint8);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(int16);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(uint16);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(int24);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(uint24);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(int32);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(uint32);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(float);
DEF_MIXED_TRANSFER_ARRAY_TO_SEQUENTIAL(double);

//// Buffer transfer functions
static inline int pack_table_index(struct mixed_packed_audio *pack){
  // This works because encodings are 1-10, and layouts are 1-2.
  // Thus INT16, SEQUENTIAL => 3, 2 => (3-1)*2 + (2-1) = 5 => mixed_transfer_array_XXX_sequential_int16
  return (pack->encoding-1)*2 + pack->layout-1;
}

typedef void (*transfer_function_from)(void *, float **, uint8_t, size_t, float);

static transfer_function_from mixed_transfer_functions_from[20] =
  { mixed_transfer_array_from_alternating_int8,
    mixed_transfer_array_from_sequential_int8,
    mixed_transfer_array_from_alternating_uint8,
    mixed_transfer_array_from_sequential_uint8,
    mixed_transfer_array_from_alternating_int16,
    mixed_transfer_array_from_sequential_int16,
    mixed_transfer_array_from_alternating_uint16,
    mixed_transfer_array_from_sequential_uint16,
    mixed_transfer_array_from_alternating_int24,
    mixed_transfer_array_from_sequential_int24,
    mixed_transfer_array_from_alternating_uint24,
    mixed_transfer_array_from_sequential_uint24,
    mixed_transfer_array_from_alternating_int32,
    mixed_transfer_array_from_sequential_int32,
    mixed_transfer_array_from_alternating_uint32,
    mixed_transfer_array_from_sequential_uint32,
    mixed_transfer_array_from_alternating_float,
    mixed_transfer_array_from_sequential_float,
    mixed_transfer_array_from_alternating_double,
    mixed_transfer_array_from_sequential_double,
  };

MIXED_EXPORT int mixed_buffer_from_packed_audio(struct mixed_packed_audio *in, struct mixed_buffer **outs, size_t samples, float volume){
  uint8_t channels = in->channels;
  void *ind = in->data;
  float *outd[channels];
  for(int i=0; i<channels; ++i)
    outd[i] = outs[i]->data;
  
  transfer_function_from fun = mixed_transfer_functions_from[pack_table_index(in)];
  fun(ind, outd, channels, samples, volume);
  
  return 1;
}

typedef void (*transfer_function_to)(float **, void *, uint8_t, size_t, float);

static transfer_function_to mixed_transfer_functions_to[20] =
  { mixed_transfer_array_to_alternating_int8,
    mixed_transfer_array_to_sequential_int8,
    mixed_transfer_array_to_alternating_uint8,
    mixed_transfer_array_to_sequential_uint8,
    mixed_transfer_array_to_alternating_int16,
    mixed_transfer_array_to_sequential_int16,
    mixed_transfer_array_to_alternating_uint16,
    mixed_transfer_array_to_sequential_uint16,
    mixed_transfer_array_to_alternating_int24,
    mixed_transfer_array_to_sequential_int24,
    mixed_transfer_array_to_alternating_uint24,
    mixed_transfer_array_to_sequential_uint24,
    mixed_transfer_array_to_alternating_int32,
    mixed_transfer_array_to_sequential_int32,
    mixed_transfer_array_to_alternating_uint32,
    mixed_transfer_array_to_sequential_uint32,
    mixed_transfer_array_to_alternating_float,
    mixed_transfer_array_to_sequential_float,
    mixed_transfer_array_to_alternating_double,
    mixed_transfer_array_to_sequential_double,
  };

MIXED_EXPORT int mixed_buffer_to_packed_audio(struct mixed_buffer **ins, struct mixed_packed_audio *out, size_t samples, float volume){
  uint8_t channels = out->channels;
  void *outd = out->data;
  float *ind[channels];
  for(int i=0; i<channels; ++i)
    ind[i] = ins[i]->data;
  
  transfer_function_to fun = mixed_transfer_functions_to[pack_table_index(out)];
  fun(ind, outd, channels, samples, volume);
  
  return 1;
}
