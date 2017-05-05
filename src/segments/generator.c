#include "internal.h"

struct generator_segment_data{
  struct mixed_buffer *out;
  enum mixed_generator_type type;
  size_t frequency;
  size_t phase;
  size_t samplerate;
};

int generator_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

int generator_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  switch(location){
  case MIXED_MONO: data->out = buffer; return 1;
  default: mixed_err(MIXED_INVALID_BUFFER_LOCATION); return 0; break;
  }
}

float sine_wave(size_t position, size_t length){
  return sinf(2 * M_PI * ((float)position) / ((float)length));
}

float square_wave(size_t position, size_t length){
  return (position < length/2)? 1.0f : -1.0f;
}

float triangle_wave(size_t position, size_t length){
  float temp = ((float)position) / ((float)length);
  float abs  = (0.5 < temp)? 1.0-temp : temp;
  return abs*4.0 - 1.0;
}

float sawtooth_wave(size_t position, size_t length){
  return ((float)position) / ((float)length)*2.0 - 1.0;
}

int generator_segment_mix(size_t samples, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  size_t length = data->samplerate / data->frequency;
  size_t position = data->phase;
  float (*generator)(size_t position, size_t length) = 0;
  float *out = data->out->data;

  switch(data->type){
  case MIXED_SINE: generator = sine_wave; break;
  case MIXED_SQUARE: generator = square_wave; break;
  case MIXED_TRIANGLE: generator = triangle_wave; break;
  case MIXED_SAWTOOTH: generator = sawtooth_wave; break;
  }
  
  for(size_t i=0; i<samples; ++i){
    out[i] = generator(position, length);
    position = (position+1) % length;
  }

  data->phase = position;
  return 1;
}

struct mixed_segment_info generator_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "generator";
  info.description = "Wave generator source segment";
  info.min_inputs = 0;
  info.max_inputs = 0;
  info.outputs = 1;
  return info;
}

int generator_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  switch(field){
  case MIXED_GENERATOR_FREQUENCY: *((size_t *)value) = data->frequency; break;
  case MIXED_GENERATOR_TYPE: *((enum mixed_generator_type *)value) = data->type; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int generator_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  switch(field){
  case MIXED_GENERATOR_FREQUENCY: data->frequency = *(size_t *)value; break;
  case MIXED_GENERATOR_TYPE: data->type = *(enum mixed_generator_type *)value; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int mixed_make_segment_generator(enum mixed_generator_type type, size_t frequency, size_t samplerate, struct mixed_segment *segment){
  struct generator_segment_data *data = calloc(1, sizeof(struct generator_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->frequency = frequency;
  data->type = type;
  data->samplerate = samplerate;
  
  segment->free = generator_segment_free;
  segment->mix = generator_segment_mix;
  segment->set_out = generator_segment_set_out;
  segment->info = generator_segment_info;
  segment->get = generator_segment_get;
  segment->set = generator_segment_set;
  segment->data = data;
  return 1;
}
