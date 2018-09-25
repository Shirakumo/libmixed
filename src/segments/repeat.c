#include "internal.h"

struct repeat_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct mixed_buffer buffer;
  size_t buffer_index;
  float time;
  size_t samplerate;
  enum mixed_repeat_mode mode;
};

int repeat_segment_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free_buffer(&((struct repeat_segment_data *)segment->data)->buffer);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int repeat_segment_start(struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  data->buffer_index = 0.0;
  mixed_buffer_clear(&data->buffer);
  return 1;
}

int repeat_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

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

int repeat_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

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

int repeat_segment_mix_record(size_t samples, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

  size_t repeat_samples = data->buffer.size;
  size_t index = data->buffer_index;

  float *buf = data->buffer.data;
  float *in = data->in->data;
  float *out = data->out->data;
  for(size_t i=0; i<samples; ++i){
    buf[index] = in[i];
    out[i] = buf[index];
    index = (index+1)%repeat_samples;
  }

  data->buffer_index = index;
  return 1;
}

int repeat_segment_mix_play(size_t samples, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

  size_t repeat_samples = data->buffer.size;
  size_t index = data->buffer_index;

  float *buf = data->buffer.data;
  float *out = data->out->data;
  for(size_t i=0; i<samples; ++i){
    out[i] = buf[index];
    index = (index+1)%repeat_samples;
  }

  data->buffer_index = index;
  return 1;
}

int repeat_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  
  return mixed_buffer_copy(data->in, data->out);
}

int repeat_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  
  info->name = "repeat";
  info->description = "Allows recording some input and then repeatedly playing it back.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
    
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_REPEAT_TIME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The time, in seconds, that is recorded and repeated.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int repeat_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  switch(field){
  case MIXED_REPEAT_TIME: *((float *)value) = data->time; break;
  case MIXED_REPEAT_MODE: *((enum mixed_repeat_mode *)value) = data->mode; break;
  case MIXED_SAMPLERATE: *((size_t *)value) = data->samplerate; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == repeat_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int repeat_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(size_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(size_t *)value;
    mixed_buffer_resize(ceil(data->samplerate * data->time), &data->buffer);
    break;
  case MIXED_DELAY_TIME:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->time = *(float *)value;
    mixed_buffer_resize(ceil(data->samplerate * data->time), &data->buffer);
    break;
  case MIXED_REPEAT_MODE:
    if(*(enum mixed_repeat_mode *)value < MIXED_RECORD ||
       MIXED_PLAY < *(enum mixed_repeat_mode *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mode = *(enum mixed_repeat_mode *)value;
    switch(data->mode){
    case MIXED_RECORD: segment->mix = repeat_segment_mix_record; break;
    case MIXED_PLAY: segment->mix = repeat_segment_mix_play; break;
    }
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = repeat_segment_mix_bypass;
    }else{
      switch(data->mode){
      case MIXED_RECORD: segment->mix = repeat_segment_mix_record; break;
      case MIXED_PLAY: segment->mix = repeat_segment_mix_play; break;
      }
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_repeat(float time, size_t samplerate, struct mixed_segment *segment){
  struct repeat_segment_data *data = calloc(1, sizeof(struct repeat_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  if(!mixed_make_buffer(ceil(time * samplerate), &data->buffer)){
    free(data);
    return 0;
  }

  data->mode = MIXED_RECORD;
  data->time = time;
  data->samplerate = samplerate;
  
  segment->free = repeat_segment_free;
  segment->start = repeat_segment_start;
  segment->mix = repeat_segment_mix_record;
  segment->set_in = repeat_segment_set_in;
  segment->set_out = repeat_segment_set_out;
  segment->info = repeat_segment_info;
  segment->get = repeat_segment_get;
  segment->set = repeat_segment_set;
  segment->data = data;
  return 1;
}
