#include "internal.h"

struct space_source{
  struct mixed_buffer *buffer;
  float location[3];
  float velocity[3];
};

struct space_segment_data{
  struct space_source **sources;
  size_t count;
  size_t size;
  float location[3];
  float orientation[3];
  struct mixed_buffer *left;
  struct mixed_buffer *right;
};

int space_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct space_segment_data *)segment->data)->sources);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int space_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  switch(location){
  case MIXED_LEFT: data->left = buffer; return 1;
  case MIXED_RIGHT: data->right = buffer; return 1;
  default: mixed_err(MIXED_INVALID_BUFFER_LOCATION); return 0;
  }
}

int space_segment_set_in(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;

  if(buffer){ // Add or set an element
    if(location < data->count){
      data->sources[location]->buffer = buffer;
    }else{
      struct space_source *source = calloc(1, sizeof(struct space_source));
      if(!source){
        mixed_err(MIXED_OUT_OF_MEMORY);
        return 0;
      }
      source->buffer = buffer;
      return vector_add(source, (struct vector *)data);
    }
  }else{ // Remove an element
    if(data->count <= location){
      mixed_err(MIXED_INVALID_BUFFER_LOCATION);
      return 0;
    }
    free(data->sources[location]);
    return vector_remove_pos(location, (struct vector *)data);
  }
  return 1;
}

int space_segment_mix(size_t samples, struct mixed_segment *segment){
  
}

struct mixed_segment_info space_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "space";
  info.description = "Mixes multiple sources while simulating 3D space.";
  info.min_inputs = 0;
  info.max_inputs = -1;
  info.outputs = 2;
  return info;
}

int mixed_make_segment_space(struct mixed_segment *segment){
  struct space_segment_data *data = calloc(1, sizeof(struct space_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  segment->free = space_segment_free;
  segment->mix = space_segment_mix;
  segment->set_in = space_segment_set_in;
  segment->set_out = space_segment_set_out;
  segment->data = data;
}
