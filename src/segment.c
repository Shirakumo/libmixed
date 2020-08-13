#include "internal.h"

MIXED_EXPORT int mixed_free_segment(struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->free)
    return segment->free(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

MIXED_EXPORT int mixed_segment_start(struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->start)
    return segment->start(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

MIXED_EXPORT int mixed_segment_mix(struct mixed_segment *segment){
  return segment->mix(segment);
}

MIXED_EXPORT int mixed_segment_end(struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->end)
    return segment->end(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

MIXED_EXPORT int mixed_segment_set_in(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set_in)
    return segment->set_in(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_set_out(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set_out)
    return segment->set_out(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_get_in(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->get_in)
    return segment->get_in(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_get_out(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->get_out)
    return segment->get_out(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  if(segment->info)
    return segment->info(info, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->get)
    return segment->get(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set)
    return segment->set(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}
