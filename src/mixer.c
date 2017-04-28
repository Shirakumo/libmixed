#include "internal.h"

int mixed_mixer_start(struct mixed_mixer *mixer){
  for(size_t i=0;; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment) break;
    if(!segment->start(segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
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
    if(!segment->end(segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
    }
  }
  return 1;
}
