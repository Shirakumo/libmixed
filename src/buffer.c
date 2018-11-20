#include "internal.h"

MIXED_EXPORT int mixed_make_buffer(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  buffer->data = calloc(size, sizeof(float));
  if(!buffer->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->size = size;
  return 1;
}

MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->data)
    free(buffer->data);
  buffer->data = 0;
}

MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer){
  if(buffer->data){
    memset(buffer->data, 0, sizeof(float)*buffer->size);
    return 1;
  }
  return 0;
}

MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  if(from != to){
    size_t size = (to->size<from->size)? from->size : to->size;
    memcpy(to->data, from->data, sizeof(float)*size);
    if(size < to->size){
      memset(to->data+size, 0, sizeof(float)*(to->size-size));
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_resize(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  void *new = realloc(buffer->data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->data = new;
  buffer->size = size;
  return 1;
}

#define DEF_MIXED_TRANSFER_SAMPLE_FROM(name, datatype)                  \
  static inline void mixed_transfer_sample_from_##name(struct mixed_packed_audio *in, size_t is, struct mixed_buffer *out, size_t os, float volume) { \
    out->data[os] = mixed_from_##name(((datatype *)in->data)[is]) * volume; \
  }

DEF_MIXED_TRANSFER_SAMPLE_FROM(int8, int8_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint8, uint8_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(int16, int16_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint16, uint16_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(int32, int32_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(uint32, uint32_t);
DEF_MIXED_TRANSFER_SAMPLE_FROM(float, float);
DEF_MIXED_TRANSFER_SAMPLE_FROM(double, double);

extern inline void mixed_transfer_sample_from_int24(struct mixed_packed_audio *in, size_t is, struct mixed_buffer *out, size_t os, float volume) {
  int8_t *data = (int8_t *)in->data;
  // Read in as uint
  uint32_t temp = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
  // Shift up to fill 32 bit range
  temp = temp << 8;
  // Convert to signed, this is actually not standards compliant.
  int32_t sample = (int32_t)temp;
  // Shift down again to get back to 24 bit range.
  out->data[os] = mixed_from_int24(sample >> 8) * volume;
}

extern inline void mixed_transfer_sample_from_uint24(struct mixed_packed_audio *in, size_t is, struct mixed_buffer *out, size_t os, float volume) {
  int8_t *data = (int8_t *)in->data;
  uint24_t sample = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
  out->data[os] = mixed_from_uint24(sample) * volume;
}

#define DEF_MIXED_BUFFER_FROM_PACKED(name)                              \
  void mixed_buffer_from_##name##_packed(struct mixed_packed_audio *in, struct mixed_buffer **outs, size_t samples, float volume) { \
    mixed_err(MIXED_NO_ERROR);                                          \
    size_t channels = in->channels;                                     \
    switch(in->layout){                                                 \
    case MIXED_ALTERNATING:{                                            \
      size_t i = 0;                                                     \
      for(uint8_t channel=0; channel<channels; ++channel){              \
        struct mixed_buffer *out = outs[channel];                       \
        if (out){                                                       \
          for(size_t sample=0; sample<samples; ++sample){               \
            mixed_transfer_sample_from_##name(in, sample*channels+channel, out, sample, volume); \
          }                                                             \
        }                                                               \
      }}                                                                \
      break;                                                            \
    case MIXED_SEQUENTIAL:{                                             \
      size_t offset = 0;                                                \
      for(uint8_t channel=0; channel<channels; ++channel){              \
        struct mixed_buffer *out = outs[channel];                       \
        if (out){                                                       \
          for(size_t sample=0; sample<samples; ++sample){               \
            mixed_transfer_sample_from_##name(in, sample+offset, out, sample, volume); \
          }                                                             \
        }                                                               \
        offset += samples;                                              \
      }}                                                                \
      break;                                                            \
    default:                                                            \
      mixed_err(MIXED_UNKNOWN_LAYOUT);                                  \
      break;                                                            \
    }                                                                   \
  }

DEF_MIXED_BUFFER_FROM_PACKED(int8);
DEF_MIXED_BUFFER_FROM_PACKED(uint8);
DEF_MIXED_BUFFER_FROM_PACKED(int16);
DEF_MIXED_BUFFER_FROM_PACKED(uint16);
DEF_MIXED_BUFFER_FROM_PACKED(int24);
DEF_MIXED_BUFFER_FROM_PACKED(uint24);
DEF_MIXED_BUFFER_FROM_PACKED(int32);
DEF_MIXED_BUFFER_FROM_PACKED(uint32);
DEF_MIXED_BUFFER_FROM_PACKED(float);
DEF_MIXED_BUFFER_FROM_PACKED(double);

MIXED_EXPORT int mixed_buffer_from_packed_audio(struct mixed_packed_audio *in, struct mixed_buffer **outs, size_t samples, float volume){
  switch(in->encoding){
  case MIXED_INT8:
    mixed_buffer_from_int8_packed(in, outs, samples, volume);
    break;
  case MIXED_UINT8:
    mixed_buffer_from_uint8_packed(in, outs, samples, volume);
    break;
  case MIXED_INT16:
    mixed_buffer_from_int16_packed(in, outs, samples, volume);
    break;
  case MIXED_UINT16:
    mixed_buffer_from_uint16_packed(in, outs, samples, volume);
    break;
  case MIXED_INT24:
    mixed_buffer_from_int24_packed(in, outs, samples, volume);
    break;
  case MIXED_UINT24:
    mixed_buffer_from_uint24_packed(in, outs, samples, volume);
    break;
  case MIXED_INT32:
    mixed_buffer_from_int32_packed(in, outs, samples, volume);
    break;
  case MIXED_UINT32:
    mixed_buffer_from_uint32_packed(in, outs, samples, volume);
    break;
  case MIXED_FLOAT:
    mixed_buffer_from_float_packed(in, outs, samples, volume);
    break;
  case MIXED_DOUBLE:
    mixed_buffer_from_double_packed(in, outs, samples, volume);
    break;
  default:
    mixed_err(MIXED_UNKNOWN_ENCODING);
    break;
  }
  return (mixed_error() == MIXED_NO_ERROR);
}

#define DEF_MIXED_TRANSFER_SAMPLE_TO(name, datatype)                    \
  static inline void mixed_transfer_sample_to_##name(struct mixed_buffer *in, size_t is, struct mixed_packed_audio *out, size_t os, float volume){ \
    ((datatype *)out->data)[os] = mixed_to_##name(in->data[is]) * volume; \
  }

// We assume little endian for all formats.
DEF_MIXED_TRANSFER_SAMPLE_TO(int8, int8_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint8, uint8_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(int16, int16_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint16, uint16_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(int32, int32_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(uint32, uint32_t);
DEF_MIXED_TRANSFER_SAMPLE_TO(float, float);
DEF_MIXED_TRANSFER_SAMPLE_TO(double, double);

extern inline void mixed_transfer_sample_to_int24(struct mixed_buffer *in, size_t is, struct mixed_packed_audio *out, size_t os, float volume){
  int24_t sample = mixed_to_int24(in->data[is]) * volume;
  ((uint8_t *)out->data)[os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out->data)[os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out->data)[os+0] = (sample >>  0) & 0xFF;
}

extern inline void mixed_transfer_sample_to_uint24(struct mixed_buffer *in, size_t is, struct mixed_packed_audio *out, size_t os, float volume){
  uint24_t sample = mixed_to_uint24(in->data[is]) * volume;
  ((uint8_t *)out->data)[os+2] = (sample >> 16) & 0xFF;
  ((uint8_t *)out->data)[os+1] = (sample >>  8) & 0xFF;
  ((uint8_t *)out->data)[os+0] = (sample >>  0) & 0xFF;
}

#define DEF_MIXED_BUFFER_TO_PACKED(name)                                \
  static inline void mixed_buffer_to_##name##_packed(struct mixed_buffer **ins, struct mixed_packed_audio *out, size_t samples, float volume){ \
    mixed_err(MIXED_NO_ERROR);                                          \
    size_t channels = out->channels;                                    \
    switch(out->layout){                                                \
    case MIXED_ALTERNATING:{                                            \
      for(size_t channel=0; channel<channels; ++channel){               \
        struct mixed_buffer *in = ins[channel];                         \
        if (in){                                                        \
          for(size_t sample=0; sample<samples; ++sample){               \
            mixed_transfer_sample_to_##name(in, sample, out, sample*channels+channel, volume); \
          }                                                             \
        }                                                               \
      }}                                                                \
      break;                                                            \
    case MIXED_SEQUENTIAL:{                                             \
      size_t offset = 0;                                                \
      for(uint8_t channel=0; channel<channels; ++channel){              \
        struct mixed_buffer *in = ins[channel];                         \
        if (in){                                                        \
          for(size_t sample=0; sample<samples; ++sample){               \
            mixed_transfer_sample_to_##name(in, sample, out, sample+offset, volume); \
            ++sample;                                                   \
          }                                                             \
        }                                                               \
        offset += samples;                                              \
      }}                                                                \
      break;                                                            \
    default:                                                            \
      mixed_err(MIXED_UNKNOWN_LAYOUT);                                  \
      break;                                                            \
    }                                                                   \
  }

DEF_MIXED_BUFFER_TO_PACKED(int8);
DEF_MIXED_BUFFER_TO_PACKED(uint8);
DEF_MIXED_BUFFER_TO_PACKED(int16);
DEF_MIXED_BUFFER_TO_PACKED(uint16);
DEF_MIXED_BUFFER_TO_PACKED(int24);
DEF_MIXED_BUFFER_TO_PACKED(uint24);
DEF_MIXED_BUFFER_TO_PACKED(int32);
DEF_MIXED_BUFFER_TO_PACKED(uint32);
DEF_MIXED_BUFFER_TO_PACKED(float);
DEF_MIXED_BUFFER_TO_PACKED(double);

MIXED_EXPORT int mixed_buffer_to_packed_audio(struct mixed_buffer **ins, struct mixed_packed_audio *out, size_t samples, float volume){
  switch(out->encoding){
  case MIXED_INT8:
    mixed_buffer_to_int8_packed(ins, out, samples, volume);
    break;
  case MIXED_UINT8:
    mixed_buffer_to_uint8_packed(ins, out, samples, volume);
    break;
  case MIXED_INT16:
    mixed_buffer_to_int16_packed(ins, out, samples, volume);
    break;
  case MIXED_UINT16:
    mixed_buffer_to_uint16_packed(ins, out, samples, volume);
    break;
  case MIXED_INT24:
    mixed_buffer_to_int24_packed(ins, out, samples, volume);
    break;
  case MIXED_UINT24:
    mixed_buffer_to_uint24_packed(ins, out, samples, volume);
    break;
  case MIXED_INT32:
    mixed_buffer_to_int32_packed(ins, out, samples, volume);
    break;
  case MIXED_UINT32:
    mixed_buffer_to_uint32_packed(ins, out, samples, volume);
    break;
  case MIXED_FLOAT:
    mixed_buffer_to_float_packed(ins, out, samples, volume);
    break;
  case MIXED_DOUBLE:
    mixed_buffer_to_double_packed(ins, out, samples, volume);
    break;
  default:
    mixed_err(MIXED_UNKNOWN_ENCODING);
    break;
  }
  return (mixed_error() == MIXED_NO_ERROR);
}
