#include "../internal.h"

struct pitch_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct fft_window_data fft_window_data;
  uint32_t samplerate;
  float pitch;
  float mix;
};

int pitch_segment_free(struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  if(data){
    free_fft_window_data(&data->fft_window_data);
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int pitch_segment_start(struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int pitch_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

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

int pitch_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

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

int pitch_segment_mix(struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

  if(data->pitch == 1.0){
    mixed_buffer_transfer(data->in, data->out);
  }else{
    float *restrict in, *restrict out;
    uint32_t samples = UINT32_MAX;
    float mix = data->mix;
    mixed_buffer_request_read(&in, &samples, data->in);
    mixed_buffer_request_write(&out, &samples, data->out);
    fft_window(in, out, samples, &data->fft_window_data, fft_pitch_shift, &data->pitch);
    for(uint32_t i=0; i<samples; ++i){
      out[i] = LERP(in[i], out[i], mix);
    }
    mixed_buffer_finish_read(samples, data->in);
    mixed_buffer_finish_write(samples, data->out);
  }
  return 1;
}

int pitch_segment_mix_bypass(struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int pitch_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "pitch";
  info->description = "Shift the pitch of the audio.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_PITCH_SHIFT,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The amount of change in pitch that is excised.");

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

int pitch_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  switch(field){
  case MIXED_PITCH_SHIFT: *((float *)value) = data->pitch; break;
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_MIX: *((float *)value) = data->mix; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == pitch_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int pitch_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
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
  case MIXED_PITCH_SHIFT:
    if(*(float *)value <= 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->pitch = *(float *)value;
    break;
  case MIXED_MIX:
    if(*(float *)value < 0 || 1 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mix = *(float *)value;
    if(data->mix == 0){
      bool bypass = 1;
      return pitch_segment_set(MIXED_BYPASS, &bypass, segment);
    }else{
      bool bypass = 0;
      return pitch_segment_set(MIXED_BYPASS, &bypass, segment);
    }
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = pitch_segment_mix_bypass;
    }else{
      segment->mix = pitch_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_pitch(float pitch, uint32_t samplerate, struct mixed_segment *segment){
  struct pitch_segment_data *data = mixed_calloc(1, sizeof(struct pitch_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  if(!make_fft_window_data(2048, 4, samplerate, &data->fft_window_data)){
    mixed_free(data);
    return 0;
  }

  data->pitch = pitch;
  data->samplerate = samplerate;
  data->mix = 1.0;
  
  segment->free = pitch_segment_free;
  segment->start = pitch_segment_start;
  segment->mix = pitch_segment_mix;
  segment->set_in = pitch_segment_set_in;
  segment->set_out = pitch_segment_set_out;
  segment->info = pitch_segment_info;
  segment->get = pitch_segment_get;
  segment->set = pitch_segment_set;
  segment->data = data;
  return 1;
}

int __make_pitch(void *args, struct mixed_segment *segment){
  return mixed_make_segment_pitch(ARG(float, 0), ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(pitch, __make_pitch, 2, {
    {.description = "pitch", .type = MIXED_FLOAT},
    {.description = "samplerate", .type = MIXED_UINT32}})
