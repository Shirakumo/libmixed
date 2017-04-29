#include "internal.h"

struct mixer_segment_data{
  struct mixed_buffer *out;
  struct mixed_buffer **in;
  size_t count;
  size_t size;
};

int mixer_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct mixer_segment_data *)segment->data)->in);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int mixer_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  if(location == 0){
    data->out = buffer;
  }else{
    --location;
    
  }
  return 1;
}

int mixer_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  struct mixer_segment_data *data = (struct mixer_segment_data *)segment->data;
  size_t count = data->count;
  float div = 1.0f/count;
  
  for(size_t i=0; i<samples; ++i){
    float out = 0.0f;
    for(size_t b=0; b<count; ++b){
      out += data->in[b]->data[i];
    }
    data->out->data[i] = out*div;
  }
  return 1;
}

int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment){
  struct mixer_segment_data *data = calloc(1, sizeof(struct mixer_segment_data));
  if(!data) return 0;

  data->in = buffers;

  segment->free = mixer_segment_free;
  segment->mix = mixer_segment_mix;
  segment->set_buffer = mixer_segment_set_buffer;
  segment->data = data;
  return 1;
}
