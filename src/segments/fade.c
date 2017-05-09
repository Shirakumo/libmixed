#include "internal.h"

struct fade_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  float from;
  float to;
  float time;
  enum mixed_fade_type type;
  size_t samplerate;
  float time_passed;
};

int fade_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

int fade_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
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

int fade_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
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

void fade_segment_mix(size_t samples, struct mixed_segment *segment){
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
  }
  
  // NOTE: You could probably get away with having the same fade factor
  //       for the entirety of the sample range if the total duration
  //       of the buffer is small enough (~1ms?) as the human ear
  //       wouldn't be able to properly notice it.
  float *in = data->in->data;
  float *out = data->out->data;
  for(size_t i=0; i<samples; ++i){
    float x = (time < endtime)? time/endtime : 1.0f;
    float fade = from+ease(x)*range;
    out[i] = in[i]*fade;
    time += sampletime;
  }

  data->time_passed = time;
}

struct mixed_segment_info fade_segment_info(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  struct mixed_segment_info info = {0};
  info.name = "fade";
  info.description = "Fade the volume of buffers.";
  info.flags = MIXED_INPLACE;
  info.min_inputs = 1;
  info.max_inputs = 1;
  info.outputs = 1;
  
  info.fields[0].field = MIXED_BUFFER;
  info.fields[0].description = "The buffer for audio data attached to the location.";
  info.fields[0].flags = MIXED_IN | MIXED_OUT | MIXED_SET;

  info.fields[1].field = MIXED_FADE_FROM;
  info.fields[1].description = "The starting volume from which the fade begins.";
  info.fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  info.fields[2].field = MIXED_FADE_TO;
  info.fields[2].description = "The ending volume at which the fade ends.";
  info.fields[2].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  info.fields[3].field = MIXED_FADE_TIME;
  info.fields[3].description = "The number of seconds it takes to fade.";
  info.fields[3].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  info.fields[4].field = MIXED_FADE_TYPE;
  info.fields[4].description = "The type of easing function used to fade.";
  info.fields[4].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  return info;
}

int fade_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  switch(field){
  case MIXED_FADE_FROM: *((float *)value) = data->from; break;
  case MIXED_FADE_TO: *((float *)value) = data->to; break;
  case MIXED_FADE_TIME: *((float *)value) = data->time; break;
  case MIXED_FADE_TYPE: *((enum mixed_fade_type *)value) = data->type; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int fade_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  switch(field){
  case MIXED_FADE_FROM: data->from = *(float *)value; break;
  case MIXED_FADE_TO: data->to = *(float *)value; break;
  case MIXED_FADE_TIME: data->time = *(float *)value; break;
  case MIXED_FADE_TYPE: data->type = *(enum mixed_fade_type *)value; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, size_t samplerate, struct mixed_segment *segment){
  struct fade_segment_data *data = calloc(1, sizeof(struct fade_segment_data));
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
  segment->mix = fade_segment_mix;
  segment->set_in = fade_segment_set_in;
  segment->set_out = fade_segment_set_out;
  segment->info = fade_segment_info;
  segment->get = fade_segment_get;
  segment->set = fade_segment_set;
  segment->data = data;
  return 1;
}
