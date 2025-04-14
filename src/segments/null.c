#include "../internal.h"

int null_segment_free(struct mixed_segment *segment){
  segment->data = 0;
  return 1;
}

int null_segment_start(struct mixed_segment *segment){
  if(segment->data == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int null_segment_set(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_MONO: segment->data = buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int void_segment_mix(struct mixed_segment *segment){
  struct mixed_buffer *data = (struct mixed_buffer *)segment->data;
  float *restrict buffer;
  uint32_t frames = UINT32_MAX;
  mixed_buffer_request_read(&buffer, &frames, data);
  mixed_buffer_finish_read(frames, data);
  return 1;
}

int zero_segment_mix(struct mixed_segment *segment){
  struct mixed_buffer *data = (struct mixed_buffer *)segment->data;
  float *restrict buffer;
  uint32_t frames = UINT32_MAX;
  mixed_buffer_request_write(&buffer, &frames, data);
  memset(buffer, 0, frames*sizeof(float));
  mixed_buffer_finish_write(frames, data);
  return 1;
}

int void_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "void";
  info->description = "Consume all audio";
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 0;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_SET,
                 "The buffer for audio data attached to the location.");
  clear_info_field(field++);
  return 1;
}

int zero_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "zero";
  info->description = "Produces silence";
  info->min_inputs = 0;
  info->max_inputs = 0;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");
  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_void(struct mixed_segment *segment){
  segment->free = null_segment_free;
  segment->start = null_segment_start;
  segment->set_in = null_segment_set;
  segment->set_out = null_segment_set;
  segment->mix = void_segment_mix;
  segment->info = void_segment_info;
  return 1;
}

int __make_zero(void *args, struct mixed_segment *segment){
  IGNORE(args);
  return mixed_make_segment_zero(segment);
}

REGISTER_SEGMENT(zero, __make_zero, 0, {0})

MIXED_EXPORT int mixed_make_segment_zero(struct mixed_segment *segment){
  segment->free = null_segment_free;
  segment->start = null_segment_start;
  segment->set_in = null_segment_set;
  segment->set_out = null_segment_set;
  segment->mix = zero_segment_mix;
  segment->info = zero_segment_info;
  return 1;
}

int __make_void(void *args, struct mixed_segment *segment){
  IGNORE(args);
  return mixed_make_segment_void(segment);
}

REGISTER_SEGMENT(void, __make_void, 0, {0})
