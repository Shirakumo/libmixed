#include "../internal.h"

struct generator_segment_data{
  struct mixed_buffer *out;
  enum mixed_generator_type type;
  float frequency;
  double phase;
  uint32_t samplerate;
  float volume;
};

int generator_segment_free(struct mixed_segment *segment){
  if(segment->data)
    mixed_free(segment->data);
  segment->data = 0;
  return 1;
}

int generator_segment_start(struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  if(data->out == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int generator_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_MONO: data->out = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

float sine_wave(float frequency, double phase, float samplerate){
  return sinf(2 * M_PI * frequency * phase / samplerate);
}

float square_wave(float frequency, double phase, float samplerate){
  float length = samplerate / frequency;
  return (fmod(phase, length) < length/2)? 1.0f : -1.0f;
}

float triangle_wave(float frequency, double phase, float samplerate){
  float length = samplerate / frequency;
  float temp = fmod(phase, length) / length;
  float abs  = (0.5 < temp)? 1.0-temp : temp;
  return abs*4.0 - 1.0;
}

float sawtooth_wave(float frequency, double phase, float samplerate){
  float length = samplerate / frequency;
  return fmod(phase, length) / length * 2.0 - 1.0;
}

int generator_segment_mix(struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  uint32_t phase = data->phase;
  float (*generator)(float frequency, double phase, float samplerate) = 0;
  float volume = data->volume;

  switch(data->type){
  case MIXED_SINE: generator = sine_wave; break;
  case MIXED_SQUARE: generator = square_wave; break;
  case MIXED_TRIANGLE: generator = triangle_wave; break;
  case MIXED_SAWTOOTH: generator = sawtooth_wave; break;
  }
  
  float *restrict out;
  uint32_t samples = UINT32_MAX;
  mixed_buffer_request_write(&out, &samples, data->out);
  for(uint32_t i=0; i<samples; ++i){
    out[i] = generator(data->frequency, phase, data->samplerate) * volume;
    phase = fmod(phase+1, data->samplerate);
  }
  mixed_buffer_finish_write(samples, data->out);

  data->phase = phase;
  return 1;
}

int generator_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "generator";
  info->description = "Wave generator source segment";
  info->min_inputs = 0;
  info->max_inputs = 0;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_VOLUME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The volume scaling factor.");
  
  set_info_field(field++, MIXED_GENERATOR_FREQUENCY,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The frequency in Hz of the generated tone.");
  
  set_info_field(field++, MIXED_GENERATOR_TYPE,
                 MIXED_GENERATOR_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The type of wave form that is produced.");
  
  clear_info_field(field++);
  return 1;
}

int generator_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    break;
  case MIXED_GENERATOR_FREQUENCY:
    *((float *)value) = data->frequency;
    break;
  case MIXED_GENERATOR_TYPE:
    *((enum mixed_generator_type *)value) = data->type;
    break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int generator_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct generator_segment_data *data = (struct generator_segment_data *)segment->data;

  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    break;
  case MIXED_GENERATOR_FREQUENCY:
    if(*(float *)value < 0.0 ||
       data->samplerate < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->frequency = *(float *)value;
    break;
  case MIXED_GENERATOR_TYPE:
    if(*(enum mixed_generator_type *)value < MIXED_SINE ||
       MIXED_SAWTOOTH < *(enum mixed_generator_type *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->type = *(enum mixed_generator_type *)value;
    break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_generator(enum mixed_generator_type type, uint32_t frequency, uint32_t samplerate, struct mixed_segment *segment){
  struct generator_segment_data *data = mixed_calloc(1, sizeof(struct generator_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->frequency = frequency;
  data->type = type;
  data->samplerate = samplerate;
  data->volume = 0.8f;
  
  segment->free = generator_segment_free;
  segment->start = generator_segment_start;
  segment->mix = generator_segment_mix;
  segment->set_out = generator_segment_set_out;
  segment->info = generator_segment_info;
  segment->get = generator_segment_get;
  segment->set = generator_segment_set;
  segment->data = data;
  return 1;
}


int __make_generator(void *args, struct mixed_segment *segment){
  return mixed_make_segment_generator(ARG(enum mixed_generator_type, 0), ARG(uint32_t, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(generator, __make_generator, 3, {
    {.description = "type", .type = MIXED_GENERATOR_TYPE_ENUM},
    {.description = "frequency", .type = MIXED_UINT32},
    {.description = "samplerate", .type = MIXED_UINT32}})
