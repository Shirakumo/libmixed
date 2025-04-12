#include "../internal.h"

struct convolution_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct fft_window_data fft_window_data;
  uint32_t samplerate;
  float mix;
  float *fir;
};

int convolution_segment_free(struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;
  if(data){
    if(data->fir){
      free(data->fir);
      data->fir = 0;
    }
    free_fft_window_data(&data->fft_window_data);
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int convolution_segment_start(struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int convolution_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->in = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int convolution_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->out = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int convolution_segment_mix(struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;

  float *in, *out;
  uint32_t samples = UINT32_MAX;
  float mix = data->mix;
  mixed_buffer_request_read(&in, &samples, data->in);
  mixed_buffer_request_write(&out, &samples, data->out);
  fft_window(in, out, samples, &data->fft_window_data, fft_convolve, data->fir);
  for(uint32_t i=0; i<samples; ++i){
    out[i] = LERP(in[i], out[i], mix);
  }
  mixed_buffer_finish_read(samples, data->in);
  mixed_buffer_finish_write(samples, data->out);
  return 1;
}

int convolution_segment_mix_bypass(struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int convolution_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "convolution";
  info->description = "Convolve the audio signal with a finite input response.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_MIX,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "How much of the output to mix with the input.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int convolution_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_MIX: *((float *)value) = data->mix; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == convolution_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int convolution_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct convolution_segment_data *data = (struct convolution_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
    free_fft_window_data(&data->fft_window_data);
    if(!make_fft_window_data(2048, 4, data->samplerate, &data->fft_window_data)){
      return 0;
    }
    break;
  case MIXED_MIX:
    if(*(float *)value < 0 || 1 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mix = *(float *)value;
    if(data->mix == 0){
      bool bypass = 1;
      return convolution_segment_set(MIXED_BYPASS, &bypass, segment);
    }else{
      bool bypass = 0;
      return convolution_segment_set(MIXED_BYPASS, &bypass, segment);
    }
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = convolution_segment_mix_bypass;
    }else{
      segment->mix = convolution_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

uint32_t next_power2(uint32_t v){
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

MIXED_EXPORT int mixed_make_segment_convolution(float *fir, uint32_t fir_size, uint32_t samplerate, struct mixed_segment *segment){
  struct convolution_segment_data *data = mixed_calloc(1, sizeof(struct convolution_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  uint32_t framesize = 0;
  for(uint32_t i=0; i<fir_size; ++i){
    if(0.000001f < fir[i]) framesize = i;
  }
  framesize = MIN(next_power2(framesize), 8192);
  
  if(!make_fft_window_data(framesize, 4, samplerate, &data->fft_window_data)){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  data->fir = mixed_calloc(framesize, sizeof(float));
  if(!data->fir){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  if(!mixed_fwd_fft(framesize, fir, data->fir))
    goto cleanup;

  data->samplerate = samplerate;
  data->mix = 1.0;
  
  segment->free = convolution_segment_free;
  segment->start = convolution_segment_start;
  segment->mix = convolution_segment_mix;
  segment->set_in = convolution_segment_set_in;
  segment->set_out = convolution_segment_set_out;
  segment->info = convolution_segment_info;
  segment->get = convolution_segment_get;
  segment->set = convolution_segment_set;
  segment->data = data;
  return 1;

 cleanup:
  free_fft_window_data(&data->fft_window_data);
  if(data->fir) free(data->fir);
  mixed_free(data);
  return 0;
}

int __make_convolution(void *args, struct mixed_segment *segment){
  return mixed_make_segment_convolution(ARG(float *, 1), ARG(uint32_t, 2), ARG(uint32_t, 3), segment);
}

REGISTER_SEGMENT(convolution, __make_convolution, 3, {
  {.description = "fir", .type = MIXED_POINTER},
  {.description = "fir_size", .type = MIXED_UINT32},
  {.description = "samplerate", .type = MIXED_UINT32}})
