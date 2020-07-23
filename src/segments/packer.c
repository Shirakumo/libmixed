#include "internal.h"
#include "samplerate.h"

struct pack_segment_data{
  struct mixed_pack *pack;
  struct mixed_buffer **buffers;
  SRC_DATA *resample_data;
  SRC_STATE *resample_state;
  size_t samplerate;
  size_t max_frames;
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

// FIXME: by using separate states per channel we could alias the
//        SRC_DATA arrays with the input buffer arrays and avoid
//        extra allocation and copying on at least one side.

int source_segment_mix(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  struct mixed_pack *pack = data->pack;

  if(pack->samplerate == data->samplerate){
    mixed_buffer_from_pack(data->pack, data->buffers, data->volume);
  }else{
    SRC_DATA *src_data = data->resample_data;
    size_t frames = data->max_frames;
    uint8_t channels = pack->channels;
    size_t frames_to_bytes = channels * mixed_samplesize(pack->encoding);
    // Step 1: determine available space
    // FIXME: resampling changes frame count
    for(uint8_t c=0; c<channels; ++c)
      frames = MIN(frames, mixed_buffer_available_write(data->buffers[c]));
    // Step 2: unpack to contiguous floats
    mixed_transfer_function_from decoder = mixed_translator_from(pack->encoding);
    size_t bytes = SIZE_MAX;
    void *pack_data;
    mixed_pack_request_read(&pack_data, &bytes, pack);
    frames = MIN(frames, bytes / frames_to_bytes);
    decoder(pack_data, src_data->data_in, 1, frames*channels, data->volume); 
    // Step 3: resample
    src_data->input_frames = frames;
    int e = src_process(data->resample_state, src_data);
    if(e){
      mixed_err(MIXED_RESAMPLE_FAILED);
      printf("%s\n", src_strerror(e));
      return 0;
    }
    // Step 4: transfer from contigous to separate buffers
    frames = src_data->output_frames_gen;
    float *source = src_data->data_out;
    for(uint8_t c=0; c<channels; ++c){
      float *target;
      size_t count = frames;
      mixed_buffer_request_write(&target, &count, data->buffers[c]);
      for(size_t i=0; i<count; ++i)
        target[i] = source[i*channels];
      source++;
      mixed_buffer_finish_write(count, data->buffers[c]);
    }
    // Step 5: update consumed samples
    mixed_pack_finish_read(src_data->input_frames_used * frames_to_bytes, pack);
  }
  return 1;
}

int drain_segment_mix(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  struct mixed_pack *pack = data->pack;

  if(pack->samplerate == data->samplerate){
    mixed_buffer_to_pack(data->buffers, pack, data->volume);
  }else{
    SRC_DATA *src_data = data->resample_data;
    size_t frames = data->max_frames;
    size_t channels = pack->channels;
    size_t frames_to_bytes = channels * mixed_samplesize(pack->encoding);
    size_t bytes = SIZE_MAX;
    void *pack_data;
    mixed_pack_request_write(&pack_data, &bytes, pack);
    frames = MIN(frames, bytes / frames_to_bytes);
    // Step 1: pack to contigous, alternating float array
    for(size_t c=0; c<channels; ++c){
      float *source;
      float *target = src_data->data_in;
      mixed_buffer_request_read(&source, &frames, data->buffers[c]);
      for(size_t i=0; i<frames; ++i)
        target[c+i*channels] = source[i];
    }
    // Step 2: resample
    src_data->input_frames = frames;
    int e = src_process(data->resample_state, src_data);
    if(e){
      printf("%s\n", src_strerror(e));
      mixed_error(MIXED_RESAMPLE_FAILED);
      return 0;
    }
    // Step 3: re-encode
    size_t out_frames = src_data->output_frames_gen;
    mixed_transfer_function_to encoder = mixed_translator_to(pack->encoding);
    encoder(src_data->data_out, pack_data, 1, out_frames*channels, data->volume);
    // Step 4: update consumed buffers
    mixed_pack_finish_write(out_frames, pack);
    for(size_t c=0; c<channels; ++c)
      mixed_buffer_finish_read(src_data->input_frames_used, data->buffers[c]);
  }
  return 1;
}

int source_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_RESAMPLE_TYPE: {
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
        mixed_buffer_clear(data->buffers[i]);
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

  set_info_field(field++, MIXED_RESAMPLE_TYPE,
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

int initialize_resample_buffers(struct mixed_pack *pack, struct pack_segment_data *data, int quality, int is_source){
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
  
  int input_samples, output_samples;
  if(is_source){
    input_samples = pack->size / mixed_samplesize(pack->encoding);
    output_samples = 1+(int)(input_samples * src_data->src_ratio);
    data->max_frames = output_samples;
  }else{
    output_samples = pack->size / mixed_samplesize(pack->encoding);
    input_samples = 1+(int)(output_samples / src_data->src_ratio);
    data->max_frames = input_samples;
  }
  
  src_data->data_in = calloc(input_samples, mixed_samplesize(MIXED_FLOAT));
  src_data->data_out = calloc(output_samples, mixed_samplesize(MIXED_FLOAT));
  if(!src_data->data_in || !src_data->data_out){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  src_data->output_frames = output_samples / pack->channels;
  
  int err = 0;
  src_state = src_new(quality, pack->channels, &err);
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

int make_pack_internal(struct mixed_pack *pack, size_t samplerate, int quality, struct mixed_segment *segment){
  struct pack_segment_data *data = 0;
  struct mixed_buffer **buffers = 0;

  if(pack->encoding < MIXED_INT8 || MIXED_DOUBLE < pack->encoding){
    mixed_err(MIXED_UNKNOWN_ENCODING);
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

  data->buffers = buffers;
  data->pack = pack;
  data->samplerate = samplerate;
  data->volume = 1.0;

  segment->free = pack_segment_free;
  segment->data = data;
  segment->get = packer_segment_get;

  if(samplerate != 0 && samplerate != pack->samplerate){
    if(!initialize_resample_buffers(pack, data, quality, (segment->info == source_segment_info))){
      goto cleanup;
    }
  }

  return 1;

 cleanup:
  if(data)
    free(data);

  if(buffers)
    free(buffers);
  return 0;
}

MIXED_EXPORT int mixed_make_segment_unpacker(struct mixed_pack *pack, size_t samplerate, struct mixed_segment *segment){
  segment->mix = source_segment_mix;
  segment->info = source_segment_info;
  segment->set = source_segment_set;
  segment->set_out = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, MIXED_SINC_FASTEST, segment);
}

MIXED_EXPORT int mixed_make_segment_packer(struct mixed_pack *pack, size_t samplerate, struct mixed_segment *segment){
  segment->mix = drain_segment_mix;
  segment->info = drain_segment_info;
  segment->set = drain_segment_set;
  segment->set_in = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, MIXED_SINC_FASTEST, segment);
}
