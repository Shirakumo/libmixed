#include "internal.h"

int mixed_make_buffer(size_t size, struct mixed_buffer *buffer){
  buffer->data = calloc(size, sizeof(float));
  if(!buffer->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->size = size;
  return 1;
}

void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->data)
    free(buffer->data);
  buffer->data = 0;
}

extern inline void mixed_transfer_sample_from(struct mixed_channel *in, size_t is, struct mixed_buffer *out, size_t os){
  switch(in->encoding){
  case MIXED_INT8:
    out->data[os] = mixed_from_int8(((int8_t *)in->data)[is]);
    break;
  case MIXED_UINT8:
    out->data[os] = mixed_from_uint8(((uint8_t *)in->data)[is]);
    break;
  case MIXED_INT16:
    out->data[os] = mixed_from_int16(((int16_t *)in->data)[is]);
    break;
  case MIXED_UINT16:
    out->data[os] = mixed_from_uint16(((uint16_t *)in->data)[is]);
    break;
  case MIXED_INT24:{
      int8_t *data = (int8_t *)in->data;
      // Read in as uint
      uint32_t temp = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
      // Shift up to fill 32 bit range
      temp = temp << 8;
      // Convert to signed, this is actually not standards compliant.
      int32_t sample = (int32_t)temp;
      // Shift down again to get back to 24 bit range.
      out->data[os] = mixed_from_int24(sample >> 8);
    }
    break;
  case MIXED_UINT24:{
      int8_t *data = (int8_t *)in->data;
      uint24_t sample = (data[3*is] << 16) + (data[3*is+1] << 8) + (data[3*is+2]);
      out->data[os] = mixed_from_uint24(sample);
    }
    break;
  case MIXED_INT32:
    out->data[os] = mixed_from_int32(((int32_t *)in->data)[is]);
    break;
  case MIXED_UINT32:
    out->data[os] = mixed_from_uint32(((uint32_t *)in->data)[is]);
    break;
  case MIXED_FLOAT:
    out->data[os] = mixed_from_float(((float *)in->data)[is]);
    break;
  case MIXED_DOUBLE:
    out->data[os] = mixed_from_double(((double *)in->data)[is]);
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
//        Probably will want to convert to an internal float buffer first.
int mixed_buffer_from_channel(struct mixed_channel *in, struct mixed_buffer **outs, size_t samples){
  mixed_err(MIXED_NO_ERROR);
  size_t channels = in->channels;
  switch(in->layout){
  case MIXED_ALTERNATING:{
    size_t i = 0;
    for(uint8_t channel=0; channel<channels; ++channel){
      struct mixed_buffer *out = outs[channel];
      for(size_t sample=0; sample<samples; ++sample){
        mixed_transfer_sample_from(in, sample*channels+channel, out, sample);
      }
    }}
    break;
  case MIXED_SEQUENTIAL:{
    size_t offset = 0;
    for(uint8_t channel=0; channel<channels; ++channel){
      struct mixed_buffer *out = outs[channel];
      for(size_t sample=0; sample<samples; ++sample){
        mixed_transfer_sample_from(in, sample+offset, out, sample);
      }
      offset += samples;
    }}
    break;
  default:
    mixed_err(MIXED_UNKNOWN_LAYOUT);
    break;
  }
  return (mixed_error() == MIXED_NO_ERROR);
}

// We assume little endian for all formats.
extern inline void mixed_transfer_sample_to(struct mixed_buffer *in, size_t is, struct mixed_channel *out, size_t os){
  switch(out->encoding){
  case MIXED_INT8:
    ((int8_t *)out->data)[os] = mixed_to_int8(in->data[is]);
    break;
  case MIXED_UINT8:
    ((uint8_t *)out->data)[os] = mixed_to_uint8(in->data[is]);
    break;
  case MIXED_INT16:
    ((int16_t *)out->data)[os] = mixed_to_int16(in->data[is]);
    break;
  case MIXED_UINT16:
    ((uint16_t *)out->data)[os] = mixed_to_uint16(in->data[is]);
    break;
  case MIXED_INT24:{
    int24_t sample = mixed_to_int24(in->data[is]);
    ((uint8_t *)out->data)[os+2] = (sample >> 16) & 0xFF;
    ((uint8_t *)out->data)[os+1] = (sample >>  8) & 0xFF;
    ((uint8_t *)out->data)[os+0] = (sample >>  0) & 0xFF;
  }
    break;
  case MIXED_UINT24:{
    uint24_t sample = mixed_to_uint24(in->data[is]);
    ((uint8_t *)out->data)[os+2] = (sample >> 16) & 0xFF;
    ((uint8_t *)out->data)[os+1] = (sample >>  8) & 0xFF;
    ((uint8_t *)out->data)[os+0] = (sample >>  0) & 0xFF;
  }
    break;
  case MIXED_INT32:
    ((int32_t *)out->data)[os] = mixed_to_int32(in->data[is]);
    break;
  case MIXED_UINT32:
    ((uint32_t *)out->data)[os] = mixed_to_uint32(in->data[is]);
    break;
  case MIXED_FLOAT:
    ((float *)out->data)[os] = mixed_to_float(in->data[is]);
    break;
  case MIXED_DOUBLE:
    ((double *)out->data)[os] = mixed_to_double(in->data[is]);
    break;
  default:
    mixed_err(MIXED_UNKNOWN_ENCODING);
    break;
  }
}

int mixed_buffer_to_channel(struct mixed_buffer **ins, struct mixed_channel *out, size_t samples){
  mixed_err(MIXED_NO_ERROR);
  size_t channels = out->channels;
  switch(out->layout){
  case MIXED_ALTERNATING:{
    for(size_t channel=0; channel<channels; ++channel){
      struct mixed_buffer *in = ins[channel];
      for(size_t sample=0; sample<samples; ++sample){
        mixed_transfer_sample_to(in, sample, out, sample*channels+channel);
      }
    }}
    break;
  case MIXED_SEQUENTIAL:{
    size_t offset = 0;
    for(uint8_t channel=0; channel<channels; ++channel){
      struct mixed_buffer *in = ins[channel];
      for(size_t sample=0; sample<samples; ++sample){
        mixed_transfer_sample_to(in, sample, out, sample+offset);
        ++sample;
      }
      offset += samples;
    }}
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
