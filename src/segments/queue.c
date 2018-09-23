#include "internal.h"

struct queue_segment_data{
  struct mixed_segment **queue;
  size_t count;
  size_t size;
  struct mixed_buffer **out;
};

int queue_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int queue_segment_start(struct mixed_segment *segment){
  
}

int queue_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->in = (struct mixed_buffer *)buffer;
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
    if(location == 0){
      data->out = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

void queue_segment_mix(size_t samples, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  if(0 <= data->count){
    size_t out = data->queue[0];
  }else{
    
  }
}

void queue_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  
  mixed_buffer_copy(data->in, data->out);
}

struct mixed_segment_info *queue_segment_info(struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));

  if(info){
    info->name = "queue";
    info->description = "Queue multiple segments one after the other";
    info->flags = MIXED_INPLACE;
    info->min_inputs = 1;
    info->max_inputs = 1;
    info->outputs = 1;
  
    struct mixed_segment_field_info *field = info->fields;
    set_info_field(field++, MIXED_BYPASS,
                   MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "Bypass the segment's processing.");
  }

  return info;
}

int queue_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct queue_segment_data *data = (struct queue_segment_data *)segment->data;
  switch(field){
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == queue_segment_mix_bypass); break;
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
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_queue(struct mixed_segment *segment){
  struct queue_segment_data *data = calloc(1, sizeof(struct queue_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  segment->free = queue_segment_free;
  segment->start = queue_segment_start;
  segment->mix = queue_segment_mix;
  segment->set_in = queue_segment_set_in;
  segment->set_out = queue_segment_set_out;
  segment->info = queue_segment_info;
  segment->get = queue_segment_get;
  segment->set = queue_segment_set;
  segment->data = data;
  return 1;
}
