#include "internal.h"

struct queue_segment_data{
  struct mixed_segment **queue;
  size_t count;
  size_t size;
  struct mixed_buffer **out;
  size_t out_count;
  struct mixed_buffer **in;
  size_t in_count;
};

int queue_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int queue_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location < data->in_count){
      data->in[location] = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int queue_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location < data->out_count){
      data->out[location] = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int queue_segment_mix(size_t samples, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  if(0 <= data->count){
    
  }else{
    
  }
  return 1;
}

int queue_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  size_t i=0;
  for(; i<data->out_count && i<data->in_count; ++i){
    mixed_buffer_copy(data->in[i], data->out[i]);
  }
  for(; i<data->out_count; ++i){
    mixed_buffer_clear(data->out[i]);
  }
  
  return 1;
}

struct mixed_segment_info *queue_segment_info(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));

  if(info){
    info->name = "queue";
    info->description = "Queue multiple segments one after the other";
    info->flags = MIXED_INPLACE;
    info->min_inputs = data->in_count;
    info->max_inputs = data->in_count;
    info->outputs = data->out_count;
  
    struct mixed_segment_field_info *field = info->fields;
    set_info_field(field++, MIXED_BYPASS,
                   MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "Bypass the segment's processing.");
    set_info_field(field++, MIXED_IN_COUNT,
                   MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "Access the number of available input buffers.");
    set_info_field(field++, MIXED_OUT_COUNT,
                   MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "Access the number of available output buffers.");
  }

  return info;
}

int queue_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == queue_segment_mix_bypass); break;
  case MIXED_IN_COUNT: *((size_t *)value) = data->in_count; break;
  case MIXED_OUT_COUNT: *((size_t *)value) = data->out_count; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int queue_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = queue_segment_mix_bypass;
    }else{
      segment->mix = queue_segment_mix;
    }
    break;
  case MIXED_IN_COUNT: {
    struct mixed_buffer **in = crealloc(data->in, data->in_count, *(size_t *)value, sizeof(struct mixed_buffer *));
    if(!in){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    data->in = in;
    data->in_count = *(size_t *)value;
  } break;
  case MIXED_OUT_COUNT: {
    struct mixed_buffer **out = crealloc(data->out, data->out_count, *(size_t *)value, sizeof(struct mixed_buffer *));
    if(!out){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    data->out = out;
    data->out_count = *(size_t *)value;
  } break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_queue(struct mixed_segment *segment){
  struct queue_segment_data *data = calloc(1, sizeof(struct queue_segment_data));
  if(!data){ goto cleanup; }

  data->in_count = 8;
  data->in = calloc(8, sizeof(struct mixed_segment *));
  if(!data->in){ goto cleanup; }

  data->out_count = 8;
  data->out = calloc(8, sizeof(struct mixed_segment *));
  if(!data->out){ goto cleanup; }
  
  segment->free = queue_segment_free;
  segment->mix = queue_segment_mix;
  segment->set_in = queue_segment_set_in;
  segment->set_out = queue_segment_set_out;
  segment->info = queue_segment_info;
  segment->get = queue_segment_get;
  segment->set = queue_segment_set;
  segment->data = data;
  return 1;

 cleanup:
  mixed_err(MIXED_OUT_OF_MEMORY);
  if(data){
    if(data->in) free(data->in);
    if(data->out) free(data->out);
    free(data);
  }
  return 0;
}

MIXED_EXPORT int mixed_queue_add(struct mixed_segment *new, struct mixed_segment *queue){
  return 1;
}

MIXED_EXPORT int mixed_queue_remove(struct mixed_segment *old, struct mixed_segment *queue){
  return 1;
}

MIXED_EXPORT int mixed_queue_clear(struct mixed_segment *queue){
  return 1;
}
