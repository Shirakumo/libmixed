#include "internal.h"

int mixed_free_mixer(struct mixed_mixer *mixer){
  if(mixer->segments)
    free(mixer->segments);
  mixer->segments = 0;
}

int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer){
  // Not yet initialised
  if(!mixer->segments){
    if(mixer->size == 0) mixer->size = 32;
    mixer->segments = calloc(mixer->size, sizeof(struct mixed_segment *));
    mixer->count = 0;
  }
  // Too small
  if(mixer->count == mixer->size){
    mixer->segments = realloc(mixer->segments, mixer->size*2);
    mixer->size *= 2;
  }
  // Check completeness
  if(!mixer->segments){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  // All good
  mixer->segments[mixer->count] = segment;
  ++mixer->count;
  return 1;
}

int mixed_mixer_start(struct mixed_mixer *mixer){
  for(size_t i=0;; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment) break;
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
  for(size_t i=0;; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment) break;
    if(!segment->mix(samples, samplerate, segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
    }
  }
  return 1;
}

int mixed_mixer_end(struct mixed_mixer *mixer){
  for(size_t i=0;; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment) break;
    if(segment->end){
      if(!segment->end(segment)){
        mixed_err(MIXED_MIXING_FAILED);
        return 0;
      }
    }
  }
  return 1;
}
