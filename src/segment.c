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

MIXED_EXPORT int mixed_segment_mix(size_t samples, struct mixed_segment *segment){
  return segment->mix(samples, segment);
}

MIXED_EXPORT int mixed_segment_end(struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->end)
    return segment->end(segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 1;
}

MIXED_EXPORT int mixed_segment_set_in(size_t field, size_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set_in)
    return segment->set_in(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_set_out(size_t field, size_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set_out)
    return segment->set_out(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_get_in(size_t field, size_t location, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->get_in)
    return segment->get_in(field, location, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_get_out(size_t field, size_t location, void *value, struct mixed_segment *segment){
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

MIXED_EXPORT int mixed_segment_get(size_t field, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->get)
    return segment->get(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}

MIXED_EXPORT int mixed_segment_set(size_t field, void *value, struct mixed_segment *segment){
  mixed_err(MIXED_NO_ERROR);
  if(segment->set)
    return segment->set(field, value, segment);
  mixed_err(MIXED_NOT_IMPLEMENTED);
  return 0;
}
