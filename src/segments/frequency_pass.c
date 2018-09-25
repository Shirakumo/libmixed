#include "internal.h"

struct frequency_pass_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  float x[2];
  float y[2];
  float a[2];
  float b[2];
  float k;
  size_t cutoff;
  size_t samplerate;
  enum mixed_frequency_pass pass;
};

void compute_coefficients(struct frequency_pass_segment_data *data){
  float a = tan(M_PI*data->cutoff/data->samplerate);
  float a2 = a*a;
  float s2 = sqrt(2);
  data->k = (a2)/(1+s2*a+a2);
  data->a[0] = (2*(a2-1))/(1+s2*a+a2);
  data->a[1] = (1-s2*a+a2)/(1+s2*a+a2);
  data->b[0] = 2;
  data->b[1] = 1;
  data->x[0] = 0;
  data->x[1] = 0;
  data->y[0] = 0;
  data->y[1] = 0;
}

int frequency_pass_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int frequency_pass_segment_start(struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;
  data->x[0] = 0;
  data->x[1] = 0;
  data->y[0] = 0;
  data->y[1] = 0;
  return 1;
}

int frequency_pass_segment_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;

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

int frequency_pass_segment_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;

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

int low_pass_segment_mix(size_t samples, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;

  float *in = data->in->data;
  float *out = data->out->data;
  float *x = data->x;
  float *y = data->y;
  float *a = data->a;
  float *b = data->b;
  float k = data->k;

  for(size_t i=0; i<samples; ++i){
    float s = in[i];
    out[i] = k*s + k*b[0]*x[0] + k*b[1]*x[1] - a[0]*y[0] - a[1]*y[1];
    x[1] = x[0];
    x[0] = s;
    y[1] = y[0];
    y[0] = out[i];
  }

  return 1;
}

int high_pass_segment_mix(size_t samples, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;

  float *in = data->in->data;
  float *out = data->out->data;
  float *x = data->x;
  float *y = data->y;
  float *a = data->a;
  float *b = data->b;
  float k = data->k;

  for(size_t i=0; i<samples; ++i){
    float s = in[i];
    out[i] = k*s + k*b[0]*x[0] + k*b[1]*x[1] - a[0]*y[0] - a[1]*y[1];
    x[1] = x[0];
    x[0] = s;
    y[1] = y[0];
    y[0] = out[i];
    out[i] = s-out[i];
  }

  return 1;
}

int frequency_pass_segment_mix_bypass(size_t samples, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;
  
  return mixed_buffer_copy(data->in, data->out);
}

int frequency_pass_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;
  
  info->name = "frequency_pass";
  info->description = "A frequency filter segment.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
    
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_FREQUENCY_CUTOFF,
                 MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The maximum/minimum frequency that will pass through the segment.");

  set_info_field(field++, MIXED_FREQUENCY_PASS,
                 MIXED_FREQUENCY_PASS_ENUM, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Whether to pass high or low frequencies.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_SIZE_T, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");

  clear_info_field(field++);
  return 1;
}

int frequency_pass_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((size_t *)value) = data->samplerate; break;
  case MIXED_FREQUENCY_CUTOFF: *((size_t *)value) = data->cutoff; break;
  case MIXED_FREQUENCY_PASS: *((enum mixed_frequency_pass *)value) = data->pass; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == frequency_pass_segment_mix_bypass); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int frequency_pass_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct frequency_pass_segment_data *data = (struct frequency_pass_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(size_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(size_t *)value;
    compute_coefficients(data);
    break;
  case MIXED_FREQUENCY_CUTOFF:
    if(*(size_t *)value <= 0 || data->samplerate <= *(size_t *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->cutoff = *(size_t *)value;
    compute_coefficients(data);
    break;
  case MIXED_FREQUENCY_PASS:
    if(*(enum mixed_frequency_pass *)value < MIXED_PASS_LOW ||
       MIXED_PASS_HIGH < *(enum mixed_frequency_pass *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->pass = *(enum mixed_frequency_pass *)value;
    if(segment->mix != frequency_pass_segment_mix_bypass){
      segment->mix = (data->pass == MIXED_PASS_LOW)
        ? low_pass_segment_mix
        : high_pass_segment_mix;
    }
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = frequency_pass_segment_mix_bypass;
    }else if(data->pass == MIXED_PASS_LOW){
      segment->mix = low_pass_segment_mix;
    }else{
      segment->mix = high_pass_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_frequency_pass(enum mixed_frequency_pass pass, size_t cutoff, size_t samplerate, struct mixed_segment *segment){
  if(samplerate <= cutoff){
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  }
  
  struct frequency_pass_segment_data *data = calloc(1, sizeof(struct frequency_pass_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->cutoff = cutoff;
  data->samplerate = samplerate;
  data->pass = pass;

  compute_coefficients(data);
  
  segment->free = frequency_pass_segment_free;
  segment->start = frequency_pass_segment_start;
  segment->mix = (pass == MIXED_PASS_LOW)? low_pass_segment_mix : high_pass_segment_mix;
  segment->set_in = frequency_pass_segment_set_in;
  segment->set_out = frequency_pass_segment_set_out;
  segment->info = frequency_pass_segment_info;
  segment->get = frequency_pass_segment_get;
  segment->set = frequency_pass_segment_set;
  segment->data = data;
  return 1;
}
