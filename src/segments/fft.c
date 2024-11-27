#include "../internal.h"

struct fft_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  uint32_t samplerate;
  uint32_t framesize;
  uint32_t oversampling;
};

int fft_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free_fft_data(&((struct fft_segment_data *)segment->data)->fft_data);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int fft_segment_start(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int fft_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

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

int fft_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

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

int fft_segment_fwd(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

  float *in, *out;
  uint32_t samples = UINT32_MAX;
  mixed_buffer_request_read(&in, &samples, data->in);
  mixed_buffer_request_write(&out, &samples, data->out);

  
  
  mixed_buffer_finish_read(samples, data->in);
  mixed_buffer_finish_write(samples, data->out);
  return 1;
}

int fft_segment_inv(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

  float *in, *out;
  uint32_t samples = UINT32_MAX;
  mixed_buffer_request_read(&in, &samples, data->in);
  mixed_buffer_request_write(&out, &samples, data->out);

  
  
  mixed_buffer_finish_read(samples, data->in);
  mixed_buffer_finish_write(samples, data->out);
  return 1;
}

int fft_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "fft";
  info->description = "Perform a Fast Fourier Transform";
  info->flags = 0;
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

  set_info_field(field++, MIXED_FRAMESIZE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The FFT moving window frame size.");

  set_info_field(field++, MIXED_OVERSAMPLING,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The FFT oversampling rate.");
  
  clear_info_field(field++);
  return 1;
}

int fft_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_FRAMESIZE: *((uint32_t *)value) = data->framesize; break;
  case MIXED_OVERSAMPLING: *((uint32_t *)value) = data->oversampling; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int fft_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
    break;
  case MIXED_FRAMESIZE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->framesize = *(uint32_t *)value;
    break;
  case MIXED_OVERSAMPLING:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->oversampling = *(uint32_t *)value;
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  // TODO: Realloc
  return 1;
}

MIXED_EXPORT int mixed_make_segment_fft_fwd(uint32_t samplerate, uint32_t framesize, uint32_t oversampling, struct mixed_segment *segment){
  struct fft_segment_data *data = mixed_calloc(1, sizeof(struct fft_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->samplerate = samplerate;
  data->framesize = framesize;
  data->oversampling = oversampling
  
  segment->free = fft_segment_free;
  segment->start = fft_segment_start;
  segment->mix = fft_segment_fwd;
  segment->set_in = fft_segment_set_in;
  segment->set_out = fft_segment_set_out;
  segment->info = fft_segment_info;
  segment->get = fft_segment_get;
  segment->set = fft_segment_set;
  segment->data = data;
  return 1;
}

MIXED_EXPORT int mixed_make_segment_fft_inv(uint32_t samplerate, uint32_t framesize, uint32_t oversampling, struct mixed_segment *segment){
  int ret = mixed_make_segment_fft_fwd(uint32_t samplerate, uint32_t framesize, uint32_t oversampling, struct mixed_segment *segment);
  segment->mix = fft_segment_inv;
  return ret;
}

int __make_fft_fwd(void *args, struct mixed_segment *segment){
  return mixed_make_segment_fft_fwd(ARG(uint32_t, 1), ARG(uint32_t, 2), ARG(uint32_t, 3), segment);
}

REGISTER_SEGMENT(fft, __make_fft_fwd, 3, {
    {.description = "samplerate", .type = MIXED_UINT32},
    {.description = "framesize", .type = MIXED_UINT32},
    {.description = "oversampling", .type = MIXED_UINT32}})

int __make_fft_inv(void *args, struct mixed_segment *segment){
  return mixed_make_segment_fft_inv(ARG(uint32_t, 1), ARG(uint32_t, 2), ARG(uint32_t, 3), segment);
}

REGISTER_SEGMENT(fft, __make_fft_inv, 3, {
    {.description = "samplerate", .type = MIXED_UINT32},
    {.description = "framesize", .type = MIXED_UINT32},
    {.description = "oversampling", .type = MIXED_UINT32}})
