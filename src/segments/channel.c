#include "../internal.h"

struct channel_data{
  struct mixed_buffer *in[14];
  struct mixed_buffer *out[14];
  mixed_channel_t in_channels;
  mixed_channel_t out_channels;
  struct biquad_data lp[3];
  uint32_t delay_i;
  uint32_t delay_size;
  float *delay;
};

int channel_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int channel_start(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  for(mixed_channel_t i=0; i<data->in_channels; ++i){
    if(data->in[i] == 0){
      mixed_err(MIXED_BUFFER_MISSING);
      return 0;
    }
  }
  for(mixed_channel_t i=0; i<data->out_channels; ++i){
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

int channel_mix_transfer(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  for(mixed_channel_t c=0; c<data->in_channels; ++c)
    mixed_buffer_transfer(data->in[c], data->out[c]);
  return 1;
}

VECTORIZE int channel_mix_2_0_to_1_0(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict out;
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

VECTORIZE int channel_mix_1_0_to_2_0(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict in;
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

VECTORIZE int channel_mix_2_0_to_3_0(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  
  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict fl, *restrict fr, *restrict fc;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&fc, &frames, data->out[2]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];
    float c = (li+ri)*0.5f;

    fl[i] = li;
    fr[i] = ri;
    fc[i] = c;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[2]);

  return 1;
}

VECTORIZE int channel_mix_2_0_to_4_0(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  
  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict fl, *restrict fr, *restrict rl, *restrict rr;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&rl, &frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_request_write(&rr, &frames, data->out[MIXED_RIGHT_REAR]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];

    fl[i] = li;
    fr[i] = ri;
    rl[i] = li;
    rr[i] = ri;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_REAR]);

  return 1;
}

VECTORIZE int channel_mix_2_0_to_5_0(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict fl, *restrict fr, *restrict rl, *restrict rr, *restrict ce;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&rl, &frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_request_write(&rr, &frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_request_write(&ce, &frames, data->out[MIXED_CENTER]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];

    float c = (li+ri)*0.5f;
    float ci = biquad_sample(c, &data->lp[0]);
    float rli = 0.571f * (li + (li - 0.5f * c));
    float rri = 0.571f * (ri + (ri - 0.5f * c));

    fl[i] = li;
    fr[i] = ri;
    rl[i] = rli;
    rr[i] = rri;
    ce[i] = ci;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_CENTER]);
  return 1;
}

// Based on Real-Time Conversion of Stereo Audio to 5.1 Channel Audio for Providing Realistic Sounds by Chan Jun Chun et al.
//   https://core.ac.uk/download/pdf/25789335.pdf
VECTORIZE int channel_mix_2_0_to_5_1(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict fl, *restrict fr, *restrict rl, *restrict rr, *restrict ce, *restrict lfe;
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

    float c = (li+ri)*0.5f;
    float ci = biquad_sample(c, &data->lp[0]);
    float lfei = biquad_sample(c, &data->lp[1]);
    float rli = 0.571f * (li + (li - 0.5f * c));
    float rri = 0.571f * (ri + (ri - 0.5f * c));

    // FIXME: Hilbert is fucked. Reinstate when it works again.
    /* float r = biquad_sample(s, &data->lp[2]); */
    /* // 90 deg hilbert phase shift. This "automatically" induces a delay as well. */
    /* float rri = hilbert(r, delay, delay_size, delay_i); */
    /* delay_i = (delay_i+1) % delay_size; */
    /* float rli = -rri; */

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

  return 1;
}

VECTORIZE int channel_mix_2_0_to_7_1(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  
  uint32_t frames = UINT32_MAX;
  float *restrict l, *restrict r, *restrict fl, *restrict fr, *restrict sl, *restrict sr, *restrict rl, *restrict rr, *restrict ce, *restrict lfe;
  mixed_buffer_request_read(&l, &frames, data->in[MIXED_LEFT]);
  mixed_buffer_request_read(&r, &frames, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&fl, &frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_request_write(&fr, &frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_request_write(&sl, &frames, data->out[MIXED_LEFT_SIDE]);
  mixed_buffer_request_write(&sr, &frames, data->out[MIXED_RIGHT_SIDE]);
  mixed_buffer_request_write(&rl, &frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_request_write(&rr, &frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_request_write(&ce, &frames, data->out[MIXED_CENTER]);
  mixed_buffer_request_write(&lfe, &frames, data->out[MIXED_SUBWOOFER]);
  for(uint32_t i=0; i<frames; ++i){
    float li = l[i];
    float ri = r[i];

    float c = (li+ri)*0.5f;
    float ci = biquad_sample(c, &data->lp[0]);
    float lfei = biquad_sample(c, &data->lp[1]);
    float rli = 0.571f * (li + (li - 0.5f * c));
    float rri = 0.571f * (ri + (ri - 0.5f * c));

    fl[i] = li;
    fr[i] = ri;
    sl[i] = (li+rli)*0.5f;
    sr[i] = (ri+rri)*0.5f;
    rl[i] = rli;
    rr[i] = rri;
    ce[i] = ci;
    lfe[i] = lfei;
  }
  mixed_buffer_finish_read(frames, data->in[MIXED_LEFT]);
  mixed_buffer_finish_read(frames, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_FRONT]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_SIDE]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_SIDE]);
  mixed_buffer_finish_write(frames, data->out[MIXED_LEFT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_RIGHT_REAR]);
  mixed_buffer_finish_write(frames, data->out[MIXED_CENTER]);
  mixed_buffer_finish_write(frames, data->out[MIXED_SUBWOOFER]);

  return 1;
}

static int channel_update(struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;

  if(data->in_channels == data->out_channels)                segment->mix = channel_mix_transfer;
  else if(data->in_channels == 1 && data->out_channels == 2) segment->mix = channel_mix_1_0_to_2_0;
  else if(data->in_channels == 2 && data->out_channels == 1) segment->mix = channel_mix_2_0_to_1_0;
  else if(data->in_channels == 2 && data->out_channels == 3) segment->mix = channel_mix_2_0_to_3_0;
  else if(data->in_channels == 2 && data->out_channels == 4) segment->mix = channel_mix_2_0_to_4_0;
  else if(data->in_channels == 2 && data->out_channels == 5) segment->mix = channel_mix_2_0_to_5_0;
  else if(data->in_channels == 2 && data->out_channels == 6) segment->mix = channel_mix_2_0_to_5_1;
  else if(data->in_channels == 2 && data->out_channels == 8) segment->mix = channel_mix_2_0_to_7_1;
  else{
    mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
    return 0;
  }
  return 1;
}

int channel_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  switch(field){
  case MIXED_CHANNEL_COUNT_IN:
    *((mixed_channel_t *)value) = data->in_channels;
    break;
  case MIXED_CHANNEL_COUNT_OUT:
    *((mixed_channel_t *)value) = data->out_channels;
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

int channel_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct channel_data *data = (struct channel_data *)segment->data;
  mixed_channel_t channels = 0;
  switch(field){
  case MIXED_CHANNEL_COUNT_IN:
    channels = data->in_channels;
    data->in_channels = *((mixed_channel_t *)value);
    if(!channel_update(segment)){
      data->in_channels = channels;
      return 0;
    }
    break;
  case MIXED_CHANNEL_COUNT_OUT:
    channels = data->out_channels;
    data->out_channels = *((mixed_channel_t *)value);
    if(!channel_update(segment)){
      data->out_channels = channels;
      return 0;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
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

  set_info_field(field++, MIXED_CHANNEL_COUNT_IN,
                 MIXED_CHANNEL_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The number of input channels.");

  set_info_field(field++, MIXED_CHANNEL_COUNT_OUT,
                 MIXED_CHANNEL_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The number of output channels.");

  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_channel_convert(mixed_channel_t in, mixed_channel_t out, uint32_t samplerate, struct mixed_segment *segment){
  struct channel_data *data = mixed_calloc(1, sizeof(struct channel_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->in_channels = in;
  data->out_channels = out;
  data->delay_size = (samplerate*12)/1000;
  data->delay = mixed_calloc(data->delay_size, sizeof(float));
  if(!data->delay){
    mixed_free(data);
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  biquad_lowpass(samplerate, 4000, 1, &data->lp[0]);
  biquad_lowpass(samplerate,  200, 1, &data->lp[1]);
  biquad_lowpass(samplerate, 7000, 1, &data->lp[2]);

  segment->data = data;
  if(!channel_update(segment)){
    mixed_free(data->delay);
    mixed_free(data);
    segment->data = 0;
    mixed_err(MIXED_BAD_CHANNEL_CONFIGURATION);
    return 0;
  }

  segment->free = channel_free;
  segment->start = channel_start;
  segment->set = channel_set;
  segment->get = channel_get;
  segment->set_in = channel_set_in;
  segment->set_out = channel_set_out;
  segment->info = channel_info;
  return 1;
}

int __make_channel_convert(void *args, struct mixed_segment *segment){
  return mixed_make_segment_channel_convert(ARG(mixed_channel_t, 0), ARG(mixed_channel_t, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(channel_convert, __make_channel_convert, 3, {
    {.description = "in", .type = MIXED_UINT8},
    {.description = "out", .type = MIXED_UINT8},
    {.description = "samplerate", .type = MIXED_UINT32}})
