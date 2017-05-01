#include "internal.h"

int mixed_free_segment(struct mixed_segment *segment){
  if(segment->free)
    return segment->free(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

int mixed_segment_start(struct mixed_segment *segment){
  if(segment->start)
    return segment->start(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

int mixed_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  if(segment->mix)
    return segment->mix(samples, samplerate, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

int mixed_segment_end(struct mixed_segment *segment){
  if(segment->end)
    return segment->end(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

int mixed_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  if(segment->set_buffer)
    return segment->set_buffer(location, buffer, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

int mixed_segment_get_buffer(size_t location, struct mixed_buffer **buffer, struct mixed_segment *segment){
  if(segment->get_buffer)
    return segment->get_buffer(location, buffer, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

struct mixed_segment_info mixed_segment_info(struct mixed_segment *segment){
  if(segment->info)
    return segment->info(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  struct mixed_segment_info info = {0};
  return info;
}

int mixed_segment_get(size_t field, void *value, struct mixed_segment *segment){
  if(segment->get)
    return segment->get(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

int mixed_segment_set(size_t field, void *value, struct mixed_segment *segment){
  if(segment->set)
    return segment->set(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}
