#include "../internal.h"

struct fade_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  float from;
  float to;
  float time;
  enum mixed_fade_type type;
  uint32_t samplerate;
  float time_passed;
};

int fade_segment_free(struct mixed_segment *segment){
  if(segment->data)
    mixed_free(segment->data);
  segment->data = 0;
  return 1;
}

int fade_segment_start(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  data->time_passed = 0.0;

  if(data->in == 0 || data->out == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }

  return 1;
}

int fade_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;

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

int fade_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;

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

float fade_linear(float x){
  return x;
}

float fade_cubic_in(float x){
  return x*x*x;
}

float fade_cubic_out(float x){
  x = x-1.0;
  return x*x*x+1.0;
}

float fade_cubic_in_out(float x){
  if(x < 0.5f){
    x = 2.0*x;
    return x*x*x/2.0;
  }else{
    x = 2.0*(x-1.0);
    return x*x*x/2.0+1.0;
  }
}

VECTORIZE int fade_segment_mix(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;

  double time = data->time_passed;
  float endtime = data->time;
  float from = data->from;
  float range = data->to - data->from;
  double sampletime = 1.0f/data->samplerate;
  
  float (*ease)(float x);
  switch(data->type){
  case MIXED_LINEAR: ease = fade_linear; break;
  case MIXED_CUBIC_IN: ease = fade_cubic_in; break;
  case MIXED_CUBIC_OUT: ease = fade_cubic_out; break;
  case MIXED_CUBIC_IN_OUT: ease = fade_cubic_in_out; break;
  default: ease = fade_linear; break;
  }
  
  // NOTE: You could probably get away with having the same fade factor
  //       for the entirety of the sample range if the total duration
  //       of the buffer is small enough (~1ms?) as the human ear
  //       wouldn't be able to properly notice it.
  with_mixed_buffer_transfer(i, samples, in, data->in, out, data->out, {
      float x = (time < endtime)? time/endtime : 1.0f;
      float fade = from+ease(x)*range;
      out[i] = in[i]*fade;
      time += sampletime;
    });
  data->time_passed = time;
  return 1;
}

int fade_segment_mix_bypass(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int fade_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  info->name = "fade";
  info->description = "Fade the volume of buffers.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_FADE_FROM,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The starting volume from which the fade begins.");

  set_info_field(field++, MIXED_FADE_TO,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The ending volume at which the fade ends.");

  set_info_field(field++, MIXED_FADE_TIME,
                 MIXED_DURATION_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The number of seconds it takes to fade.");

  set_info_field(field++, MIXED_FADE_TYPE,
                 MIXED_FADE_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The type of easing function used to fade.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  clear_info_field(field++);
  return 1;
}

int fade_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  switch(field){
  case MIXED_FADE_FROM: *((float *)value) = data->from; break;
  case MIXED_FADE_TO: *((float *)value) = data->to; break;
  case MIXED_FADE_TIME: *((float *)value) = data->time; break;
  case MIXED_FADE_TYPE: *((enum mixed_fade_type *)value) = data->type; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == fade_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int fade_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  switch(field){
  case MIXED_FADE_FROM:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->from = *(float *)value;
    break;
  case MIXED_FADE_TO:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->to = *(float *)value;
    break;
  case MIXED_FADE_TIME:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->time = *(float *)value;
    break;
  case MIXED_FADE_TYPE:
    if(*(enum mixed_fade_type *)value < MIXED_LINEAR ||
       MIXED_CUBIC_IN_OUT < *(enum mixed_fade_type *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->type = *(enum mixed_fade_type *)value;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = fade_segment_mix_bypass;
    }else{
      segment->mix = fade_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, uint32_t samplerate, struct mixed_segment *segment){
  struct fade_segment_data *data = mixed_calloc(1, sizeof(struct fade_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->from = from;
  data->to = to;
  data->time = time;
  data->type = type;
  data->samplerate = samplerate;
  
  segment->free = fade_segment_free;
  segment->start = fade_segment_start;
  segment->mix = fade_segment_mix;
  segment->set_in = fade_segment_set_in;
  segment->set_out = fade_segment_set_out;
  segment->info = fade_segment_info;
  segment->get = fade_segment_get;
  segment->set = fade_segment_set;
  segment->data = data;
  return 1;
}

int __make_fade(void *args, struct mixed_segment *segment){
  return mixed_make_segment_fade(ARG(float, 0), ARG(float, 1), ARG(float, 2), ARG(enum mixed_fade_type, 3), ARG(uint32_t, 4), segment);
}

REGISTER_SEGMENT(fade, __make_fade, 5, {
    {.description = "from", .type = MIXED_FLOAT},
    {.description = "to", .type = MIXED_FLOAT},
    {.description = "time", .type = MIXED_FLOAT},
    {.description = "type", .type = MIXED_FADE_TYPE_ENUM},
    {.description = "samplerate", .type = MIXED_UINT32}})
