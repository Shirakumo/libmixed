#include "internal.h"

MIXED_EXPORT void mixed_free_segment_sequence(struct mixed_segment_sequence *mixer){
  if(mixer->segments)
    free(mixer->segments);
  mixer->segments = 0;
}

MIXED_EXPORT int mixed_segment_sequence_add(struct mixed_segment *segment, struct mixed_segment_sequence *mixer){
  mixed_err(MIXED_NO_ERROR);
  return vector_add(segment, (struct vector *)mixer);
}

MIXED_EXPORT int mixed_segment_sequence_remove(struct mixed_segment *segment, struct mixed_segment_sequence *mixer){
  mixed_err(MIXED_NO_ERROR);
  return vector_remove_item(segment, (struct vector *)mixer);
}

MIXED_EXPORT int mixed_segment_sequence_start(struct mixed_segment_sequence *mixer){
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(segment->start){
      if(!segment->start(segment)){
        return 0;
      }
    }
  }
  return 1;
}

MIXED_EXPORT void mixed_segment_sequence_mix(size_t samples, struct mixed_segment_sequence *mixer){
  size_t count = mixer->count;
  struct mixed_segment **segments = mixer->segments;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = segments[i];
    segment->mix(samples, segment);
  }
}

MIXED_EXPORT int mixed_segment_sequence_end(struct mixed_segment_sequence *mixer){
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(segment->end){
      if(!segment->end(segment)){
        return 0;
      }
    }
  }
  return 1;
}
