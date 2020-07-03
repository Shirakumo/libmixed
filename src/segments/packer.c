#include "internal.h"
#include "samplerate.h"

struct pack_segment_data{
  struct mixed_packed_audio *pack;
  struct mixed_buffer **buffers;
  SRC_DATA *resample_data;
  SRC_STATE *resample_state;
  size_t samplerate;
  float volume;
};

int pack_segment_free(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  if(data){
    free(data->buffers);
    if(data->resample_data){
      free(data->resample_data);
    }
    if(data->resample_state){
      src_delete(data->resample_state);
    }
    free(data);
  }
  segment->data = 0;
  return 1;
}

int pack_segment_set_buffer(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  switch(field){
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

  if(data->resample_data){
    // FIXME: resampling
  }else{
    mixed_buffer_from_packed_audio(data->pack, data->buffers, samples, data->volume);
  }
  return 1;
}

int drain_segment_mix(size_t samples, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  if(data->resample_state){
    uint8_t channels = data->pack->channels;
    SRC_DATA *src_data = data->resample_data;
    float *buffers[channels];
    for(size_t i=0; i<channels; ++i)
      buffers[i] = data->buffers[i]->data;
    
    //mixed_transfer_array_to_alternating_float(buffers, data->resample_data->data_in, channels, samples, 1.0);
    src_data->input_frames = samples;
    src_process(data->resample_state, src_data);
    // FIXME: what if src does not provide the exact amount of samples we need
    // FIXME: get the data out again???
  }else{
    mixed_buffer_to_packed_audio(data->buffers, data->pack, samples, data->volume);
  }
  return 1;
}

int source_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_PACKED_AUDIO_RESAMPLE_TYPE: {
    int error;
    SRC_STATE *new = src_new(*(enum mixed_resample_type *)value, data->pack->channels, &error);
    if(!new) {
      mixed_err(MIXED_RESAMPLE_FAILED);
      return 0;
    }
    if(data->resample_state)
      src_delete(data->resample_state);
    data->resample_state = new;
  }
    return 1;
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
  case MIXED_BYPASS:
    if(*(bool *)value){
      memset(data->pack->data, 0, data->pack->size);
      segment->mix = mix_noop;
    }else{
      segment->mix = drain_segment_mix;
    }
    return 1;
  default:
    return source_segment_set(field, value, segment);
  }
}

// FIXME: add start method that checks for buffer completeness.

int packer_segment_get(size_t field, void *value, struct mixed_segment *segment){
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

  set_info_field(field++, MIXED_PACKED_AUDIO_RESAMPLE_TYPE,
                 MIXED_RESAMPLE_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET,
                 "The type of resampling algorithm used.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int drain_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  source_segment_info(info, segment);
  info->name = "packer";
  info->description = "Segment acting as an audio packer.";
  info->min_inputs = ((struct pack_segment_data *)segment->data)->pack->channels;
  info->max_inputs = info->min_inputs;
  info->outputs = 0;
  return 1;
}

int initialize_resample_buffers(struct mixed_packed_audio *pack, struct pack_segment_data *data, int is_source){
  SRC_DATA *src_data = 0;
  SRC_STATE *src_state = 0;
  
  src_data = calloc(1, sizeof(SRC_DATA));
  if(!src_data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  src_data->end_of_input = 0;
  src_data->src_ratio = (is_source)
    ? ((double)data->samplerate)/((double)pack->samplerate)
    : ((double)pack->samplerate)/((double)data->samplerate);
  
  int input_samples = (is_source)
    ? (pack->size / mixed_samplesize(pack->encoding))
    : 1+(int)(pack->size / mixed_samplesize(pack->encoding) / src_data->src_ratio);
  int output_samples = 1+(int)(input_samples * src_data->src_ratio);
  
  src_data->data_in = calloc(input_samples, mixed_samplesize(MIXED_FLOAT));
  src_data->data_out = calloc(output_samples, mixed_samplesize(MIXED_FLOAT));
  if(!src_data->data_in || !src_data->data_out){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  src_data->output_frames = output_samples / pack->channels;
  
  int err = 0;
  src_state = src_new(MIXED_SINC_FASTEST, pack->channels, &err);
  if(!src_state){
    mixed_err(MIXED_RESAMPLE_FAILED);
    goto cleanup;
  }
  
  data->resample_data = src_data;
  data->resample_state = src_state;
  return 1;
  
 cleanup:
  if(src_data){
    if(src_data->data_in)
      free((float *)src_data->data_in);
    if(src_data->data_out)
      free(src_data->data_out);
    free(src_data);
  }
  if(src_state){
    src_delete(src_state);
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
    if(!initialize_resample_buffers(pack, data, (segment->info == drain_segment_info))){
      goto cleanup;
    }
  }

  data->buffers = buffers;
  data->pack = pack;
  data->samplerate = samplerate;
  data->volume = 1.0;

  segment->free = pack_segment_free;
  segment->data = data;
  segment->get = packer_segment_get;
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
