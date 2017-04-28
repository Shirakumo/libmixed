#include "mixed.h"

void mixed_free_mixer(struct mixed_mixer *mixer){  
  if(mixer->buffers){
    for(size_t i=0; mixer->buffers[i]; ++i){
      mixed_buffer_free(mixer->buffers[i]);
    }
    free(mixer->buffers);
  }
  mixer->buffers = 0;

  if(mixer->segments)
    free(mixer->segments);
  mixer->segments = 0;
}

int mixed_pack_mixer(struct mixed_graph *graph, struct mixed_mixer *mixer){
  struct mixed_vector elements = {0};
  if(!mixed_vector_make(&elements)){
    return 0;
  }
  if(!mixed_graph_elements(&elements, graph)){
    mixed_vector_free(&elements);
    return 0;
  }

  // Flatten
  // Color
  // Allocate
}

int mixed_mix(size_t samples, struct mixed_mixer *mixer){
  for(size_t i=0;; ++i){
    struct mixed_segment *segment = mixer->segments[i];
    if(!segment) break;
    if(!segment->mix(samples, segment)){
      mixed_err(MIXED_MIXING_FAILED);
      return 0;
    }
  }
  return 1;
}
