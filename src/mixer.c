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

int mixed_flatten_dag(struct mixed_segment *segments, struct mixed_connection *connections, struct mixed_vector *flattened){
  
}

int mixed_color_dag(struct mixed_segment *segments, struct mixed_connection *connections, size_t *colors){
  
}

int mixed_pack_mixer(struct mixed_graph *graph, struct mixed_mixer *mixer){
  struct mixed_vector elements = {0};
  struct mixed_segment **segments = 0;
  struct mixed_buffer *buffers = 0;
  size_t colors = 0;
  
  // Prepare data
  if(!mixed_vector_make(&elements)){
    return 0;
  }
  if(!mixed_graph_elements(&elements, graph)){
    goto cleanup;
  }

  segments = calloc(elements.count+1, sizeof(struct mixed_segment *));
  if(!segments){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  // Process graphs
  if(!mixed_flatten_dag(elements.data, graph->connections, segments)){
    goto cleanup;
  }
  
  if(!mixed_color_dag(elements.data, graph->connections, &colors)){
    goto cleanup;
  }
  
  // Allocate buffers
  buffers = calloc(colors+1, sizeof(struct mixed_buffer *));
  if(!buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  for(size_t i=0; i<colors; ++i){
    struct mixed_buffer *buffer = buffers[i];
    buffer->size = mixer->buffersize;
    buffer->samplerate = mixer->samplerate;
    if(!mixed_buffer_make(buffer)){
      goto cleanup;
    }
  }
  
  // Tie buffers to segments
  for(size_t i=0; segments[i]; ++i){
    struct mixed_segment *segment = segments[i];
    for(site_t j=0; segment->inputs[j]; j++){
      segment->inputs[j] = buffers[segment->inputs[j]];
    }
    for(site_t j=0; segment->outputs[j]; j++){
      segment->outputs[j] = buffers[segment->outputs[j]];
    }
  }
  
  // All done
  mixer->buffers = buffers;
  mixer->segments = segments;
  return 1;

 cleanup:
  mixed_vector_free(&elements);
  if(segments)
    free(segments);
  if(colors)
    free(colors);
  if(buffers){
    for(size_t i=0; buffers[i]; ++i){
      mixed_buffer_free(buffers[i]);
    }
    free(buffers);
  }
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
