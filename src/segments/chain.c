#include "../internal.h"

MIXED_EXPORT int mixed_chain_add(struct mixed_segment *segment, struct mixed_segment *chain){
  mixed_err(MIXED_NO_ERROR);
  return vector_add(segment, (struct vector *)chain->data);
}

MIXED_EXPORT int mixed_chain_add_pos(uint32_t i, struct mixed_segment *segment, struct mixed_segment *chain){
  mixed_err(MIXED_NO_ERROR);
  return vector_add_pos(i, segment, (struct vector *)chain->data);
}

MIXED_EXPORT int mixed_chain_remove(struct mixed_segment *segment, struct mixed_segment *chain){
  mixed_err(MIXED_NO_ERROR);
  return vector_remove_item(segment, (struct vector *)chain->data);
}

MIXED_EXPORT int mixed_chain_remove_at(uint32_t i, struct mixed_segment *chain){
  mixed_err(MIXED_NO_ERROR);
  return vector_remove_pos(i, (struct vector *)chain->data);
}

int chain_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free_vector(segment->data);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int chain_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(0 < data->count){
      return mixed_segment_set_in(field, location, buffer, data->data[0]);
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int chain_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(0 < data->count){
      return mixed_segment_set_out(field, location, buffer, data->data[data->count-1]);
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int chain_segment_start(struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;
  uint32_t count = data->count;
  for(uint32_t i=0; i<count; ++i){
    struct mixed_segment *segment = (struct mixed_segment *)data->data[i];
    if(segment->start){
      if(!segment->start(segment)){
        return 0;
      }
    }
  }
  return 1;
}

int chain_segment_mix(struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;
  uint32_t count = data->count;
  for(uint32_t i=0; i<count; ++i){
    struct mixed_segment *segment = (struct mixed_segment *)data->data[i];
    if(segment->mix){
      if(!segment->mix(segment)){
        return 0;
      }
    }
  }
  return 1;
}

int chain_segment_mix_bypass(struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;
  uint32_t count = data->count;

  if(count == 0) return 1;

  struct mixed_segment *in = data->data[0];
  struct mixed_segment *out = data->data[count-1];
  
  mixed_channel_t inc, outc;
  if(!mixed_segment_get(MIXED_IN_COUNT, &inc, in)) return 0;
  if(!mixed_segment_get(MIXED_OUT_COUNT, &outc, out)) return 0;
  
  for(mixed_channel_t c=0; c<MIN(inc,outc); ++c){
    struct mixed_buffer *inb, *outb;
    if(!mixed_segment_get_in(MIXED_BUFFER, c, &inb, in)) return 0;
    if(!mixed_segment_get_out(MIXED_BUFFER, c, &outb, out)) return 0;
    if(!mixed_buffer_transfer(inb, outb)) return 0;
  }
  return 1;
}

int chain_segment_end(struct mixed_segment *segment){
  struct vector *data = (struct vector *)segment->data;
  uint32_t count = data->count;
  for(uint32_t i=0; i<count; ++i){
    struct mixed_segment *segment = (struct mixed_segment *)data->data[i];
    if(segment->end){
      if(!segment->end(segment)){
        return 0;
      }
    }
  }
  return 1;
}

int chain_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "chain";
  info->description = "Chain together several segments.";
  info->flags = 0;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int chain_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  switch(field){
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == chain_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int chain_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  switch(field){
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = chain_segment_mix_bypass;
    }else{
      segment->mix = chain_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_chain(struct mixed_segment *segment){
  struct vector *data = mixed_calloc(1, sizeof(struct vector));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  segment->free = chain_segment_free;
  segment->start = chain_segment_start;
  segment->mix = chain_segment_mix;
  segment->end = chain_segment_end;
  segment->set_in = chain_segment_set_in;
  segment->set_out = chain_segment_set_out;
  segment->info = chain_segment_info;
  segment->get = chain_segment_get;
  segment->set = chain_segment_set;
  segment->data = data;
  return 1;
}

int __make_chain(void *args, struct mixed_segment *segment){
  IGNORE(args);
  return mixed_make_segment_chain(segment);
}

REGISTER_SEGMENT(chain, __make_chain, 0, {0})
