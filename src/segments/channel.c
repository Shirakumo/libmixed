#include "../internal.h"

struct channel_data{
  struct mixed_buffer *in[8];
  struct mixed_buffer *out[8];
  channel_t in_channels;
  channel_t out_channels;
};

struct channel_data_2_to_5_1{
  struct mixed_buffer *in[8];
  struct mixed_buffer *out[8];
  channel_t in_channels;
  channel_t out_channels;
  struct lowpass_data lp[3];
  uint32_t delay_i;
  uint32_t delay_size;
  float *delay;
};

struct channel_data_2_to_4_0{
  struct mixed_buffer *in[8];
  struct mixed_buffer *out[8];
  channel_t in_channels;
  channel_t out_channels;
  struct lowpass_data lp;
  uint32_t delay_i;
  uint32_t delay_size;
  float *delay;
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
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&out, &frames, data->out[MIXED_MONO]);
  for(uint32_t i=0; i<frames; ++i){
    out[i] = (l[i]+r[i])*0.5;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_MONO]);
  
  return 1;
}

int channel_mix_mono_stereo(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *l, *r, *in;
  mixed_buffer_request_write(&l, &frames, data->out[MIXED_LEFT]);
  mixed_buffer_request_write(&r, &frames, data->out[MIXED_RIGHT]);
  mixed_buffer_request_read(&in, &frames, data->in[MIXED_MONO]);
  for(uint32_t i=0; i<frames; ++i){
    l[i] = in[i];
    r[i] = in[i];
  }
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_MONO]);
  
  return 1;
}

int channel_mix_stereo_4_0(struct mixed_segment *segment){
  struct channel_data_2_to_4_0 *data = (struct channel_data_2_to_4_0 *)segment->data;
  const float invsqrt = 1.0/sqrt(2);
  
  uint32_t frames = UINT32_MAX;
  uint32_t delay_i = data->delay_i;
  const uint32_t delay_size = data->delay_size;
  float *l, *r, *fl, *fr, *rl, *rr;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&rl, &frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_request_write(&rr, &frames, data->out[MIXED_RIGHT_REAR]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];
    float s = (li-ri)*invsqrt;
    // Surround low-pass 7kHz
    float r = lowpass(s, &data->lp);
    // 90 deg hilbert phase shift. This "automatically" induces a delay as well.
    float rri = hilbert(r, data->delay, delay_size, delay_i);
    delay_i = (delay_i+1) % delay_size;
    float rli = 1.0f-rri;

    fl[i] = li;
    fr[i] = ri;
    rl[i] = rli;
    rr[i] = rri;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_REAR]);
  data->delay_i = delay_i;

  return 1;
}

// Based on Real-Time Conversion of Stereo Audio to 5.1 Channel Audio for Providing Realistic Sounds by Chan Jun Chun et al.
//   https://core.ac.uk/download/pdf/25789335.pdf
int channel_mix_stereo_5_1(struct mixed_segment *segment){
  struct channel_data_2_to_5_1 *data = (struct channel_data_2_to_5_1 *)segment->data;
  const float invsqrt = 1.0/sqrt(2);
  
  uint32_t frames = UINT32_MAX;
  uint32_t delay_i = data->delay_i;
  const uint32_t delay_size = data->delay_size;
  float *l, *r, *fl, *fr, *rl, *rr, *ce, *lfe;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&rl, &frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_request_write(&rr, &frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_request_write(&ce, &frames, data->out[MIXED_CENTER]);
  mixed_buffer_request_write(&lfe, &frames, data->out[MIXED_SUBWOOFER]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];
    float c = (li+ri)*invsqrt;
    float s = (li-ri)*invsqrt;
    // Center low-pass 4kHz
    float ci = lowpass(c, &data->lp[0]);
    // Subwoofer low-pass 200Hz
    float lfei = lowpass(c, &data->lp[1]);
    // Surround low-pass 7kHz
    float r = lowpass(s, &data->lp[2]);
    // 90 deg hilbert phase shift. This "automatically" induces a delay as well.
    float rri = hilbert(r, data->delay, delay_size, delay_i);
    delay_i = (delay_i+1) % delay_size;
    float rli = 1.0f-rri;

    fl[i] = li;
    fr[i] = ri;
    rl[i] = rli;
    rr[i] = rri;
    ce[i] = ci;
    lfe[i] = lfei;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_CENTER]);
  mixed_buffer_finish_write(frames, data->out[MIXED_SUBWOOFER]);
  data->delay_i = delay_i;

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

MIXED_EXPORT int mixed_make_segment_channel_convert(channel_t in, channel_t out, uint32_t samplerate, struct mixed_segment *segment){
  if(in == 1 && out == 2){
    struct channel_data *data = calloc(1, sizeof(struct channel_data));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    
    data->in_channels = in;
    data->out_channels = out;
    segment->mix = channel_mix_mono_stereo;
    segment->data = data;
  }else if(in == 2 && out == 1){
    struct channel_data *data = calloc(1, sizeof(struct channel_data));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }

    data->in_channels = in;
    data->out_channels = out;
    segment->mix = channel_mix_stereo_mono;
    segment->data = data;
  }else if(in == 2 && out == 4){
    struct channel_data_2_to_4_0 *data = calloc(1, sizeof(struct channel_data_2_to_4_0));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    
    data->in_channels = in;
    data->out_channels = out;
    data->delay_size = (samplerate*12)/1000;
    data->delay = calloc(data->delay_size, sizeof(float));
    if(!data->delay){
      free(data);
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    lowpass_init(samplerate, 7000, &data->lp);
    segment->mix = channel_mix_stereo_4_0;
    segment->data = data;
  }else if(in == 2 && out == 6){
    struct channel_data_2_to_5_1 *data = calloc(1, sizeof(struct channel_data_2_to_5_1));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    
    data->in_channels = in;
    data->out_channels = out;
    data->delay_size = (samplerate*12)/1000;
    data->delay = calloc(data->delay_size, sizeof(float));
    if(!data->delay){
      free(data);
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    lowpass_init(samplerate, 4000, &data->lp[0]);
    lowpass_init(samplerate,  200, &data->lp[1]);
    lowpass_init(samplerate, 7000, &data->lp[2]);
    segment->mix = channel_mix_stereo_5_1;
    segment->data = data;
  }else{
    mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
    return 0;
  }

  segment->free = channel_free;
  segment->start = channel_start;
  segment->set_in = channel_set_in;
  segment->set_out = channel_set_out;
  segment->info = channel_info;
  return 1;
}

int __make_channel_convert(void *args, struct mixed_segment *segment){
  return mixed_make_segment_channel_convert(ARG(channel_t, 0), ARG(channel_t, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(channel_convert, __make_channel_convert, 3, {
    {.description = "in", .type = MIXED_UINT8},
    {.description = "out", .type = MIXED_UINT8},
    {.description = "samplerate", .type = MIXED_UINT32}})
