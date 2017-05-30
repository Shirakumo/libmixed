#include "internal.h"

struct mixer_segment_data{
  struct mixed_buffer **in;
  size_t count;
  size_t size;
  struct mixed_buffer **out;
  size_t channels;
  float volume;
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
    if(data->channels <= location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->out[location] = (struct mixed_buffer *)buffer;
    return 1;
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
  size_t buffers = data->count / data->channels;
  if(0 < buffers){
    float volume = data->volume;
    float channels = data->channels;
    float div = volume/buffers;
    
    for(size_t c=0; c<channels; ++c){
      size_t offset = c*channels;
      float *out = data->out[c]->data;

      // Mix first buffer directly.
      float *in = data->in[offset]->data;
      for(size_t i=0; i<samples; ++i){
        out[i] = in[i]*div;
      }
      // Mix other buffers additively.
      for(size_t b=1; b<buffers; ++b){
        in = data->in[offset+b]->data;
        for(size_t i=0; i<samples; ++i){
          out[i] += in[i]*div;
        }
      }
    }
  }
}

// FIXME: add start method that checks for buffer completeness.

int mixer_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int mixer_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

struct mixed_segment_info *mixer_segment_info(struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));
  
  info->name = "mixer";
  info->description = "Mixes multiple buffers together";
  info->min_inputs = 0;
  info->max_inputs = -1;
  info->outputs = data->channels;
  
  info->fields[0].field = MIXED_BUFFER;
  info->fields[0].description = "The buffer for audio data attached to the location.";
  info->fields[0].flags = MIXED_IN | MIXED_OUT | MIXED_SET;

  info->fields[1].field = MIXED_VOLUME;
  info->fields[1].description = "The volume scaling factor for the output.";
  info->fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  return info;
}

MIXED_EXPORT int mixed_make_segment_mixer(size_t channels, struct mixed_segment *segment){
  struct mixer_segment_data *data = calloc(1, sizeof(struct mixer_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->volume = 1.0f;
  data->channels = channels;
  data->out = calloc(channels, sizeof(struct mixed_buffer *));
  if(!data->out){
    mixed_err(MIXED_OUT_OF_MEMORY);
    free(data);
    return 0;
  }
  
  segment->free = mixer_segment_free;
  segment->mix = mixer_segment_mix;
  segment->set = mixer_segment_set;
  segment->get = mixer_segment_get;
  segment->set_in = mixer_segment_set_in;
  segment->set_out = mixer_segment_set_out;
  segment->info = mixer_segment_info;
  segment->data = data;
  return 1;
}
