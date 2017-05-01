#include "internal.h"

struct mixer_segment_data{
  struct mixed_buffer *out;
  struct mixed_buffer **in;
  size_t count;
  size_t size;
};

int mixer_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct mixer_segment_data *)segment->data)->in);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int mixer_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  if(location == 0){
    data->out = buffer;
  }else{
    --location;
    if(buffer){ // Add or set an element
      if(data->count < location){
        mixed_err(MIXED_MIXER_INVALID_INDEX);
        return 0;
      }
      // Not yet initialized
      if(!data->in){
        if(data->size == 0) data->size = BASE_VECTOR_SIZE;
        data->in = calloc(data->size, sizeof(struct mixed_buffer *));
        data->count = 0;
      }
      // Too small
      if(data->count == data->size){
        data->in = crealloc(data->in, data->size, data->size*2,
                            sizeof(struct mixed_buffer *));
        data->size *= 2;
      }
      // Check completeness
      if(!data->in){
        mixed_err(MIXED_OUT_OF_MEMORY);
        data->count = 0;
        data->size = 0;
      }
      // All good
      data->in[location] = buffer;
      if(location = data->count){
        ++data->count;
      }
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_MIXER_INVALID_INDEX);
        return 0;
      }
      for(size_t i=location+1; i<data->count; ++i){
        data->in[i-1] = data->in[i];
      }
      data->in[data->count] = 0;
      --data->count;
      // We have sufficiently deallocated. Shrink.
      if(data->count < data->size/4 && BASE_VECTOR_SIZE < data->size){
        data->in = crealloc(data->in, data->size, data->size/2,
                            sizeof(struct mixed_buffer *));
        if(!data->in){
          mixed_err(MIXED_OUT_OF_MEMORY);
          data->count = 0;
          data->size = 0;
          return 0;
        }
      }
    }
  }
  return 1;
}

int mixer_segment_get_buffer(size_t location, struct mixed_buffer **buffer, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  if(location == 0){
    *buffer = data->out;
  }else{
    --location;
    if(data->count <= location){
      mixed_err(MIXED_MIXER_INVALID_INDEX);
      return 0;
    }
    *buffer = data->in[location];
  }
  return 1;
}

int mixer_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  size_t count = data->count;
  float div = 1.0f/count;
  
  for(size_t i=0; i<samples; ++i){
    float out = 0.0f;
    for(size_t b=0; b<count; ++b){
      out += data->in[b]->data[i];
    }
    data->out->data[i] = out*div;
  }
  return 1;
}

struct mixed_segment_info mixer_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "mixer";
  info.description = "Mixes multiple buffers together";
  info.min_inputs = 0;
  info.max_inputs = -1;
  info.outputs = 1;
  return info;
}

int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment){
  struct mixer_segment_data *data = calloc(1, sizeof(struct mixer_segment_data));
  if(!data) return 0;

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
  segment->set_buffer = mixer_segment_set_buffer;
  segment->get_buffer = mixer_segment_get_buffer;
  segment->data = data;
  return 1;
}
