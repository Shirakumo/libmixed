#include "../internal.h"

//FIXME: allow arbitrary buffers
struct volume_control_segment_data{
  struct mixed_buffer *in[2];
  struct mixed_buffer *out[2];
  float volume;
  float pan;
};

int volume_control_segment_free(struct mixed_segment *segment){
  if(segment->data)
    mixed_free(segment->data);
  segment->data = 0;
  return 1;
}

int volume_control_segment_start(struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  if(data->out[0] == 0 || data->in[0] == 0
  || data->out[1] == 0 || data->in[1] == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int volume_control_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: data->in[MIXED_LEFT] = (struct mixed_buffer *)buffer; return 1;
    case MIXED_RIGHT: data->in[MIXED_RIGHT] = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int volume_control_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: data->out[MIXED_LEFT] = (struct mixed_buffer *)buffer; return 1;
    case MIXED_RIGHT: data->out[MIXED_RIGHT] = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0; break;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int volume_control_segment_mix(struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  float lvolume = data->volume * ((0.0<data->pan)?(1.0f-data->pan):1.0f);
  float rvolume = data->volume * ((data->pan<0.0)?(1.0f+data->pan):1.0f);
  float *in, *out;
  uint32_t samples;

  mixed_buffer_request_read(&in, &samples, data->in[MIXED_LEFT]);
  mixed_buffer_request_write(&out, &samples, data->out[MIXED_LEFT]);
  for(uint32_t i=0; i<samples; ++i)
    out[i] = in[i]*lvolume;
  mixed_buffer_finish_read(samples, data->in[MIXED_LEFT]);
  mixed_buffer_finish_write(samples, data->out[MIXED_LEFT]);
  
  mixed_buffer_request_read(&in, &samples, data->in[MIXED_RIGHT]);
  mixed_buffer_request_write(&out, &samples, data->out[MIXED_RIGHT]);
  for(uint32_t i=0; i<samples; ++i)
    out[i] = in[i]*rvolume;
  mixed_buffer_finish_read(samples, data->in[MIXED_RIGHT]);
  mixed_buffer_finish_write(samples, data->out[MIXED_RIGHT]);
  return 1;
}

int volume_control_segment_mix_bypass(struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;

  mixed_buffer_transfer(data->in[MIXED_LEFT], data->out[MIXED_LEFT]);
  mixed_buffer_transfer(data->in[MIXED_RIGHT], data->out[MIXED_RIGHT]);
  return 1;
}

int volume_control_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  info->name = "volume_control";
  info->description = "General segment for volume adjustment and panning.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 2;
  info->max_inputs = 2;
  info->outputs = 2;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_VOLUME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The volume scaling factor for the input.");

  set_info_field(field++, MIXED_VOLUME_CONTROL_PAN,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The left/right stereo panning.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int volume_control_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  switch(field){
  case MIXED_VOLUME: *((float *)value) = data->volume; break;
  case MIXED_VOLUME_CONTROL_PAN: *((float *)value) = data->pan; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == volume_control_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int volume_control_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  switch(field){
  case MIXED_VOLUME:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->volume = *(float *)value;
    break;
  case MIXED_VOLUME_CONTROL_PAN: 
    if(*(float *)value < -1.0 ||
       1.0 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->pan = *(float *)value;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = volume_control_segment_mix_bypass;
    }else{
      segment->mix = volume_control_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_volume_control(float volume, float pan, struct mixed_segment *segment){
  struct volume_control_segment_data *data = mixed_calloc(1, sizeof(struct volume_control_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->volume = volume;
  data->pan = pan;
  
  segment->free = volume_control_segment_free;
  segment->start = volume_control_segment_start;
  segment->mix = volume_control_segment_mix;
  segment->set_in = volume_control_segment_set_in;
  segment->set_out = volume_control_segment_set_out;
  segment->info = volume_control_segment_info;
  segment->get = volume_control_segment_get;
  segment->set = volume_control_segment_set;
  segment->data = data;
  return 1;
}

int __make_volume_control(void *args, struct mixed_segment *segment){
  return mixed_make_segment_volume_control(ARG(float, 0), ARG(float, 1), segment);
}

REGISTER_SEGMENT(volume_control, __make_volume_control, 2, {
    {.description = "volume", .type = MIXED_FLOAT},
    {.description = "pan", .type = MIXED_FLOAT}})
