#include "internal.h"

struct mixer_segment_data{
  struct mixed_buffer **in;
  size_t count;
  size_t size;
  struct mixed_buffer *out;
};

int mixer_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct mixer_segment_data *)segment->data)->in);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int mixer_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_MONO: data->out = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int mixer_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(buffer){ // Add or set an element
      if(location < data->count){
        data->in[location] = (struct mixed_buffer *)buffer;
    }else{
        return vector_add(buffer, (struct vector *)data);
      }
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_INVALID_LOCATION);
        return 0;
      }
    return vector_remove_pos(location, (struct vector *)data);
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

void mixer_segment_mix(size_t samples, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  size_t count = data->count;
  if(0 < count){
    float div = 1.0f/count;
    for(size_t i=0; i<samples; ++i){
      float out = 0.0f;
      for(size_t b=0; b<count; ++b){
        out += data->in[b]->data[i];
      }
      data->out->data[i] = out*div;
    }
  }
}

struct mixed_segment_info *mixer_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));
  info->name = "mixer";
  info->description = "Mixes multiple buffers together";
  info->min_inputs = 0;
  info->max_inputs = -1;
  info->outputs = 1;
  
  info->fields[0].field = MIXED_BUFFER;
  info->fields[0].description = "The buffer for audio data attached to the location.";
  info->fields[0].flags = MIXED_IN | MIXED_OUT | MIXED_SET;

  return info;
}

MIXED_EXPORT int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment){
  struct mixer_segment_data *data = calloc(1, sizeof(struct mixer_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  if(buffers){
    size_t count = 0;
    while(buffers[count]) ++count;
    // Copy data over
    if(0 < count){
      data->out = buffers[0];
      data->in = calloc(count-1, sizeof(struct mixed_buffer *));
      if(!data->in){
        mixed_err(MIXED_OUT_OF_MEMORY);
        free(data);
        return 0;
      }
      
      data->size = count-1;
      for(data->count = 0; data->count < data->size; ++data->count){
        data->in[data->count] = buffers[data->count+1];
      }
    }
  }

  segment->free = mixer_segment_free;
  segment->mix = mixer_segment_mix;
  segment->set_in = mixer_segment_set_in;
  segment->set_out = mixer_segment_set_out;
  segment->info = mixer_segment_info;
  segment->data = data;
  return 1;
}
