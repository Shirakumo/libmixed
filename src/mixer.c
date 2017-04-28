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

int mixed_flatten_dag(){
  
}

int mixed_color_dag(){
  
}

int mixed_pack_mixer(struct mixed_graph *graph, struct mixed_mixer *mixer){
  struct mixed_vector elements = {0};
  struct mixed_segment **segments = 0;
  struct mixed_buffer *buffers = 0;
  size_t *colors = 0;
  
  if(!mixed_vector_make(&elements)){
    return 0;
  }
  if(!mixed_graph_elements(&elements, graph)){
    goto cleanup;
  }

  segments = calloc(elements.count, sizeof(struct mixed_segment *));
  if(!segments){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  colors = calloc(elements.count, sizeof(size_t *));
  if(!colors){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  if(!mixed_flatten_dag(elements->data, graph->connections, segments)){
    goto cleanup;
  }
  
  if(!mixed_color_dag(elements->data, graph->connections, colors)){
    goto cleanup;
  }

  size_t colorc = 0;
  for(int i=0; i<elements.count; ++i){
    if(colorc < colors[i]) colorc = colors[i];
  }

  buffers = calloc(colorc, sizeof(struct mixed_buffer));
  if(!buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  // EGHUIADWHUIADA
  
  mixer->buffers = buffers;
  mixer->segments = segments;
  return 1;

 cleanup:
  mixed_vector_free(&elements);
  if(segments)
    free(segments);
  if(colors)
    free(colors);
  if(buffers)
    free(buffers);
  return 0;
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
