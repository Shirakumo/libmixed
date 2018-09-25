#include "internal.h"

//FIXME: allow arbitrary buffers
struct volume_control_segment_data{
  struct mixed_buffer *in[2];
  struct mixed_buffer *out[2];
  float volume;
  float pan;
};

int volume_control_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int volume_control_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
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

int volume_control_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
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

int volume_control_segment_mix(size_t samples, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  float lvolume = data->volume * ((0.0<data->pan)?(1.0f-data->pan):1.0f);
  float rvolume = data->volume * ((data->pan<0.0)?(1.0f+data->pan):1.0f);

  for(size_t i=0; i<samples; ++i){
    data->out[MIXED_LEFT]->data[i] = data->in[MIXED_LEFT]->data[i]*lvolume;
    data->out[MIXED_RIGHT]->data[i] = data->in[MIXED_RIGHT]->data[i]*rvolume;
  }
  return 1;
}

int volume_control_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;

  mixed_buffer_copy(data->in[MIXED_LEFT], data->out[MIXED_LEFT]);
  mixed_buffer_copy(data->in[MIXED_RIGHT], data->out[MIXED_RIGHT]);
  return 1;
}

int volume_control_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
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

int volume_control_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct volume_control_segment_data *data = (struct volume_control_segment_data *)segment->data;
  switch(field){
  case MIXED_VOLUME: *((float *)value) = data->volume; break;
  case MIXED_VOLUME_CONTROL_PAN: *((float *)value) = data->pan; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == volume_control_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int volume_control_segment_set(size_t field, void *value, struct mixed_segment *segment){
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
  struct volume_control_segment_data *data = calloc(1, sizeof(struct volume_control_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->volume = volume;
  data->pan = pan;
  
  segment->free = volume_control_segment_free;
  segment->mix = volume_control_segment_mix;
  segment->set_in = volume_control_segment_set_in;
  segment->set_out = volume_control_segment_set_out;
  segment->info = volume_control_segment_info;
  segment->get = volume_control_segment_get;
  segment->set = volume_control_segment_set;
  segment->data = data;
  return 1;
}
