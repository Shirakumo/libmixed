#include "../internal.h"

struct distribute_data{
  struct mixed_buffer **out;
  uint32_t count;
  uint32_t size;
  struct mixed_buffer *in;
  uint32_t was_available;
};

int distribute_free(struct mixed_segment *segment){
  free_vector((struct vector *)segment->data);
  return 1;
}

int distribute_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    if(0 < location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->in = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int distribute_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(buffer){ // Add or set an element
      if(((struct mixed_buffer *)buffer)->_data){
        mixed_err(MIXED_BUFFER_ALLOCATED);
        return 0;
      }
      ((struct mixed_buffer *)buffer)->is_virtual = 1;
      if(location < data->count){
        data->out[location] = (struct mixed_buffer *)buffer;
      }else{
        return vector_add(buffer, (struct vector *)data);
      }
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_INVALID_LOCATION);
        return 0;
      }
      mixed_free_buffer(data->out[location]);
      return vector_remove_pos(location, (struct vector *)data);
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int distribute_start(struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  struct mixed_buffer *in = data->in;

  if(!in){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }

  for(uint32_t i=0; i<data->count; ++i){
    struct mixed_buffer *buffer = data->out[i];
    buffer->_data = in->_data;
    buffer->size = in->size;
    buffer->read = in->read;
    buffer->write = in->write;
  }

  data->was_available = 0;
  return 1;
}

int distribute_mix(struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  struct mixed_buffer *in = data->in;
  // FIXME: This explicitly does /not/ support reads from both bip regions at once.
  uint32_t max_available = mixed_buffer_available_read(data->out[0]);
  for(uint32_t i=1; i<data->count; ++i){
    max_available = MAX(max_available, mixed_buffer_available_read(data->out[i]));
  }
  mixed_buffer_finish_read(data->was_available - max_available, in);
  // Update virtual buffers with content of real buffer.
  data->was_available = mixed_buffer_available_read(in);
  uint32_t read = atomic_read(in->read);
  uint32_t write = atomic_read(in->write);
  for(uint32_t i=0; i<data->count; ++i){
    struct mixed_buffer *buffer = data->out[i];
    atomic_write(buffer->write, write);
    atomic_write(buffer->read, read);
  }
  return 1;
}

int distribute_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "distribute";
  info->description = "Multiplexes a buffer to multiple outputs to consume it from.";
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = -1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");
  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_distribute(struct mixed_segment *segment){
  struct distribute_data *data = mixed_calloc(1, sizeof(struct distribute_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  segment->free = distribute_free;
  segment->start = distribute_start;
  segment->mix = distribute_mix;
  segment->set_in = distribute_set_in;
  segment->set_out = distribute_set_out;
  segment->info = distribute_info;
  segment->data = data;
  return 1;
}

int __make_distribute(void *args, struct mixed_segment *segment){
  IGNORE(args);
  return mixed_make_segment_distribute(segment);
}

REGISTER_SEGMENT(distribute, __make_distribute, 0, {0})
