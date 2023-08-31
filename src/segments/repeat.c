#include "../internal.h"

struct repeat_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  float *buffer;
  uint32_t buffer_size;
  uint32_t buffer_index;
  float time;
  uint32_t samplerate;
  enum mixed_repeat_mode mode;
  uint32_t fade_position;
  uint32_t fade_length;
};

int repeat_segment_data_resize_buffer(struct repeat_segment_data *data, uint32_t new_size) {
  float *new_buffer = crealloc(data->buffer, data->buffer_size, new_size, sizeof(float));
  if (!new_buffer){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  data->buffer = new_buffer;
  data->buffer_size = new_size;
  data->buffer_index = 0;
  return 1;
}

int repeat_segment_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free(((struct repeat_segment_data *)segment->data)->buffer);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int repeat_segment_start(struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  if (data->in == 0 || data->out == 0) {
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  data->buffer_index = 0;
  return 1;
}

int repeat_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
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

int repeat_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
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

int repeat_segment_mix(struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

  uint32_t repeat_samples = data->buffer_size;
  uint32_t index = data->buffer_index;
  float *buf = data->buffer;

  float *in, *out;
  uint32_t samples = UINT32_MAX;
  if (data->mode == MIXED_RECORD_ONCE) {
    samples = repeat_samples - index;
  }

  mixed_buffer_request_read(&in, &samples, data->in);
  mixed_buffer_request_write(&out, &samples, data->out);
  for(uint32_t i=0; i<samples; ++i){
    buf[index] = LERP(buf[index], in[i], data->fade_position/(float)data->fade_length);
    if (data->mode == MIXED_PLAY) {
      if (data->fade_position > 0) {
        data->fade_position--;
      }
    } else {
      if (data->fade_position < data->fade_length) {
        data->fade_position++;
      }
    }
    out[i] = buf[index];
    index = (index+1)%repeat_samples;
  }
  mixed_buffer_finish_read(samples, data->in);
  mixed_buffer_finish_write(samples, data->out);

  if (data->mode == MIXED_RECORD_ONCE && index == 0 && samples > 0) {
    data->mode = MIXED_PLAY;
  }

  data->buffer_index = index;
  return 1;
}

int repeat_segment_mix_bypass(struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;

  return mixed_buffer_transfer(data->in, data->out);
}

int repeat_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);

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
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_REPEAT_MODE,
                 MIXED_REPEAT_MODE_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The current mode the repeater segment is in.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  clear_info_field(field++);
  return 1;
}

int repeat_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  switch(field){
  case MIXED_REPEAT_TIME: *((float *)value) = data->time; break;
  case MIXED_REPEAT_MODE: *((enum mixed_repeat_mode *)value) = data->mode; break;
  case MIXED_REPEAT_POSITION: *((float *)value) = data->buffer_index/(float)data->samplerate; break;
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == repeat_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int repeat_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct repeat_segment_data *data = (struct repeat_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    uint32_t samplerate = *(uint32_t *)value;
    if (!repeat_segment_data_resize_buffer(data, ceil(samplerate * data->time))){
      return 0;
    }
    data->samplerate = samplerate;
    break;
  case MIXED_REPEAT_TIME:
    if(*(float *)value <= 0.0f){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    float time = *(float *)value;
    if (!repeat_segment_data_resize_buffer(data, ceil(data->samplerate * time))){
      return 0;
    }
    data->time = time;
    break;
  case MIXED_REPEAT_MODE:
    if(*(enum mixed_repeat_mode *)value != MIXED_RECORD &&
       *(enum mixed_repeat_mode *)value != MIXED_PLAY &&
       *(enum mixed_repeat_mode *)value != MIXED_RECORD_ONCE){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mode = *(enum mixed_repeat_mode *)value;
    break;
  case MIXED_REPEAT_POSITION: {
    float position = (*(float *)value);
    if(position < 0.0f){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    uint32_t index = ceil(position * data->samplerate);
    if(index >= data->buffer_size){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->buffer_index = index;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = repeat_segment_mix_bypass;
    }else{
      segment->mix = repeat_segment_mix;
    }
    break;}
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_repeat(float time, uint32_t samplerate, struct mixed_segment *segment){
  if(time <= 0.0f){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }

  struct repeat_segment_data *data = mixed_calloc(1, sizeof(struct repeat_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->buffer_size = ceil(time * samplerate);
  data->buffer = mixed_calloc(data->buffer_size, sizeof(float));
  if(!data->buffer){
    mixed_err(MIXED_OUT_OF_MEMORY);
    mixed_free(data);
    return 0;
  }

  data->mode = MIXED_RECORD;
  data->time = time;
  data->samplerate = samplerate;
  data->fade_position = 0;
  data->fade_length = 100;

  segment->free = repeat_segment_free;
  segment->start = repeat_segment_start;
  segment->mix = repeat_segment_mix;
  segment->set_in = repeat_segment_set_in;
  segment->set_out = repeat_segment_set_out;
  segment->info = repeat_segment_info;
  segment->get = repeat_segment_get;
  segment->set = repeat_segment_set;
  segment->data = data;
  return 1;
}

int __make_repeat(void *args, struct mixed_segment *segment){
  return mixed_make_segment_repeat(ARG(float, 0), ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(repeat, __make_repeat, 2, {
    {.description = "time", .type = MIXED_FLOAT},
    {.description = "samplerate", .type = MIXED_UINT32}})
