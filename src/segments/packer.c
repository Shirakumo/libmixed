#include "internal.h"

struct pack_segment_data{
  struct mixed_packed_audio *pack;
  struct mixed_buffer **buffers;
  struct mixed_buffer **resample_buffers;
  int (*resampler)(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples);
  size_t samplerate;
  float volume;
};

int pack_segment_free(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  if(data){
    if(data->resample_buffers){
      for(size_t i=0; i<data->pack->channels; ++i){
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

int pack_segment_set_buffer(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  switch(field){
  case MIXED_PACKED_AUDIO_RESAMPLER:
    data->resampler = (int (*)(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples))buffer;
    return 1;
  case MIXED_BUFFER:
    if(location<data->pack->channels){
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

int source_segment_mix(size_t samples, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  if(data->resample_buffers){
    size_t source_samples = samples * data->samplerate / data->pack->samplerate;
    mixed_buffer_from_packed_audio(data->pack, data->resample_buffers, source_samples, data->volume);
    for(size_t i=0; i<data->pack->channels; ++i){
      data->resampler(data->resample_buffers[i], data->pack->samplerate, data->buffers[i], data->samplerate, samples);
    }
  }else{
    mixed_buffer_from_packed_audio(data->pack, data->buffers, samples, data->volume);
  }
  return 1;
}

int drain_segment_mix(size_t samples, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  if(data->resample_buffers){
    for(size_t i=0; i<data->pack->channels; ++i){
      data->resampler(data->buffers[i], data->samplerate, data->resample_buffers[i], data->pack->samplerate, samples);
    }
    size_t drain_samples = samples * data->samplerate / data->pack->samplerate;
    mixed_buffer_to_packed_audio(data->resample_buffers, data->pack, drain_samples, data->volume);
  }else{
    mixed_buffer_to_packed_audio(data->buffers, data->pack, samples, data->volume);
  }
  return 1;
}

int source_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  case MIXED_BYPASS:
    if(*(bool *)value){
      for(size_t i=0; i<data->pack->channels; ++i){
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
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    return 1;
  case MIXED_BYPASS:
    if(*(bool *)value){
      memset(data->pack->data, 0, data->pack->size);
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
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
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

int source_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  info->name = "unpacker";
  info->description = "Segment acting as an audio unpacker.";
  info->min_inputs = 0;
  info->max_inputs = 0;
  info->outputs = ((struct pack_segment_data *)segment->data)->pack->channels;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_OUT | MIXED_SET,
                 "The buffer to attach to the port.");

  set_info_field(field++, MIXED_VOLUME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The volume scaling factor.");

  set_info_field(field++, MIXED_PACKED_AUDIO_RESAMPLER,
                 MIXED_FUNCTION, 1, MIXED_SEGMENT | MIXED_SET,
                 "The function used to resample the audio if necessary.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  return 1;
}

int drain_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
    info->name = "packer";
    info->description = "Segment acting as an audio packer.";
    info->min_inputs = ((struct pack_segment_data *)segment->data)->pack->channels;
    info->max_inputs = info->min_inputs;
    info->outputs = 0;

    struct mixed_segment_field_info *field = info->fields;
    set_info_field(field++, MIXED_BUFFER,
                   MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_SET,
                   "The buffer for audio data attached to the location.");

    set_info_field(field++, MIXED_VOLUME,
                   MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "The volume scaling factor.");

    set_info_field(field++, MIXED_PACKED_AUDIO_RESAMPLER,
                   MIXED_FUNCTION, 1, MIXED_SEGMENT | MIXED_SET,
                   "The function used to resample the audio if necessary.");

    set_info_field(field++, MIXED_BYPASS,
                   MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                   "Bypass the segment's processing.");
    return 1;
}

int initialize_resample_buffers(struct mixed_packed_audio *pack, struct pack_segment_data *data){
  struct mixed_buffer **resample_buffers = 0;
  
  resample_buffers = calloc(pack->channels, sizeof(struct mixed_buffer *));
  if(!resample_buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  // Determine max number of samples in source array
  size_t samples = pack->size / pack->channels / mixed_samplesize(pack->encoding);

  size_t i;
  for(i=0; i<pack->channels; ++i){
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

int make_pack_internal(struct mixed_packed_audio *pack, size_t samplerate, struct mixed_segment *segment){
  struct pack_segment_data *data = 0;
  struct mixed_buffer **buffers = 0;

  if(pack->encoding < MIXED_INT8 || MIXED_DOUBLE < pack->encoding){
    mixed_err(MIXED_UNKNOWN_ENCODING);
    goto cleanup;
  }

  if(pack->layout < MIXED_ALTERNATING || MIXED_SEQUENTIAL < pack->layout){
    mixed_err(MIXED_UNKNOWN_LAYOUT);
    goto cleanup;
  }

  data = calloc(1, sizeof(struct pack_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  buffers = calloc(pack->channels, sizeof(struct mixed_buffer *));
  if(!buffers){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  if(samplerate != 0 && samplerate != pack->samplerate){
    if(!initialize_resample_buffers(pack, data)){
      goto cleanup;
    }
  }

  data->buffers = buffers;
  data->pack = pack;
  data->samplerate = samplerate;
  data->resampler = mixed_resample_linear;
  data->volume = 1.0;

  segment->free = pack_segment_free;
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

MIXED_EXPORT int mixed_make_segment_unpacker(struct mixed_packed_audio *pack, size_t samplerate, struct mixed_segment *segment){
  segment->mix = source_segment_mix;
  segment->info = source_segment_info;
  segment->set = source_segment_set;
  segment->set_out = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, segment);
}

MIXED_EXPORT int mixed_make_segment_packer(struct mixed_packed_audio *pack, size_t samplerate, struct mixed_segment *segment){
  segment->mix = drain_segment_mix;
  segment->info = drain_segment_info;
  segment->set = drain_segment_set;
  segment->set_in = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, segment);
}
