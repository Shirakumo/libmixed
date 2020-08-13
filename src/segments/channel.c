#include "../internal.h"

struct channel_data{
  struct mixed_buffer *in[8];
  struct mixed_buffer *out[8];
  channel_t in_channels;
  channel_t out_channels;
};

int channel_free(struct mixed_segment *segment){
  if(segment->data){
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int channel_start(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  for(channel_t i=0; i<data->in_channels; ++i){
    if(data->in[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  for(channel_t i=0; i<data->out_channels; ++i){
    if(data->out[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  return 1;
}

int channel_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    if(data->out_channels <= location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->out[location] = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int channel_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(data->in_channels <= location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->in[location] = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int channel_mix_stereo_mono(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *l, *r, *out;
  mixed_buffer_request_read(&l, &frames, data->in[0]);
  mixed_buffer_request_read(&r, &frames, data->in[1]);
  mixed_buffer_request_write(&out, &frames, data->out[0]);
  for(uint32_t i=0; i<frames; ++i){
    out[i] = (l[i]+r[i])*0.5;
  }
  mixed_buffer_finish_read(frames, data->in[0]);
  mixed_buffer_finish_read(frames, data->in[1]);
  mixed_buffer_finish_write(frames, data->out[0]);
  
  return 1;
}

int channel_mix_mono_stereo(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *l, *r, *in;
  mixed_buffer_request_write(&l, &frames, data->out[0]);
  mixed_buffer_request_write(&r, &frames, data->out[1]);
  mixed_buffer_request_read(&in, &frames, data->in[0]);
  for(uint32_t i=0; i<frames; ++i){
    l[i] = in[i];
    r[i] = in[i];
  }
  mixed_buffer_finish_write(frames, data->out[0]);
  mixed_buffer_finish_write(frames, data->out[1]);
  mixed_buffer_finish_read(frames, data->in[0]);
  
  return 1;
}


int channel_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  info->name = "channel_convert";
  info->description = "Mixes multiple buffers together";
  info->min_inputs = data->in_channels;
  info->max_inputs = data->in_channels;
  info->outputs = data->out_channels;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");
  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_channel_convert(channel_t in, channel_t out, struct mixed_segment *segment){
  if(in == 1 && out == 2){
    segment->mix = channel_mix_mono_stereo;
  }else if(in == 2 && out == 1){
    segment->mix = channel_mix_stereo_mono;
  }else{
    mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
    return 0;
  }
  
  struct channel_data *data = calloc(1, sizeof(struct channel_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->in_channels = in;
  data->out_channels = out;
  
  segment->free = channel_free;
  segment->start = channel_start;
  segment->set_in = channel_set_in;
  segment->set_out = channel_set_out;
  segment->info = channel_info;
  segment->data = data;
  return 1;
}

int __make_channel_convert(void *args, struct mixed_segment *segment){
  return mixed_make_segment_channel_convert(ARG(channel_t, 0), ARG(channel_t, 1), segment);
}

REGISTER_SEGMENT(channel_convert, __make_channel_convert, 2, {
    {.description = "in", .type = MIXED_UINT8},
    {.description = "out", .type = MIXED_UINT8}})
