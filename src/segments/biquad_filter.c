#include "../internal.h"

struct biquad_filter_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct biquad_data data;
  struct biquad_data data_2;
  uint32_t samplerate;
  float frequency;
  float Q;
  float gain;
  enum mixed_biquad_filter type;
};

static int biquad_reinit(struct biquad_filter_segment_data *data){
  switch(data->type){
  case MIXED_LOWPASS: biquad_lowpass(data->samplerate, data->frequency, data->Q, &data->data_2); break;
  case MIXED_HIGHPASS: biquad_highpass(data->samplerate, data->frequency, data->Q, &data->data_2); break;
  case MIXED_BANDPASS: biquad_bandpass(data->samplerate, data->frequency, data->Q, &data->data_2); break;
  case MIXED_NOTCH: biquad_notch(data->samplerate, data->frequency, data->Q, &data->data_2); break;
  case MIXED_PEAKING: biquad_peaking(data->samplerate, data->frequency, data->Q, data->gain, &data->data_2); break;
  case MIXED_ALLPASS: biquad_allpass(data->samplerate, data->frequency, data->Q, &data->data_2); break;
  case MIXED_LOWSHELF: biquad_lowshelf(data->samplerate, data->frequency, data->Q, data->gain, &data->data_2); break;
  case MIXED_HIGHSHELF: biquad_highshelf(data->samplerate, data->frequency, data->Q, data->gain, &data->data_2); break;
  default:
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  return 1;
}

int biquad_filter_segment_free(struct mixed_segment *segment){
  if(segment->data){
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int biquad_filter_segment_start(struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;
  memcpy(&data->data, &data->data_2, sizeof(struct biquad_data));
  biquad_reset(&data->data);

  if(data->in == 0 || data->out == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int biquad_filter_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->in = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int biquad_filter_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(location == 0){
      data->out = (struct mixed_buffer *)buffer;
      return 1;
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int biquad_segment_mix(struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;
  biquad_process(data->in, data->out, &data->data);
  float a = 0.99f;
  float b = 1.f - a;

  data->data.a[0] = (data->data_2.a[0] * b) + (data->data.a[0] * a);
  data->data.a[1] = (data->data_2.a[1] * b) + (data->data.a[1] * a);
  data->data.b[0] = (data->data_2.b[0] * b) + (data->data.b[0] * a);
  data->data.b[1] = (data->data_2.b[1] * b) + (data->data.b[1] * a);
  data->data.b[2] = (data->data_2.b[2] * b) + (data->data.b[2] * a);
  return 1;
}

int biquad_filter_segment_mix_bypass(struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;
  return mixed_buffer_transfer(data->in, data->out);
}

int biquad_filter_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "biquad_filter";
  info->description = "A frequency filter segment.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
    
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_FREQUENCY,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The frequency that the filter attunes to.");

  set_info_field(field++, MIXED_BIQUAD_FILTER,
                 MIXED_BIQUAD_FILTER_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Which filter type to encompass.");

  set_info_field(field++, MIXED_Q,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The Q or resonance factor of the filter.");

  set_info_field(field++, MIXED_GAIN,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The gain on the filter.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  clear_info_field(field++);
  return 1;
}

int biquad_filter_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_FREQUENCY: *((float *)value) = data->frequency; break;
  case MIXED_Q: *((float *)value) = data->Q; break;
  case MIXED_GAIN: *((float *)value) = data->gain; break;
  case MIXED_BIQUAD_FILTER: *((enum mixed_biquad_filter *)value) = data->type; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == biquad_filter_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int biquad_filter_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct biquad_filter_segment_data *data = (struct biquad_filter_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
    biquad_reinit(data);
    break;
  case MIXED_FREQUENCY:
    if(*(float *)value <= 0 || data->samplerate < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->frequency = *(float *)value;
    biquad_reinit(data);
    break;
  case MIXED_BIQUAD_FILTER:
    if(*(enum mixed_biquad_filter *)value < MIXED_LOWPASS ||
       MIXED_HIGHSHELF < *(enum mixed_biquad_filter *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->type = *(enum mixed_biquad_filter *)value;
    biquad_reinit(data);
    break;
  case MIXED_Q:
    data->Q = *(float *)value;
    biquad_reinit(data);
    break;
  case MIXED_GAIN:
    data->gain = *(float *)value;
    biquad_reinit(data);
    break;
  case MIXED_BYPASS:
    biquad_reset(&data->data);
    if(*(bool *)value){
      segment->mix = biquad_filter_segment_mix_bypass;
    }else{
      segment->mix = biquad_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_biquad_filter(enum mixed_biquad_filter type, float frequency, uint32_t samplerate, struct mixed_segment *segment){
  if(samplerate <= frequency){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  
  struct biquad_filter_segment_data *data = mixed_calloc(1, sizeof(struct biquad_filter_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->samplerate = samplerate;
  data->frequency = frequency;
  data->Q = 1.0;
  data->gain = 1.0;
  data->type = type;

  if(!biquad_reinit(data)){
    mixed_free(data);
    return 0;
  }
  
  segment->free = biquad_filter_segment_free;
  segment->start = biquad_filter_segment_start;
  segment->mix = biquad_segment_mix;
  segment->set_in = biquad_filter_segment_set_in;
  segment->set_out = biquad_filter_segment_set_out;
  segment->info = biquad_filter_segment_info;
  segment->get = biquad_filter_segment_get;
  segment->set = biquad_filter_segment_set;
  segment->data = data;
  return 1;
}

int __make_biquad_filter(void *args, struct mixed_segment *segment){
  return mixed_make_segment_biquad_filter(ARG(enum mixed_biquad_filter, 0), ARG(uint32_t, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(biquad_filter, __make_biquad_filter, 3, {
    {.description = "type", .type = MIXED_BIQUAD_FILTER_ENUM},
    {.description = "frequency", .type = MIXED_UINT32},
    {.description = "samplerate", .type = MIXED_UINT32}})
