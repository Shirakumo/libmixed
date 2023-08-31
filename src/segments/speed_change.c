#include "../internal.h"
#include <samplerate.h>
#include <limits.h>

struct speed_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  SRC_STATE *resample_state;
  double speed;
};

int speed_segment_free(struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  if(data){
    if(data->resample_state){
      src_delete(data->resample_state);
    }
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int speed_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;

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

int speed_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;

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

int speed_segment_start(struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  if(data->resample_state)
    src_reset(data->resample_state);
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int speed_segment_mix(struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  SRC_DATA src_data = {0};
  uint32_t in = UINT32_MAX, out = UINT32_MAX;
  mixed_buffer_request_read((float **)&src_data.data_in, &in, data->in);
  mixed_buffer_request_write(&src_data.data_out, &out, data->out);
  src_data.src_ratio = 1.0 / data->speed;
  src_data.input_frames = in;
  src_data.output_frames = out;
  if(src_data.input_frames && src_data.output_frames){
    int e = src_process(data->resample_state, &src_data);
    if(e){
      printf("%s\n", src_strerror(e));
      mixed_err(MIXED_RESAMPLE_FAILED);
      return 0;
    }
    mixed_buffer_finish_read(src_data.input_frames_used, data->in);
    mixed_buffer_finish_write(src_data.output_frames_gen, data->out);
  }else{
    mixed_buffer_finish_write(0, data->out);
  }
  return 1;
}

int speed_segment_mix_bypass(struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int speed_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "speed";
  info->description = "Change the speed of the audio.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_SPEED_FACTOR,
                 MIXED_DOUBLE, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The amount of change in speed that is excised.");

  set_info_field(field++, MIXED_RESAMPLE_TYPE,
                 MIXED_RESAMPLE_TYPE_ENUM, 1, MIXED_SEGMENT | MIXED_SET,
                 "The type of resampling algorithm used.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int speed_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  switch(field){
  case MIXED_SPEED_FACTOR: *((double *)value) = data->speed; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == speed_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int speed_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct speed_segment_data *data = (struct speed_segment_data *)segment->data;
  switch(field){
  case MIXED_RESAMPLE_TYPE: {
    int error;
    SRC_STATE *new = src_new(*(enum mixed_resample_type *)value, 1, &error);
    if(!new) {
      mixed_err(MIXED_RESAMPLE_FAILED);
      return 0;
    }
    if(data->resample_state)
      src_delete(data->resample_state);
    data->resample_state = new;
  }
    return 1;
  case MIXED_SPEED_FACTOR:
    if(*(double *)value <= 0.0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->speed = *(double *)value;
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = speed_segment_mix_bypass;
    }else{
      segment->mix = speed_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_speed_change(double speed, struct mixed_segment *segment){
  if(speed <= 0.003 || 256.0 < speed){
    mixed_err(MIXED_BAD_RESAMPLE_FACTOR);
    return 0;
  }

  struct speed_segment_data *data = mixed_calloc(1, sizeof(struct speed_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  int err = 0;
  data->resample_state = src_new(MIXED_SINC_FASTEST, 1, &err);
  data->speed = speed;
  
  segment->free = speed_segment_free;
  segment->start = speed_segment_start;
  segment->mix = speed_segment_mix;
  segment->set_in = speed_segment_set_in;
  segment->set_out = speed_segment_set_out;
  segment->info = speed_segment_info;
  segment->get = speed_segment_get;
  segment->set = speed_segment_set;
  segment->data = data;
  return 1;
}

int __make_speed_change(void *args, struct mixed_segment *segment){
  return mixed_make_segment_speed_change(ARG(double, 0), segment);
}

REGISTER_SEGMENT(speed_change, __make_speed_change, 1, {
    {.description = "speed", .type = MIXED_DOUBLE}})
