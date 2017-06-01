#include "internal.h"

struct general_segment_data{
  struct mixed_buffer *in[2];
  struct mixed_buffer *out[2];
  float volume;
  float pan;
};

int general_segment_free(struct mixed_segment *segment){
  if(segment->data)
    free(segment->data);
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int general_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;

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

int general_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;

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

void general_segment_mix(size_t samples, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  float lvolume = data->volume * ((0.0<data->pan)?(1.0f-data->pan):1.0f);
  float rvolume = data->volume * ((data->pan<0.0)?(1.0f+data->pan):1.0f);

  for(size_t i=0; i<samples; ++i){
    data->out[MIXED_LEFT]->data[i] = data->in[MIXED_LEFT]->data[i]*lvolume;
    data->out[MIXED_RIGHT]->data[i] = data->in[MIXED_RIGHT]->data[i]*rvolume;
  }
}

void general_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;

  mixed_buffer_copy(data->in[MIXED_LEFT], data->out[MIXED_LEFT]);
  mixed_buffer_copy(data->in[MIXED_RIGHT], data->out[MIXED_RIGHT]);
}

struct mixed_segment_info *general_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info *info = calloc(1, sizeof(struct mixed_segment_info));

  if(info){
    info->name = "general";
    info->description = "General segment for volume adjustment and panning.";
    info->flags = MIXED_INPLACE;
    info->min_inputs = 2;
    info->max_inputs = 2;
    info->outputs = 2;
  
    info->fields[0].field = MIXED_BUFFER;
    info->fields[0].description = "The buffer for audio data attached to the location.";
    info->fields[0].flags = MIXED_IN | MIXED_OUT | MIXED_SET;

    info->fields[1].field = MIXED_VOLUME;
    info->fields[1].description = "The volume scaling factor for the input.";
    info->fields[1].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

    info->fields[2].field = MIXED_GENERAL_PAN;
    info->fields[2].description = "The left/right stereo panning.";
    info->fields[2].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;

    info->fields[3].field = MIXED_BYPASS;
    info->fields[3].description = "Bypass the segment's processing.";
    info->fields[3].flags = MIXED_SEGMENT | MIXED_SET | MIXED_GET;
  }
  
  return info;
}

int general_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(field){
  case MIXED_VOLUME: *((float *)value) = data->volume; break;
  case MIXED_GENERAL_PAN: *((float *)value) = data->pan; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == general_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int general_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct general_segment_data *data = (struct general_segment_data *)segment->data;
  switch(field){
  case MIXED_VOLUME:
    if(*(float *)value < 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->volume = *(float *)value;
    break;
  case MIXED_GENERAL_PAN: 
    if(*(float *)value < -1.0 ||
       1.0 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->pan = *(float *)value;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = general_segment_mix_bypass;
    }else{
      segment->mix = general_segment_mix;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_general(float volume, float pan, struct mixed_segment *segment){
  struct general_segment_data *data = calloc(1, sizeof(struct general_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->volume = volume;
  data->pan = pan;
  
  segment->free = general_segment_free;
  segment->mix = general_segment_mix;
  segment->set_in = general_segment_set_in;
  segment->set_out = general_segment_set_out;
  segment->info = general_segment_info;
  segment->get = general_segment_get;
  segment->set = general_segment_set;
  segment->data = data;
  return 1;
}
