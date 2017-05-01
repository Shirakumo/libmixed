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

int general_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(location){
  case 0: data->in[0] = buffer; return 1;
  case 1: data->in[1] = buffer; return 1;
  case 2: data->out[0] = buffer; return 1;
  case 3: data->out[1] = buffer; return 1;
  default: return 0; break;
  }
}

int general_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  float lvolume = data->volume * ((0.0<data->pan)?(1.0f-data->pan):1.0f);
  float rvolume = data->volume * ((data->pan<0.0)?(1.0f+data->pan):1.0f);
  
  for(size_t i=0; i<samples; ++i){
    data->out[0]->data[i] = data->in[0]->data[i]*lvolume;
    data->out[1]->data[i] = data->in[1]->data[i]*rvolume;
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
  default: return 0;
  }
  return 1;
}

int general_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(field){
  case MIXED_GENERAL_VOLUME: data->volume = *(float *)value; break;
  case MIXED_GENERAL_PAN: data->pan = *(float *)value; break;
  default: return 0;
  }
  return 1;
}

int mixed_make_segment_general(float volume, float pan, struct mixed_segment *segment){
  struct general_segment_data *data = calloc(1, sizeof(struct general_segment_data));
  if(!data) return 0;

  data->volume = volume;
  data->pan = pan;
  
  segment->free = general_segment_free;
  segment->mix = general_segment_mix;
  segment->set_buffer = general_segment_set_buffer;
  segment->info = general_segment_info;
  segment->get = general_segment_get;
  segment->set = general_segment_set;
  segment->data = data;
  return 1;
}
