#include "internal.h"

struct channel_segment_data{
  struct mixed_channel *channel;
  struct mixed_buffer **buffers;
};

int channel_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct channel_segment_data *)segment->data)->buffers);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int channel_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  if(location<data->channel->channels){
    data->buffers[location] = buffer;
    return 1;
  }else{
    return 0;
  }
}

int source_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  return mixed_buffer_from_channel(data->channel, data->buffers);
}

int drain_segment_mix(size_t samples, size_t samplerate, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  return mixed_buffer_to_channel(data->buffers, data->channel);
}

int mixed_make_segment_source(struct mixed_channel *channel, struct mixed_segment *segment){
  struct channel_segment_data *data = calloc(1, sizeof(struct channel_segment_data));
  if(!data) return 0;

  struct mixed_buffer **buffers = calloc(channel->channels, sizeof(struct mixed_buffer));
  if(!buffers){
    free(data);
    return 0;
  }

  data->buffers = buffers;
  data->channel = channel;

  segment->free = channel_segment_free;
  segment->mix = source_segment_mix;
  segment->set_buffer = channel_segment_set_buffer;
  segment->data = data;
  return 1;
}

int mixed_make_segment_drain(struct mixed_channel *channel, struct mixed_segment *segment){
  struct channel_segment_data *data = calloc(1, sizeof(struct channel_segment_data));
  if(!data) return 0;

  struct mixed_buffer **buffers = calloc(channel->channels, sizeof(struct mixed_buffer));
  if(!buffers){
    free(data);
    return 0;
  }

  data->buffers = buffers;
  data->channel = channel;

  segment->free = channel_segment_free;
  segment->mix = drain_segment_mix;
  segment->set_buffer = channel_segment_set_buffer;
  segment->data = data;
  return 1;
}
