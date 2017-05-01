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
    printf("Mixing %i samples in %s\n", samples, segment->info(segment).name);
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
