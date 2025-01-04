#include "../internal.h"

struct queue_segment_data{
  struct mixed_segment **queue;
  uint32_t count;
  uint32_t size;
  struct mixed_buffer **out;
  uint32_t out_count;
  struct mixed_buffer **in;
  uint32_t in_count;
};

int queue_segment_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int queue_segment_start(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  for(mixed_channel_t i=0; i<data->out_count; ++i){
    if(data->out[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  for(mixed_channel_t i=0; i<data->in_count; ++i){
    if(data->in[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  return 1;
}

int queue_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location < data->in_count){
      data->in[location] = (struct mixed_buffer *)buffer;
      // Update existing segments in the queue.
      for(uint32_t i=0; i<data->count; ++i){
        mixed_segment_set_in(MIXED_BUFFER, location, buffer, data->queue[i]);
      }
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int queue_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location < data->out_count){
      data->out[location] = (struct mixed_buffer *)buffer;
      // Update existing segments in the queue.
      for(uint32_t i=0; i<data->count; ++i){
        mixed_segment_set_out(MIXED_BUFFER, location, buffer, data->queue[i]);
      }
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int queue_segment_mix_bypass(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  uint32_t i=0;
  for(; i<data->out_count && i<data->in_count; ++i){
    mixed_buffer_transfer(data->in[i], data->out[i]);
  }
  for(; i<data->out_count; ++i){
    mixed_buffer_clear(data->out[i]);
  }
  
  return 1;
}

int queue_segment_mix(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
 start:
  if(0 < data->count){
    struct mixed_segment *inner = data->queue[0];
    int result = inner->mix(inner);
    if(result)
      return result;
    mixed_segment_end(inner);
    if(mixed_queue_remove(inner, segment))
      goto start;
    return 0;
  }else{
    return queue_segment_mix_bypass(segment);
  }
}

int queue_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  
  info->name = "queue";
  info->description = "Queue multiple segments one after the other";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 0;
  info->max_inputs = data->in_count;
  info->outputs = data->out_count;

  if(0 < data->count){
    struct mixed_segment_info inner = {0};
    mixed_segment_info(&inner, data->queue[0]);
    info->flags = inner.flags;
    info->min_inputs = inner.min_inputs;
    info->max_inputs = inner.max_inputs;
    info->outputs = inner.outputs;
  }
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  set_info_field(field++, MIXED_CURRENT_SEGMENT,
                 MIXED_SEGMENT_POINTER, 1, MIXED_SEGMENT | MIXED_GET,
                 "Retrieve the currently playing segment, if any.");

  set_info_field(field++, MIXED_IN_COUNT,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Access the number of available input buffers.");

  set_info_field(field++, MIXED_OUT_COUNT,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Access the number of available output buffers.");

  clear_info_field(field++);
  return 1;
}

int queue_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == queue_segment_mix_bypass); break;
  case MIXED_CURRENT_SEGMENT: *((struct mixed_segment **)value) = (data->queue)?data->queue[0]:0; break;
  case MIXED_IN_COUNT: *((uint32_t *)value) = data->in_count; break;
  case MIXED_OUT_COUNT: *((uint32_t *)value) = data->out_count; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int queue_resize_buffers(struct mixed_buffer ***buffers, uint32_t *count, uint32_t new){
  if(new == *count) return 1;
  if(new == 0){
    mixed_free(*buffers);
    *buffers = 0;
  }else{
    *buffers = crealloc(*buffers, *count, new, sizeof(struct mixed_buffer *));
    if(!*buffers){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
  }
  *count = new;
  return 1;
}

int queue_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = queue_segment_mix_bypass;
    }else{
      segment->mix = queue_segment_mix;
    }
    break;
  case MIXED_IN_COUNT: return queue_resize_buffers(&data->in, &data->in_count, *(uint32_t *)value);
  case MIXED_OUT_COUNT: return queue_resize_buffers(&data->out, &data->out_count, *(uint32_t *)value);
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_queue(struct mixed_segment *segment){
  struct queue_segment_data *data = mixed_calloc(1, sizeof(struct queue_segment_data));
  if(!data){ goto cleanup; }

  data->in_count = 2;
  data->in = mixed_calloc(2, sizeof(struct mixed_segment *));
  if(!data->in){ goto cleanup; }

  data->out_count = 2;
  data->out = mixed_calloc(2, sizeof(struct mixed_segment *));
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
    if(data->in) mixed_free(data->in);
    if(data->out) mixed_free(data->out);
    mixed_free(data);
  }
  return 0;
}

MIXED_EXPORT int mixed_queue_add(struct mixed_segment *new, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  struct mixed_segment_info info = {0};

  if(!mixed_segment_start(new))
    return 0;
  
  if(!vector_add(new, (struct vector *)data))
    return 0;
  
  mixed_segment_info(&info, new);
  uint32_t ins = MIN(data->in_count, info.max_inputs);
  for(uint32_t i=0; i<ins; ++i){
    mixed_segment_set_in(MIXED_BUFFER, i, data->in[i], new);
  }
  uint32_t outs = MIN(data->out_count, info.outputs);
  for(uint32_t i=0; i<outs; ++i){
    mixed_segment_set_out(MIXED_BUFFER, i, data->out[i], new);
  }
  return 1;
}

MIXED_EXPORT int mixed_queue_remove(struct mixed_segment *old, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  struct mixed_segment_info info = {0};

  mixed_segment_end(old);
  // If the stop fails we remove it anyway.
  
  if(!vector_remove_item(old, (struct vector *)data))
    return 0;
    
  mixed_segment_info(&info, old);
  for(uint32_t i=0; i<info.max_inputs; ++i){
    mixed_segment_set_in(MIXED_BUFFER, i, 0, old);
  }
  for(uint32_t i=0; i<info.outputs; ++i){
    mixed_segment_set_out(MIXED_BUFFER, i, 0, old);
  }
  return 1;
}

MIXED_EXPORT int mixed_queue_remove_at(uint32_t pos, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  return mixed_queue_remove(data->queue[pos], segment);
}

MIXED_EXPORT int mixed_queue_clear(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  struct mixed_segment_info info = {0};
  
  for(uint32_t i=0; i<data->count; ++i){
    struct mixed_segment *old = data->queue[i];
    mixed_segment_end(old);
    mixed_segment_info(&info, old);
    for(uint32_t i=0; i<info.max_inputs; ++i){
      mixed_segment_set_in(MIXED_BUFFER, i, 0, old);
    }
    for(uint32_t i=0; i<info.outputs; ++i){
      mixed_segment_set_out(MIXED_BUFFER, i, 0, old);
    }
  }
  return vector_clear((struct vector *)data);
}

int __make_queue(void *args, struct mixed_segment *segment){
  IGNORE(args);
  return mixed_make_segment_queue(segment);
}

REGISTER_SEGMENT(queue, __make_queue, 0, {0})
