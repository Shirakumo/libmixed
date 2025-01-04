#include "../internal.h"
#include "samplerate.h"

struct pack_segment_data{
  struct mixed_pack *pack;
  struct mixed_buffer *buffers[12];
  SRC_STATE *resample_state;
  uint32_t samplerate;
  float volume;
  float target_volume;
  int quality;
  float resample_in[512];
  float resample_out[512];
};

int pack_segment_free(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  if(data){
    if(data->resample_state)
      src_delete(data->resample_state);
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int pack_segment_set_buffer(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location<data->pack->channels && location<12){
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

int pack_segment_start(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  if(data->pack == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  
  if(12 < data->pack->channels){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }

  for(int i=0; i<data->pack->channels; ++i){
    if(data->buffers[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  
  double ratio = ((double)data->samplerate)/((double)data->pack->samplerate);
  if(ratio <= 0.003 || 256.0 < ratio){
    mixed_err(MIXED_BAD_RESAMPLE_FACTOR);
    return 0;
  }

  if(data->resample_state)
    src_reset(data->resample_state);
  else{
    int e = 0;
    SRC_STATE *src_state = src_new(data->quality, data->pack->channels, &e);
    if(!src_state){
      fprintf(stderr, "libsamplerate: %s\n", src_strerror(e));
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    data->resample_state = src_state;
  }
  return 1;
}

// FIXME: by using separate states per channel we could alias the
//        SRC_DATA arrays with the input buffer arrays and avoid
//        extra allocation and copying on at least one side.

int source_segment_mix(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  struct mixed_pack *pack = data->pack;

  if(pack->samplerate == data->samplerate){
    mixed_buffer_from_pack(data->pack, data->buffers, &data->volume, data->target_volume);
  }else{
    void *pack_data;
    float *target = data->resample_in;
    mixed_channel_t channels = pack->channels;
    uint32_t frames, buffer_frames = 512 / channels;
    uint32_t frames_to_bytes = channels * mixed_samplesize(pack->encoding);
    mixed_transfer_function_from decoder = mixed_translator_from(pack->encoding);
    SRC_DATA src_data = {0};
    src_data.src_ratio = ((double)data->samplerate)/((double)pack->samplerate);
    src_data.data_in = target;
    src_data.data_out = data->resample_out;
    do{
      // Step 1: determine available space
      frames = buffer_frames;
      for(mixed_channel_t c=0; c<channels; ++c)
        frames = MIN(frames, mixed_buffer_available_write(data->buffers[c]));
      src_data.output_frames = frames;
      // Step 2: unpack to contiguous floats
      uint32_t bytes = UINT32_MAX;
      mixed_pack_request_read(&pack_data, &bytes, pack);
      frames = MIN(frames, bytes / frames_to_bytes);
      if(pack_data){
        data->volume = decoder(pack_data, (float*)src_data.data_in, 1, frames*channels, data->volume, data->target_volume);
        // Step 3: resample
        src_data.input_frames = frames;
        int e = src_process(data->resample_state, &src_data);
        if(e){
          fprintf(stderr, "libsamplerate: %s\n", src_strerror(e));
          mixed_err(MIXED_RESAMPLE_FAILED);
          return 0;
        }
        // Step 4: transfer from contigous to separate buffers
        frames = src_data.input_frames_used;
        float *source = src_data.data_out;
        for(mixed_channel_t c=0; c<channels; ++c){
          uint32_t out_frames = src_data.output_frames_gen;
          mixed_buffer_request_write(&target, &out_frames, data->buffers[c]);
          for(uint32_t i=0; i<out_frames; ++i)
            target[i] = source[i*channels];
          source++;
          mixed_buffer_finish_write(out_frames, data->buffers[c]);
        }
        // Step 5: update consumed samples
        mixed_pack_finish_read(frames * frames_to_bytes, pack);
      }
    }while(frames);
  }
  return 1;
}

int drain_segment_mix(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  struct mixed_pack *pack = data->pack;

  if(pack->samplerate == data->samplerate){
    mixed_buffer_to_pack(data->buffers, pack, &data->volume, data->target_volume);
  }else{
    void *pack_data;
    float *target = data->resample_in;
    mixed_channel_t channels = pack->channels;
    uint32_t frames, buffer_frames = 512 / channels;
    uint32_t frames_to_bytes = channels * mixed_samplesize(pack->encoding);
    mixed_transfer_function_to encoder = mixed_translator_to(pack->encoding);
    SRC_DATA src_data = {0};
    src_data.src_ratio = ((double)pack->samplerate)/((double)data->samplerate);
    src_data.data_in = target;
    src_data.data_out = data->resample_out;
    do{
      uint32_t bytes = UINT32_MAX;
      // Interleave data, count frames
      mixed_pack_request_write(&pack_data, &bytes, pack);
      // If we don't even have 2 frames worth of data remaining to write, clear.
      if(bytes < 2*frames_to_bytes && mixed_pack_available_read(pack) == 0){
        mixed_pack_clear(pack);
        mixed_pack_request_write(&pack_data, &bytes, pack);
      }
      src_data.output_frames = MIN(buffer_frames, bytes / frames_to_bytes);
      frames = MIN(buffer_frames, (src_data.output_frames*data->samplerate) / pack->samplerate);
      if(pack_data){
        for(mixed_channel_t c=0; c<channels; ++c){
          float *source;
          mixed_buffer_request_read(&source, &frames, data->buffers[c]);
          for(uint32_t i=0; i<frames; ++i)
            target[c+i*channels] = source[i];
        }
        // Resample
        src_data.input_frames = frames;
        int e = src_process(data->resample_state, &src_data);
        if(e){
          fprintf(stderr, "libsamplerate: %s\n", src_strerror(e));
          mixed_err(MIXED_RESAMPLE_FAILED);
          return 0;
        }
        // Pack
        frames = src_data.input_frames_used;
        uint32_t out_frames = src_data.output_frames_gen;
        data->volume = encoder(src_data.data_out, pack_data, 1, out_frames*channels, data->volume, data->target_volume);
        // Update consumed buffers
        mixed_pack_finish_write(out_frames * frames_to_bytes, pack);
        for(mixed_channel_t c=0; c<channels; ++c){
          mixed_buffer_finish_read(frames, data->buffers[c]);
        }
      }
    }while(frames);
  }
  return 1;
}

int pack_segment_end(struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  if(data->resample_state){
    src_delete(data->resample_state);
    data->resample_state = 0;
  }
  return 1;
}

int source_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_RESAMPLE_TYPE: {
    int error;
    data->quality = *(enum mixed_resample_type *)value;
    if(data->resample_state){
      SRC_STATE *new = src_new(data->quality, data->pack->channels, &error);
      if(!new) {
        mixed_err(MIXED_RESAMPLE_FAILED);
        return 0;
      }
      if(data->resample_state)
        src_delete(data->resample_state);
      data->resample_state = new;
    }
  }
    return 1;
  case MIXED_VOLUME:
    data->target_volume = *((float *)value);
    return 1;
  case MIXED_BYPASS:
    if(*(bool *)value){
      for(mixed_channel_t i=0; i<data->pack->channels; ++i){
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

int drain_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
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

int packer_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct pack_segment_data *data = (struct pack_segment_data *)segment->data;
  
  switch(field){
  case MIXED_RESAMPLE_TYPE:
    *((int *)value) = data->quality;
    return 1;
  case MIXED_VOLUME:
    *((float *)value) = data->target_volume;
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
                 MIXED_RESAMPLE_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
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

int make_pack_internal(struct mixed_pack *pack, uint32_t samplerate, int quality, struct mixed_segment *segment){
  struct pack_segment_data *data = 0;

  if(pack->encoding < MIXED_INT8 || MIXED_DOUBLE < pack->encoding){
    mixed_err(MIXED_UNKNOWN_ENCODING);
    goto cleanup;
  }

  data = mixed_calloc(1, sizeof(struct pack_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  data->pack = pack;
  data->samplerate = samplerate;
  data->volume = 1.0;
  data->target_volume = 1.0;
  data->quality = quality;

  segment->free = pack_segment_free;
  segment->start = pack_segment_start;
  segment->end = pack_segment_end;
  segment->get = packer_segment_get;
  segment->data = data;
  return 1;

 cleanup:
  if(data)
    mixed_free(data);
  return 0;
}

MIXED_EXPORT int mixed_make_segment_unpacker(struct mixed_pack *pack, uint32_t samplerate, struct mixed_segment *segment){
  segment->mix = source_segment_mix;
  segment->info = source_segment_info;
  segment->set = source_segment_set;
  segment->set_out = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, MIXED_SINC_FASTEST, segment);
}

int __make_unpacker(void *args, struct mixed_segment *segment){
  return mixed_make_segment_unpacker(ARG(struct mixed_pack*, 0), ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(unpacker, __make_unpacker, 2, {
    {.description = "pack", .type = MIXED_PACK_POINTER},
    {.description = "samplerate", .type = MIXED_UINT32}})

MIXED_EXPORT int mixed_make_segment_packer(struct mixed_pack *pack, uint32_t samplerate, struct mixed_segment *segment){
  segment->mix = drain_segment_mix;
  segment->info = drain_segment_info;
  segment->set = drain_segment_set;
  segment->set_in = pack_segment_set_buffer;
  return make_pack_internal(pack, samplerate, MIXED_SINC_FASTEST, segment);
}

int __make_packer(void *args, struct mixed_segment *segment){
  return mixed_make_segment_packer(ARG(struct mixed_pack*, 0), ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(packer, __make_packer, 2, {
    {.description = "pack", .type = MIXED_PACK_POINTER},
    {.description = "samplerate", .type = MIXED_UINT32}})
