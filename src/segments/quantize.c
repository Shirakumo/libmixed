#include "../internal.h"

struct quantize_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  float steps;
  float mix;
};

int quantize_segment_free(struct mixed_segment *segment){
  if(segment->data)
    mixed_free(segment->data);
  segment->data = 0;
  return 1;
}

int quantize_segment_start(struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int quantize_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;

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

int quantize_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;

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

int quantize_segment_mix(struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;

  float steps = data->steps;
  float mix = data->mix;
  with_mixed_buffer_transfer(i, samples, in, data->in, out, data->out, {
      float s = in[i];
      float o = floor(s * steps) / steps;
      out[i] = LERP(s, o, mix);
    });
  return 1;
}

int quantize_segment_mix_bypass(struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int quantize_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  info->name = "quantize";
  info->description = "Quantize the signal to a specified number of intervals.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_MIX,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "How much of the output to mix with the input.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  clear_info_field(field++);
  return 1;
}

int quantize_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;
  switch(field){
  case MIXED_MIX: *((float *)value) = data->mix; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == quantize_segment_mix_bypass); break;
  case MIXED_QUANTIZE_STEPS: *((uint32_t *)value) = (uint32_t)data->steps; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int quantize_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct quantize_segment_data *data = (struct quantize_segment_data *)segment->data;
  switch(field){
  case MIXED_MIX:
    if(*(float *)value < 0 || 1 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mix = *(float *)value;
    if(data->mix == 0){
      bool bypass = 1;
      return quantize_segment_set(MIXED_BYPASS, &bypass, segment);
    }else{
      bool bypass = 0;
      return quantize_segment_set(MIXED_BYPASS, &bypass, segment);
    }
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = quantize_segment_mix_bypass;
    }else{
      segment->mix = quantize_segment_mix;
    }
    break;
  case MIXED_QUANTIZE_STEPS:
    data->steps = *(uint32_t *)value;
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_quantize(uint32_t steps, struct mixed_segment *segment){
  struct quantize_segment_data *data = mixed_calloc(1, sizeof(struct quantize_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->steps = steps;
  data->mix = 1.0;
  segment->free = quantize_segment_free;
  segment->start = quantize_segment_start;
  segment->mix = quantize_segment_mix;
  segment->set_in = quantize_segment_set_in;
  segment->set_out = quantize_segment_set_out;
  segment->info = quantize_segment_info;
  segment->get = quantize_segment_get;
  segment->set = quantize_segment_set;
  segment->data = data;
  return 1;
}

int __make_quantize(void *args, struct mixed_segment *segment){
  return mixed_make_segment_quantize(ARG(uint32_t, 0), segment);
}

REGISTER_SEGMENT(quantize, __make_quantize, 1, {
    {.description = "steps", .type = MIXED_UINT32}})
