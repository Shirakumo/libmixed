#include "internal.h"

MIXED_EXPORT void mixed_free_mixer(struct mixed_mixer *mixer){
  if(mixer->segments)
    free(mixer->segments);
  mixer->segments = 0;
}

MIXED_EXPORT int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer){
  return vector_add(segment, (struct vector *)mixer);
}

MIXED_EXPORT int mixed_mixer_remove(struct mixed_segment *segment, struct mixed_mixer *mixer){
  return vector_remove_item(segment, (struct vector *)mixer);
}

MIXED_EXPORT int mixed_mixer_start(struct mixed_mixer *mixer){
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(segment->start){
      if(!segment->start(segment)){
        mixed_err(MIXED_MIXING_FAILED);
        return 0;
      }
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer){
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment->mix(samples, segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_mixer_end(struct mixed_mixer *mixer){
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(segment->end){
      if(!segment->end(segment)){
        mixed_err(MIXED_MIXING_FAILED);
        return 0;
      }
    }
  }
  return 1;
}
