#include "../internal.h"
#define WHITE_NOISE_SAMPLES 4096

struct noise_segment_data{
  struct mixed_buffer *out;
  float (*type)(struct noise_segment_data *data);
  float volume;
  int64_t pink_rows[30];
  int64_t pink_running_sum;
  int32_t pink_index;
  int32_t pink_index_mask;
  float pink_scalar;
  float brown;
};

int noise_segment_free(struct mixed_segment *segment){
  if(segment->data)
    mixed_free(segment->data);
  segment->data = 0;
  return 1;
}

int noise_segment_start(struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;
  if(data->out == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int noise_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;
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

float noise_white(struct noise_segment_data *data){
  IGNORE(data);
  return mixed_random()*2.0-1.0;
}

float noise_pink(struct noise_segment_data *data){
  int64_t random;
  int64_t sum;

  data->pink_index = (data->pink_index + 1) & data->pink_index_mask;

  if(data->pink_index != 0) {
    int zeroes = 0;
    int n = data->pink_index;
    while((n & 1) == 0){
      n = n >> 1;
      zeroes++;
    }

    data->pink_running_sum -= data->pink_rows[zeroes];
    random = (mixed_random()-0.5)*67108864;
    data->pink_running_sum += random;
    data->pink_rows[zeroes] = random;
  }
  
  random = (mixed_random()-0.5)*67108864;
  sum = data->pink_running_sum + random;

  return data->pink_scalar * sum;
}

float noise_brown(struct noise_segment_data *data){
  data->brown += (mixed_random()*2.0-1.0);
  data->brown -= data->brown * 0.03125;
  return data->brown * 0.06250;
}

int noise_segment_mix(struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;

  float (*noise)(struct noise_segment_data *data) = data->type;
  float volume = data->volume;
  float *restrict out;
  uint32_t samples = UINT32_MAX;
  mixed_buffer_request_write(&out, &samples, data->out);
  for(uint32_t i=0; i<samples; ++i){
    out[i] = noise(data) * volume;
  }
  mixed_buffer_finish_write(samples, data->out);
  return 1;
}

int noise_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "noise";
  info->description = "Noise generator segment";
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
  
  set_info_field(field++, MIXED_NOISE_TYPE,
                 MIXED_NOISE_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The type of noise that is produced.");
  
  clear_info_field(field++);
  return 1;
}

int noise_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    break;
  case MIXED_GENERATOR_TYPE:
    if(data->type == noise_white)
      *((enum mixed_noise_type *)value) = MIXED_WHITE_NOISE;
    else if(data->type == noise_pink)
      *((enum mixed_noise_type *)value) = MIXED_PINK_NOISE;
    else if(data->type == noise_brown)
      *((enum mixed_noise_type *)value) = MIXED_BROWN_NOISE;
    break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int noise_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;

  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    break;
  case MIXED_NOISE_TYPE: {
    enum mixed_noise_type type = *(enum mixed_noise_type *)value;
    switch(type){
    case MIXED_WHITE_NOISE: data->type = noise_white; break;
    case MIXED_PINK_NOISE: data->type = noise_pink; break;
    case MIXED_BROWN_NOISE: data->type = noise_brown; break;
    default: 
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    break;
  }
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_noise(enum mixed_noise_type type, struct mixed_segment *segment){
  struct noise_segment_data *data = mixed_calloc(1, sizeof(struct noise_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  segment->data = data;

  noise_segment_set(MIXED_NOISE_TYPE, &type, segment);
  data->volume = 1.0f;

  data->pink_index = 0;
  data->pink_index_mask = (1<<30) - 1;
  long pmax = ((30 + 1) * (1<<(23)));
  data->pink_scalar = 1.0 / (float)pmax;
  data->pink_running_sum = 0;
  for(uint32_t i=0; i<30; ++i) data->pink_rows[i] = 0;
  
  segment->free = noise_segment_free;
  segment->start = noise_segment_start;
  segment->mix = noise_segment_mix;
  segment->set_out = noise_segment_set_out;
  segment->info = noise_segment_info;
  segment->get = noise_segment_get;
  segment->set = noise_segment_set;
  return 1;
}

int __make_noise(void *args, struct mixed_segment *segment){
  return mixed_make_segment_noise(ARG(enum mixed_noise_type, 0), segment);
}

REGISTER_SEGMENT(noise, __make_noise, 1, {
    {.description = "type", .type = MIXED_NOISE_TYPE_ENUM}})
