#include <string.h>
#include "mixed.h"

int mixed_make_buffer(struct mixed_buffer *buffer){
  buffer->data = calloc(buffer->size, sizeof(float));
  if(!buffer->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  return 1;
}

void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->data)
    free(buffer->data);
  buffer->data = 0;
}

inline void mixed_transfer_sample_from(struct mixed_channel *in, size_t is, struct mixed_buffer *out, size_t os){
  switch(in->encoding){
  MIXED_INT8:
    out->data[os] = mixed_from_int8(((int8_t *)in->data)[is]);
    break;
  MIXED_UINT8:
    out->data[os] = mixed_from_uint8(((uint8_t *)in->data)[is]);
    break;
  MIXED_INT16:
    out->data[os] = mixed_from_int16(((int16_t *)in->data)[is]);
    break;
  MIXED_UINT16:
    out->data[os] = mixed_from_uint16(((uint16_t *)in->data)[is]);
    break;
  MIXED_INT24:
    // FIXME
    int24_t sample = 0;
    out->data[os] = mixed_from_int24(sample);
    break;
  MIXED_UINT24:
    int8_t data = (int8_t *)in->data;
    uint24_t sample = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
    out->data[os] = mixed_from_uint24(sample);
    break;
  MIXED_INT32:
    out->data[os] = mixed_from_int32(((int32_t *)in->data)[is]);
    break;
  MIXED_UINT32:
    out->data[os] = mixed_from_uint32(((uint32_t *)in->data)[is]);
    break;
  MIXED_FLOAT:
    out->data[os] = mixed_from_float(((float *)in->data)[is]);
    break;
  MIXED_DOUBLE:
    out->data[os] = mixed_from_double(((double *)in->data)[is]);
    break;
  default:
    mixed_err(MIXED_UNKNOWN_ENCODING);
    break;
  }
}

// We assume little endian for all formats.
inline void mixed_transfer_sample_to(struct mixed_buffer *in, size_t is, struct mixed_channel *out, size_t os){
  switch(in->encoding){
  MIXED_INT8:
    ((int8_t *)out->data)[os] = mixed_to_int8(in->data[is]);
    break;
  MIXED_UINT8:
    ((uint8_t *)out->data)[os] = mixed_to_uint8(in->data[is]);
    break;
  MIXED_INT16:
    ((int16_t *)out->data)[os] = mixed_to_int16(in->data[is]);
    break;
  MIXED_UINT16:
    ((uint16_t *)out->data)[os] = mixed_to_uint16(in->data[is]);
    break;
  MIXED_INT24:
    // FIXME
    int24_t sample = mixed_to_int24(sample);
    ((uint8_t *)out->data)[os];
    break;
  MIXED_UINT24:
    uint24_t sample = mixed_to_uint24(in->data);
    ((uint8_t *)out->data)[os+2] = (sample >> 16) & 0xFF;
    ((uint8_t *)out->data)[os+1] = (sample >>  8) & 0xFF;
    ((uint8_t *)out->data)[os+0] = (sample >>  0) & 0xFF;
    break;
  MIXED_INT32:
    ((int32_t *)out->data)[os] = mixed_to_int32(in->data[is]);
    break;
  MIXED_UINT32:
    ((uint32_t *)out->data)[os] = mixed_to_uint32(in->data[is]);
    break;
  MIXED_FLOAT:
    ((float *)out->data)[os] = mixed_to_float(in->data[is]);
    break;
  MIXED_DOUBLE:
    ((double *)out->data)[os] = mixed_to_double(in->data[is]);
    break;
  default:
    mixed_err(MIXED_UNKNOWN_ENCODING);
    break;
  }
}

// FIXME: Naturally, rolling the transfer functions out into the outer
//        loops so that we don't need to do the swich for every sample
//        would be much faster, but I ain't doin' any o dat.
// FIXME: We currently don't do any resampling because doing so is a
//        huge pain and I currently don't want to think about it.
//        Probably will want to conver to an internal float buffer first.
int mixed_buffer_from_channel(struct mixed_channel *in, struct mixed_buffer **outs){
  mixed_err(MIXED_NO_ERROR);
  switch(in->layout){
  case MIXED_ALTERNATING:
    uint8_t channel = 0;
    size_t i = 0;
    for(size_t sample=0; sample<in->size; ++sample){
      struct mixed_buffer *out = outs[channel];
      mixed_transfer_sample_from(in, sample, out, i);
      ++channel;
      if(in->channels == channel){
        channel = 0;
        ++i;
      }
    }
    break;
  case MIXED_SEQUENTIAL:
    size_t chansize = in->size / in->channels;
    size_t sample = 0;
    for(uint8_t channel=0; channel<in->channels; ++channel){
      struct mixed_buffer *out = outs[channel];
      for(size_t i=0; i<chansize; ++i){
        mixed_transfer_sample_from(in, sample, out, i);
        ++sample;
      }
    }
    break;
  default:
    mixed_err(MIXED_UNKNOWN_LAYOUT);
    break;
  }
  return (mixed_error() == MIXED_NO_ERROR);
}

int mixed_buffer_to_channel(struct mixed_buffer **ins, struct mixed_channel *out){
  mixed_err(MIXED_NO_ERROR);
  switch(out->layout){
  case MIXED_ALTERNATING:
    uint8_t channel = 0;
    size_t i = 0;
    for(size_t sample=0; sample<out->size; ++sample){
      struct mixed_buffer *in = ins[channel];
      mixed_transfer_sample_to(in, i, out, sample);
      ++channel;
      if(out->channels == channel){
        channel = 0;
        ++i;
      }
    }
    break;
  case MIXED_SEQUENTIAL:
    size_t chansize = out->size / out->channels;
    size_t sample = 0;
    for(uint8_t channel=0; channel<out->channels; ++channel){
      struct mixed_buffer *in = ins[channel];
      for(size_t i=0; i<chansize; ++i){
        mixed_transfer_sample_to(in, i, out, sample);
        ++sample;
      }
    }
    break;
  default:
    mixed_err(MIXED_UNKNOWN_LAYOUT);
    break;
  }
  return (mixed_error() == MIXED_NO_ERROR);
}

int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to){
  if(from != to){
    size_t size = (to->size<from->size)? from->size : to->size;
    memcpy(to->data, from->data, sizeof(float)*size);
    for(size_t i=size; size<to->size; ++i){
      to->data[i] = 0.0f;
    }
  }
  return 1;
}
