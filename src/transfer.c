#include "internal.h"

//// Single sample transfer functions
// We assume little endian for all formats.
#define DEF_MIXED_TRANSFER_SAMPLE_FROM(name, datatype)                  \
  static inline void mixed_transfer_sample_from_##name(void *in, uint32_t is, float *out, uint32_t os, float volume) { \
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

extern inline void mixed_transfer_sample_from_int24(void *in, uint32_t is, float *out, uint32_t os, float volume) {
  // Read MSB as int8, others as uint8
  int32_t sample = (((int8_t *)in)[3*is+2] << 16) + \
                  (((uint8_t *)in)[3*is+1] << 8 ) + \
                  (((uint8_t *)in)[3*is]);
  out[os] = mixed_from_int24(sample) * volume;
}

extern inline void mixed_transfer_sample_from_uint24(void *in, uint32_t is, float *out, uint32_t os, float volume) {
  uint8_t *data = (uint8_t *)in;
  uint24_t sample = (data[3*is+2] << 16) + (data[3*is+1] << 8) + (data[3*is]);
  out[os] = mixed_from_uint24(sample) * volume;
}

#define DEF_MIXED_TRANSFER_SAMPLE_TO(name, datatype)                    \
  static inline void mixed_transfer_sample_to_##name(float *in, uint32_t is, void *out, uint32_t os, float volume){ \
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

extern inline void mixed_transfer_sample_to_int24(float *in, uint32_t is, void *out, uint32_t os, float volume){
  int24_t sample = mixed_to_int24(in[is] * volume);
  ((uint8_t *)out)[3*os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out)[3*os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out)[3*os+0] = (sample >>  0) & 0xFF;
}

extern inline void mixed_transfer_sample_to_uint24(float *in, uint32_t is, void *out, uint32_t os, float volume){
  uint24_t sample = mixed_to_uint24(in[is] * volume);
  ((uint8_t *)out)[3*os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out)[3*os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out)[3*os+0] = (sample >>  0) & 0xFF;
}

//// Array transfer functions
#define DEF_MIXED_TRANSFER_ARRAY_FROM_ALTERNATING(datatype)             \
  VECTORIZE float mixed_transfer_array_from_alternating_##datatype(void *in, float *out, uint8_t stride, uint32_t samples, float volume, float target_volume) { \
    if(samples == 0) return volume;                                     \
    mixed_transfer_sample_from_##datatype(in, 0, out, 0, volume);       \
    for(uint32_t sample=1; sample<samples; ++sample){                   \
      mixed_transfer_sample_from_##datatype(in, sample*stride, out, sample, volume); \
      if(out[sample-1] * out[sample] < 0.0f) {                          \
        volume = target_volume;                                         \
        mixed_transfer_sample_from_##datatype(in, sample*stride, out, sample, volume); \
      }                                                                 \
    }                                                                   \
    return volume;                                                      \
  }

#define DEF_MIXED_TRANSFER_ARRAY_TO_ALTERNATING(datatype)               \
  VECTORIZE static inline float mixed_transfer_array_to_alternating_##datatype(float *in, void *out, uint8_t stride, uint32_t samples, float volume, float target_volume){ \
    if(samples == 0) return volume;                                     \
    mixed_transfer_sample_to_##datatype(in, 0, out, 0, volume);         \
    for(uint32_t sample=1; sample<samples; ++sample){                   \
      if(in[sample-1] * in[sample] < 0.0f){                             \
        volume = target_volume;                                         \
      }                                                                 \
      mixed_transfer_sample_to_##datatype(in, sample, out, sample*stride, volume); \
    }                                                                   \
    return volume;                                                      \
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

VECTORIZE MIXED_EXPORT int mixed_buffer_from_pack(struct mixed_pack *in, struct mixed_buffer **outs, float *volume, float target_volume){
  channel_t channels = in->channels;
  uint32_t frames_to_bytes = channels * mixed_samplesize(in->encoding);
  uint32_t frames = UINT32_MAX;
  char *ind;
  float *outd[channels];

  mixed_pack_request_read((void**)&ind, &frames, in);
  frames = frames / frames_to_bytes;
  for(uint32_t i=0; i<channels; ++i)
    mixed_buffer_request_write(&outd[i], &frames, outs[i]);

  if(0 < frames){
    mixed_transfer_function_from fun = transfer_array_functions_from[in->encoding-1];
    uint8_t size = mixed_samplesize(in->encoding);
    float vol = *volume;
    // KLUDGE: this is not necessarily correct...
    *volume = target_volume;
    for(int8_t c=0; c<channels; ++c){
      fun(ind, outd[c], channels, frames, vol, target_volume);
      ind += size;
    }
  }

  mixed_pack_finish_read(frames * frames_to_bytes, in);
  for(uint32_t i=0; i<channels; ++i)
    mixed_buffer_finish_write(frames, outs[i]);
  
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

VECTORIZE MIXED_EXPORT int mixed_buffer_to_pack(struct mixed_buffer **ins, struct mixed_pack *out, float *volume, float target_volume){
  channel_t channels = out->channels;
  uint32_t frames_to_bytes = channels * mixed_samplesize(out->encoding);
  uint32_t frames = UINT32_MAX;
  char *outd;
  float *ind[channels];

  mixed_pack_request_write((void**)&outd, &frames, out);
  frames = frames / frames_to_bytes;
  for(uint32_t i=0; i<channels; ++i)
    mixed_buffer_request_read(&ind[i], &frames, ins[i]);

  if(0 < frames){
    mixed_transfer_function_to fun = transfer_array_functions_to[out->encoding-1];
    uint8_t size = mixed_samplesize(out->encoding);
    float vol = *volume;
    // KLUDGE: this is not necessarily correct...
    *volume = target_volume;
    for(int8_t c=0; c<channels; ++c){
      fun(ind[c], outd, channels, frames, vol, target_volume);
      outd += size;
    }
  }

  mixed_pack_finish_write(frames * frames_to_bytes, out);
  for(uint32_t i=0; i<channels; ++i)
    mixed_buffer_finish_read(frames, ins[i]);
  
  return 1;
}
