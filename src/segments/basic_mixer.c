#include "internal.h"

struct mixer_source{
  struct mixed_segment *segment;
  struct mixed_buffer *buffer;
};

struct basic_mixer_data{
  struct mixer_source **sources;
  size_t count;
  size_t size;
  struct mixed_buffer **out;
  size_t channels;
  float volume;
};

int basic_mixer_free(struct mixed_segment *segment){
  free_vector((struct vector *)segment->data);
  return 1;
}

int basic_mixer_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  
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

int basic_mixer_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(buffer){ // Add or set an element
      if(location < data->count){
        data->sources[location]->buffer = (struct mixed_buffer *)buffer;
      }else{
        struct mixer_source *source = calloc(1, sizeof(struct mixer_source));
        if(!source){
          mixed_err(MIXED_OUT_OF_MEMORY);
          return 0;
        }
        source->buffer = (struct mixed_buffer *)buffer;
        return vector_add(source, (struct vector *)data);
      }
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_INVALID_LOCATION);
        return 0;
      }
      return vector_remove_pos(location, (struct vector *)data);
    }
    return 1;
  case MIXED_SOURCE:
    if(data->count <= location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->sources[location]->segment = (struct mixed_segment *)buffer;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int basic_mixer_mix(size_t samples, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  size_t buffers = data->count / data->channels;
  if(0 < buffers){
    size_t channels = data->channels;
    float volume = data->volume;
    float div = volume;

    for(size_t c=0; c<channels; ++c){
      float *out = data->out[c]->data;

      // Mix first buffer directly.
      struct mixer_source *source = data->sources[c];
      struct mixed_segment *segment = source->segment;
      float *in = source->buffer->data;
      if(segment){
        // FIXME: This is not optimal. Ideally we'd avoid mixing entirely.
        if(!segment->mix(samples, segment))
          memset(in, 0, samples*sizeof(float));
      }
      
      for(size_t i=0; i<samples; ++i){
        out[i] = in[i]*div;
      }
      // Mix other buffers additively.
      for(size_t b=1; b<buffers; ++b){
        source = data->sources[b*channels+c];
        segment = source->segment;
        in = source->buffer->data;
        
        if(segment){
          if(!segment->mix(samples, segment))
            memset(in, 0, samples*sizeof(float));
        }
        
        for(size_t i=0; i<samples; ++i){
          out[i] += in[i]*div;
        }
      }
    }
  }else{
    for(size_t c=0; c<data->channels; ++c){
      memset(data->out[c]->data, 0, samples*sizeof(float));
    }
  }
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int basic_mixer_start(struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  for(size_t i=0; i<data->count; ++i){
    if(data->sources[i]->segment){
      if(!mixed_segment_start(data->sources[i]->segment))
        return 0;
    }
  }
  return 1;
}

int basic_mixer_end(struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  for(size_t i=0; i<data->count; ++i){
    if(data->sources[i]->segment){
      if(!mixed_segment_end(data->sources[i]->segment))
        return 0;
    }
  }
  return 1;
}

int basic_mixer_set(size_t field, void *value, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int basic_mixer_get(size_t field, void *value, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int basic_mixer_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct basic_mixer_data *data = (struct basic_mixer_data *)segment->data;
  info->name = "basic_mixer";
  info->description = "Mixes multiple buffers together";
  info->min_inputs = 0;
  info->max_inputs = -1;
  info->outputs = data->channels;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_VOLUME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The volume scaling factor for the output.");

  set_info_field(field++, MIXED_SOURCE,
                 MIXED_SEGMENT_POINTER, 1, MIXED_IN | MIXED_SET,
                 "The segment that needs to be mixed before its buffer has any useful data.");
  
  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_basic_mixer(size_t channels, struct mixed_segment *segment){
  struct basic_mixer_data *data = calloc(1, sizeof(struct basic_mixer_data));
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
  
  segment->free = basic_mixer_free;
  segment->start = basic_mixer_start;
  segment->mix = basic_mixer_mix;
  segment->end = basic_mixer_end;
  segment->set = basic_mixer_set;
  segment->get = basic_mixer_get;
  segment->set_in = basic_mixer_set_in;
  segment->set_out = basic_mixer_set_out;
  segment->info = basic_mixer_info;
  segment->data = data;
  return 1;
}
