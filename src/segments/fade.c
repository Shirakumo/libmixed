#include "internal.h"

struct fade_segment_data{
  struct mixed_buffer **in;
  struct mixed_buffer **out;
  size_t count;
  float from;
  float to;
  float time;
  enum mixed_fade_type type;
  size_t samplerate;
  float time_passed;
};

int fade_segment_free(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  if(data){
    if(data->in)
      free(data->in);
    if(data->out)
      free(data->out);
    free(data);
  }
  segment->data = 0;
  return 1;
}

int fade_segment_set_in(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  if(location < data->count){
    data->in[location] = buffer;
  }
  mixed_err(MIXED_INVALID_BUFFER_LOCATION);
  return 0;
}

int fade_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  if(location < data->count){
    data->out[location] = buffer;
  }
  mixed_err(MIXED_INVALID_BUFFER_LOCATION);
  return 0;
}

float fade_ease(float from, float to, float x, size_t type){
  float factor = 1.0f;
  float temp = 0.0f;
  switch(type){
  case MIXED_LINEAR:
    factor = x;
  case MIXED_CUBIC_IN:
    factor = x*x*x;
  case MIXED_CUBIC_OUT:
    temp = x-1.0f;
    factor = temp*temp*temp+1.0f;
  case MIXED_CUBIC_IN_OUT:
    if(factor < 0.5f){
      temp = 2.0f*x;
      factor = temp*temp*temp/2.0f;
    }else{
      temp = 2.0f*(x-1.0f);
      factor = temp*temp*temp/2.0f+1.0f;
    }
  default:
    factor = 1.0f;
  }
  return from+(to-from)*factor;
}

int fade_segment_mix(size_t samples, struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;

  double time;
  float endtime = data->time;
  float from = data->from;
  float to = data->to;
  double sampletime = 1.0f/data->samplerate;
  enum mixed_fade_type type = data->type;
  size_t channels = data->count;
  // NOTE: You could probably get away with having the same fade factor
  //       for the entirety of the sample range if the total duration
  //       of the buffer is small enough (~1ms?) as the human ear
  //       wouldn't be able to properly notice it.
  for(size_t c=0; c<channels; ++c){
    float *in = data->in[c]->data;
    float *out = data->out[c]->data;
    time = data->time_passed;
    for(size_t i=0; i<samples; ++i){
      float x = (time < endtime)? time/endtime : 1.0f;
      // FIXME: Could optimise the easing function dispatch out since
      //        it cannot be changed between start/end anyway.
      float fade = fade_ease(from, to, x, type);
      out[i] = in[i]*fade;
      time += sampletime;
    }
  }

  data->time_passed = time;
  return 1;
}

struct mixed_segment_info fade_segment_info(struct mixed_segment *segment){
  struct fade_segment_data *data = (struct fade_segment_data *)segment->data;
  struct mixed_segment_info info = {0};
  info.name = "fade";
  info.description = "Fade the volume of buffers.";
  info.flags = MIXED_INPLACE;
  info.min_inputs = data->count;
  info.max_inputs = data->count;
  info.outputs = data->count;
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

int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, size_t samplerate, struct mixed_segment *segment){
  struct fade_segment_data *data = calloc(1, sizeof(struct fade_segment_data));
  if(!data) return 0;

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
