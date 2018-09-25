#include "internal.h"

struct pitch_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct pitch_data pitch_data;
  size_t samplerate;
  float pitch;
};

int pitch_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free_pitch_data(&((struct pitch_segment_data *)segment->data)->pitch_data);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int pitch_segment_start(struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  return 1;
}

int pitch_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

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

int pitch_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

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

int pitch_segment_mix(size_t samples, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;

  if(data->pitch == 1.0){
    mixed_buffer_copy(data->in, data->out);
  }else{
    pitch_shift(data->pitch, data->in->data, data->out->data, samples, &data->pitch_data);
  }
  return 1;
}

int pitch_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  
  return mixed_buffer_copy(data->in, data->out);
}

int pitch_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  info->name = "pitch";
  info->description = "Shift the pitch of the audio.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_PITCH_SHIFT,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The amount of change in pitch that is excised.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int pitch_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  switch(field){
  case MIXED_PITCH_SHIFT: *((float *)value) = data->pitch; break;
  case MIXED_SAMPLERATE: *((size_t *)value) = data->samplerate; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == pitch_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int pitch_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct pitch_segment_data *data = (struct pitch_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(size_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(size_t *)value;
    free_pitch_data(&data->pitch_data);
    if(!make_pitch_data(2048, 4, data->samplerate, &data->pitch_data)){
      return 0;
    }
    break;
  case MIXED_PITCH_SHIFT:
    if(*(float *)value <= 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->pitch = *(float *)value;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = pitch_segment_mix_bypass;
    }else{
      segment->mix = pitch_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_pitch(float pitch, size_t samplerate, struct mixed_segment *segment){
  struct pitch_segment_data *data = calloc(1, sizeof(struct pitch_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  if(!make_pitch_data(2048, 4, samplerate, &data->pitch_data)){
    free(data);
    return 0;
  }

  data->pitch = pitch;
  data->samplerate = samplerate;
  
  segment->free = pitch_segment_free;
  segment->start = pitch_segment_start;
  segment->mix = pitch_segment_mix;
  segment->set_in = pitch_segment_set_in;
  segment->set_out = pitch_segment_set_out;
  segment->info = pitch_segment_info;
  segment->get = pitch_segment_get;
  segment->set = pitch_segment_set;
  segment->data = data;
  return 1;
}
