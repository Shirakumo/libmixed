#include "../internal.h"

struct delay_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct mixed_buffer buffer;
  float time;
  uint32_t samplerate;
};

int delay_segment_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free_buffer(&((struct delay_segment_data *)segment->data)->buffer);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int delay_segment_start(struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;
  if(data->in == 0 || data->out == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  // Fill the entire buffer with nothing to initiate the delay.
  mixed_buffer_clear(&data->buffer);
  uint32_t samples = UINT32_MAX;
  float *out;
  mixed_buffer_request_write(&out, &samples, &data->buffer);
  for(uint32_t i=0; i<samples; ++i){
    out[i] = 0.0f;
  }
  mixed_buffer_finish_write(samples, &data->buffer);
  return 1;
}

int delay_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;

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

int delay_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;

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

int delay_segment_mix(struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;

  mixed_buffer_transfer(&data->buffer, data->out);
  mixed_buffer_transfer(data->in, &data->buffer);

  return 1;
}

int delay_segment_mix_bypass(struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int delay_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "delay";
  info->description = "Delay the output by some time.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_DELAY_TIME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The time, in seconds, by which the output is delayed.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int delay_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;
  switch(field){
  case MIXED_DELAY_TIME: *((float *)value) = data->time; break;
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == delay_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int delay_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct delay_segment_data *data = (struct delay_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
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
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = delay_segment_mix_bypass;
    }else{
      segment->mix = delay_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_delay(float time, uint32_t samplerate, struct mixed_segment *segment){
  struct delay_segment_data *data = mixed_calloc(1, sizeof(struct delay_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  if(!mixed_make_buffer(ceil(time * samplerate), &data->buffer)){
    mixed_free(data);
    return 0;
  }

  data->time = time;
  data->samplerate = samplerate;
  
  segment->free = delay_segment_free;
  segment->start = delay_segment_start;
  segment->mix = delay_segment_mix;
  segment->set_in = delay_segment_set_in;
  segment->set_out = delay_segment_set_out;
  segment->info = delay_segment_info;
  segment->get = delay_segment_get;
  segment->set = delay_segment_set;
  segment->data = data;
  return 1;
}

int __make_delay(void *args, struct mixed_segment *segment){
  return mixed_make_segment_delay(ARG(float, 0), ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(delay, __make_delay, 2, {
    {.description = "time", .type = MIXED_FLOAT},
    {.description = "samplerate", .type = MIXED_UINT32}})
