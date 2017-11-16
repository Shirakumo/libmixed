#include "internal.h"
#define WHITE_NOISE_SAMPLES 1024

struct noise_segment_data{
  struct mixed_buffer *out;
  enum mixed_noise_type type;
  float white_noise[WHITE_NOISE_SAMPLES];
  float window[8];
  size_t white_noise_index;
  float volume;
};

int noise_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int noise_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
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

float noise_white(float *window, float white){
  return white;
}

float noise_pink(float *window, float white){
  window[0] = 0.99886 * window[0] + white * 0.0555179;
  window[1] = 0.99332 * window[1] + white * 0.0750759;
  window[2] = 0.96900 * window[2] + white * 0.1538520;
  window[3] = 0.86650 * window[3] + white * 0.3104856;
  window[4] = 0.55000 * window[4] + white * 0.5329522;
  window[5] = -0.7616 * window[5] - white * 0.0168980;
  float result = (window[0] + window[1] + window[2] + window[3] + window[4] + window[5] + window[6] + white * 0.5362) * 0.11;
  window[6] = white * 0.115926;
  return result;
}

float noise_brown(float *window, float white){
  float result = (window[0] + 0.02*white) / 1.02;
  window[0] = result * 3.5;
  return result;
}

void noise_segment_mix(size_t samples, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;

  float (*noise)(float *window, float white) = 0;
  size_t index = data->white_noise_index;
  float volume = data->volume;
  float *out = data->out->data;
  float *window = data->window;
  float *white = data->white_noise;
  
  switch(data->type){
  case MIXED_WHITE_NOISE: noise = noise_white; break;
  case MIXED_PINK_NOISE: noise = noise_pink; break;
  case MIXED_BROWN_NOISE: noise = noise_brown; break;
  }
  
  for(size_t i=0; i<samples; ++i){
    out[i] = noise(window, white[index]) * volume;
    index = (index+1) % WHITE_NOISE_SAMPLES;
  }
  data->white_noise_index = index;
}

struct mixed_segment_info *noise_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));

  if(info){
    info->name = "noise";
    info->description = "Noise generator segment";
    info->min_inputs = 0;
    info->max_inputs = 0;
    info->outputs = 1;
  
    info->fields[0].field = MIXED_BUFFER;
    info->fields[0].description = "The buffer for audio data attached to the location.";
    info->fields[0].flags = MIXED_OUT | MIXED_SET;

    info->fields[1].field = MIXED_VOLUME;
    info->fields[1].description = "The volume scaling factor.";
    info->fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;
  
    info->fields[3].field = MIXED_NOISE_TYPE;
    info->fields[3].description = "The type of noise that is produced.";
    info->fields[3].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;
  }
  
  return info;
}

int noise_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    break;
  case MIXED_GENERATOR_TYPE:
    *((enum mixed_noise_type *)value) = data->type;
    break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int noise_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct noise_segment_data *data = (struct noise_segment_data *)segment->data;

  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    break;
  case MIXED_NOISE_TYPE:
    if(*(enum mixed_noise_type *)value < MIXED_WHITE_NOISE ||
       MIXED_BROWN_NOISE < *(enum mixed_noise_type *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->type = *(enum mixed_noise_type *)value;
    break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_noise(enum mixed_noise_type type, struct mixed_segment *segment){
  struct noise_segment_data *data = calloc(1, sizeof(struct noise_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  for(size_t i=0; i<WHITE_NOISE_SAMPLES; ++i){
    data->white_noise[i] = (float)rand()/(float)RAND_MAX*2-1;
  }

  data->type = type;
  data->volume = 1.0f;
  
  segment->free = noise_segment_free;
  segment->mix = noise_segment_mix;
  segment->set_out = noise_segment_set_out;
  segment->info = noise_segment_info;
  segment->get = noise_segment_get;
  segment->set = noise_segment_set;
  segment->data = data;
  return 1;
}
