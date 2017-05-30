#include "internal.h"

struct channel_segment_data{
  struct mixed_channel *channel;
  struct mixed_buffer **buffers;
  struct mixed_buffer **resample_buffers;
  int (*resampler)(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples);
  size_t samplerate;
  float volume;
};

int channel_segment_free(struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  if(data){
    if(data->resample_buffers){
      for(size_t i=0; i<data->channel->channels; ++i){
        struct mixed_buffer *buffer = data->resample_buffers[i];
        mixed_free_buffer(buffer);
        free(buffer);
      }
      free(data->resample_buffers);
    }
    free(data->buffers);
    free(data);
  }
  segment->data = 0;
  return 1;
}

int channel_segment_set_buffer(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;

  switch(field){
  case MIXED_CHANNEL_RESAMPLER:
    data->resampler = (int (*)(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples))buffer;
    return 1;
  case MIXED_BUFFER:
    if(location<data->channel->channels){
      data->buffers[location] = (struct mixed_buffer *)buffer;
      return 1;
    }else{
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

void source_segment_mix(size_t samples, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;

  if(data->resample_buffers){
    size_t source_samples = samples * data->samplerate / data->channel->samplerate;
    mixed_buffer_from_channel(data->channel, data->resample_buffers, source_samples, data->volume);
    for(size_t i=0; i<data->channel->channels; ++i){
      data->resampler(data->resample_buffers[i], data->channel->samplerate, data->buffers[i], data->samplerate, samples);
    }
  }else{
    mixed_buffer_from_channel(data->channel, data->buffers, samples, data->volume);
  }
}

void drain_segment_mix(size_t samples, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;

  if(data->resample_buffers){
    for(size_t i=0; i<data->channel->channels; ++i){
      data->resampler(data->buffers[i], data->samplerate, data->resample_buffers[i], data->channel->samplerate, samples);
    }
    size_t drain_samples = samples * data->samplerate / data->channel->samplerate;
    mixed_buffer_to_channel(data->resample_buffers, data->channel, drain_samples, data->volume);
  }else{
    mixed_buffer_to_channel(data->buffers, data->channel, samples, data->volume);
  }
}

int source_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  case MIXED_BYPASS:
    if(*(bool *)value){
      for(size_t i=0; i<data->channel->channels; ++i){
        memset(data->buffers[i]->data, 0, data->buffers[i]->size*4);
      }
      segment->mix = mix_noop;
    }else{
      segment->mix = source_segment_mix;
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int drain_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  case MIXED_BYPASS:
    if(*(bool *)value){
      memset(data->channel->data, 0, data->channel->size);
      segment->mix = mix_noop;
    }else{
      segment->mix = drain_segment_mix;
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

// FIXME: add start method that checks for buffer completeness.

int source_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    return 1;
  case MIXED_BYPASS:
    *(bool *)value = (segment->mix == mix_noop);
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

struct mixed_segment_info *source_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));
  info->name = "source";
  info->description = "Segment acting as an audio source.";
  info->min_inputs = 0;
  info->max_inputs = 0;
  info->outputs = ((struct channel_segment_data *)segment->data)->channel->channels;
  
  info->fields[0].field = MIXED_BUFFER;
  info->fields[0].description = "The buffer to attach to the port.";
  info->fields[0].flags = MIXED_OUT | MIXED_SET;

  info->fields[1].field = MIXED_VOLUME;
  info->fields[1].description = "The volume scaling factor.";
  info->fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  info->fields[2].field = MIXED_CHANNEL_RESAMPLER;
  info->fields[2].description = "The function used to resample the audio if necessary.";
  info->fields[2].flags = MIXED_SEGMENT | MIXED_SET;

  info->fields[3].field = MIXED_BYPASS;
  info->fields[3].description = "Bypass the segment's processing.";
  info->fields[3].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;
  
  return info;
}

struct mixed_segment_info *drain_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));
  info->name = "drain";
  info->description = "Segment acting as an audio drain.";
  info->min_inputs = ((struct channel_segment_data *)segment->data)->channel->channels;
  info->max_inputs = info->min_inputs;
  info->outputs = 0;

  info->fields[0].field = MIXED_BUFFER;
  info->fields[0].description = "The buffer for audio data attached to the location.";
  info->fields[0].flags = MIXED_IN | MIXED_SET;

  info->fields[1].field = MIXED_VOLUME;
  info->fields[1].description = "The volume scaling factor.";
  info->fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  info->fields[2].field = MIXED_CHANNEL_RESAMPLER;
  info->fields[2].description = "The function used to resample the audio if necessary.";
  info->fields[2].flags = MIXED_SEGMENT | MIXED_SET;

  info->fields[3].field = MIXED_BYPASS;
  info->fields[3].description = "Bypass the segment's processing.";
  info->fields[3].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

  return info;
}

int initialize_resample_buffers(struct mixed_channel *channel, struct channel_segment_data *data){
  struct mixed_buffer **resample_buffers = 0;
  
  resample_buffers = calloc(channel->channels, sizeof(struct mixed_buffer *));
  if(!resample_buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  // Determine max number of samples in source array
  size_t samples = channel->size / channel->channels / mixed_samplesize(channel->encoding);

  size_t i;
  for(i=0; i<channel->channels; ++i){
    struct mixed_buffer *buffer = calloc(1, sizeof(struct mixed_buffer));
    resample_buffers[i] = buffer;
      
    if(!buffer){
      mixed_err(MIXED_OUT_OF_MEMORY);
      goto cleanup;
    }
      
    if(!mixed_make_buffer(samples, buffer)){
      goto cleanup;
    }
  }

  data->resample_buffers = resample_buffers;
  return 1;
  
 cleanup:
  if(resample_buffers){
    do{ --i;
      struct mixed_buffer *buffer = resample_buffers[i];
      if(buffer){
        mixed_free_buffer(buffer);
        free(buffer);
      }
    }while(i > 0);
    free(resample_buffers);
  }
  return 0;
}

int make_channel_internal(struct mixed_channel *channel, size_t samplerate, struct mixed_segment *segment){
  struct channel_segment_data *data = 0;
  struct mixed_buffer **buffers = 0;

  if(channel->encoding < MIXED_INT8 || MIXED_DOUBLE < channel->encoding){
    mixed_err(MIXED_UNKNOWN_ENCODING);
    goto cleanup;
  }

  if(channel->layout < MIXED_ALTERNATING || MIXED_SEQUENTIAL < channel->layout){
    mixed_err(MIXED_UNKNOWN_LAYOUT);
    goto cleanup;
  }

  data = calloc(1, sizeof(struct channel_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  buffers = calloc(channel->channels, sizeof(struct mixed_buffer *));
  if(!buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  if(samplerate != 0 && samplerate != channel->samplerate){
    if(!initialize_resample_buffers(channel, data)){
      goto cleanup;
    }
  }

  data->buffers = buffers;
  data->channel = channel;
  data->samplerate = samplerate;
  data->resampler = mixed_resample_linear;
  data->volume = 1.0;

  segment->free = channel_segment_free;
  segment->data = data;
  segment->get = source_segment_get;
  return 1;

 cleanup:
  if(data)
    free(data);

  if(buffers)
    free(buffers);
  return 0;
}

MIXED_EXPORT int mixed_make_segment_source(struct mixed_channel *channel, size_t samplerate, struct mixed_segment *segment){
  segment->mix = source_segment_mix;
  segment->info = source_segment_info;
  segment->set = source_segment_set;
  segment->set_out = channel_segment_set_buffer;
  return make_channel_internal(channel, samplerate, segment);
}

MIXED_EXPORT int mixed_make_segment_drain(struct mixed_channel *channel, size_t samplerate, struct mixed_segment *segment){
  segment->mix = drain_segment_mix;
  segment->info = drain_segment_info;
  segment->set = drain_segment_set;
  segment->set_in = channel_segment_set_buffer;
  return make_channel_internal(channel, samplerate, segment);
}
