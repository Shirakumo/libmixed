#include "internal.h"

struct general_segment_data{
  struct mixed_buffer *in[2];
  struct mixed_buffer *out[2];
  float volume;
  float pan;
};

int general_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

int general_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: data->in[MIXED_LEFT] = (struct mixed_buffer *)buffer; return 1;
    case MIXED_RIGHT: data->in[MIXED_RIGHT] = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_BUFFER_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int general_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: data->out[MIXED_LEFT] = (struct mixed_buffer *)buffer; return 1;
    case MIXED_RIGHT: data->out[MIXED_RIGHT] = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_BUFFER_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int general_segment_mix(size_t samples, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  float lvolume = data->volume * ((0.0<data->pan)?(1.0f-data->pan):1.0f);
  float rvolume = data->volume * ((data->pan<0.0)?(1.0f+data->pan):1.0f);
  
  for(size_t i=0; i<samples; ++i){
    data->out[MIXED_LEFT]->data[i] = data->in[MIXED_LEFT]->data[i]*lvolume;
    data->out[MIXED_RIGHT]->data[i] = data->in[MIXED_RIGHT]->data[i]*rvolume;
  }
  return 1;
}

struct mixed_segment_info general_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "general";
  info.description = "General segment for volume adjustment and panning.";
  info.flags = MIXED_INPLACE;
  info.min_inputs = 2;
  info.max_inputs = 2;
  info.outputs = 2;
  return info;
}

int general_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(field){
  case MIXED_GENERAL_VOLUME: *((float *)value) = data->volume; break;
  case MIXED_GENERAL_PAN: *((float *)value) = data->pan; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int general_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(field){
  case MIXED_GENERAL_VOLUME: data->volume = *(float *)value; break;
  case MIXED_GENERAL_PAN: data->pan = *(float *)value; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int mixed_make_segment_general(float volume, float pan, struct mixed_segment *segment){
  struct general_segment_data *data = calloc(1, sizeof(struct general_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->volume = volume;
  data->pan = pan;
  
  segment->free = general_segment_free;
  segment->mix = general_segment_mix;
  segment->set_in = general_segment_set_in;
  segment->set_out = general_segment_set_out;
  segment->info = general_segment_info;
  segment->get = general_segment_get;
  segment->set = general_segment_set;
  segment->data = data;
  return 1;
}
