#include "internal.h"

int mixed_free_mixer(struct mixed_mixer *mixer){
  if(mixer->segments)
    free(mixer->segments);
  mixer->segments = 0;
}

int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer){
  // Not yet initialised
  if(!mixer->segments){
    if(mixer->size == 0) mixer->size = BASE_VECTOR_SIZE;
    mixer->segments = calloc(mixer->size, sizeof(struct mixed_segment *));
    mixer->count = 0;
  }
  // Too small
  if(mixer->count == mixer->size){
    mixer->segments = crealloc(mixer->segments, mixer->size, mixer->size*2,
                               sizeof(struct mixed_segment *));
    mixer->size *= 2;
  }
  // Check completeness
  if(!mixer->segments){
    mixed_err(MIXED_OUT_OF_MEMORY);
    mixer->count = 0;
    mixer->size = 0;
    return 0;
  }
  // All good
  mixer->segments[mixer->count] = segment;
  ++mixer->count;
  return 1;
}

int mixed_mixer_remove(struct mixed_segment *segment, struct mixed_mixer *mixer){
  if(!mixer->segments){
    mixed_err(MIXED_NOT_INITIALIZED);
    return 0;
  }
  for(size_t i=0; i<mixer->count; ++i){
    if(mixer->segments[i] == segment){
      // Shift down
      for(size_t j=i+1; j<mixer->count; ++j){
        mixer->segments[j-1] = mixer->segments[j];
      }
      mixer->segments[mixer->count-1] = 0;
      --mixer->count;
      // We have sufficiently deallocated. Shrink.
      if(mixer->count < mixer->size/4 && BASE_VECTOR_SIZE < mixer->size){
        mixer->segments = crealloc(mixer->segments, mixer->size, mixer->size/2,
                                   sizeof(struct mixed_buffer *));
        if(!mixer->segments){
          mixed_err(MIXED_OUT_OF_MEMORY);
          mixer->count = 0;
          mixer->size = 0;
          return 0;
        }
      }
      break;
    }
  }
  return 1;
}

int mixed_mixer_start(struct mixed_mixer *mixer){
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

int mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer){
  size_t samplerate = mixer->samplerate;
  size_t count = mixer->count;
  for(size_t i=0; i<count; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment->mix(samples, samplerate, segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
    }
  }
  return 1;
}

int mixed_mixer_end(struct mixed_mixer *mixer){
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
