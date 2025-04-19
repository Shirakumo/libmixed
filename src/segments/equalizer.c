#include "../internal.h"

struct equalizer_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  struct fft_window_data fft_window_data;
  uint32_t samplerate;
  float mix;
  float bands[8];
};

int equalizer_segment_free(struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;
  if(data){
    free_fft_window_data(&data->fft_window_data);
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int equalizer_segment_start(struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int equalizer_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;

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

int equalizer_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;

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

static inline float catmullrom(float t, float p0, float p1, float p2, float p3){
  return 0.5f * ((2 * p1) +
			     (-p0 + p2) * t +
			     (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t +
			     (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t);
}

VECTORIZE void fft_equalize(struct fft_window_data *data, void *user){
  struct equalizer_segment_data *user_data = (struct equalizer_segment_data *)user;
  uint32_t framesize = data->framesize;
  float *fft_workspace = data->fft_workspace;
  float *bands = user_data->bands;

  // Change to mag/phase
  for(uint32_t i = 0; i < framesize; i+= 2){
    float re = fft_workspace[i+0];
    float im = fft_workspace[i+1];
    fft_workspace[i+0] = sqrtf(re * re + im * im) * 2;
    fft_workspace[i+1] = atan2f(im, re);
  }

  // Perform band eq
  for(uint32_t p = 0; p < framesize; p+= 2){
    int i = (int)floor(sqrt(p / (float)(framesize)) * (framesize / 2));
	int p2 = (i / (framesize / 16));
	int p1 = p2 - 1;
	int p0 = p1 - 1;
	int p3 = p2 + 1;
	if (p1 < 0) p1 = 0;
	if (p0 < 0) p0 = 0;
	if (p3 > 7) p3 = 7;
	float v = (float)(i % (framesize / 16)) / (float)(framesize / 16);
	fft_workspace[p] *= catmullrom(v, bands[p0], bands[p1], bands[p2], bands[p3]);
  }

  // Change back to complex
  for(uint32_t i = 0; i < framesize; i+= 2){
	float mag = fft_workspace[i+0];
	float pha = fft_workspace[i+1];
	fft_workspace[i+0] = cosf(pha) * mag;
	fft_workspace[i+1] = sinf(pha) * mag;
  }
}

int equalizer_segment_mix(struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;

  float *in, *out;
  uint32_t samples = UINT32_MAX;
  float mix = data->mix;
  mixed_buffer_request_read(&in, &samples, data->in);
  mixed_buffer_request_write(&out, &samples, data->out);
  fft_window(in, out, samples, &data->fft_window_data, fft_equalize, data);
  for(uint32_t i=0; i<samples; ++i){
    out[i] = LERP(in[i], out[i], mix);
  }
  mixed_buffer_finish_read(samples, data->in);
  mixed_buffer_finish_write(samples, data->out);
  return 1;
}

int equalizer_segment_mix_bypass(struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;
  
  return mixed_buffer_transfer(data->in, data->out);
}

int equalizer_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "equalizer";
  info->description = "Perform frequency equalization over 8 bands.";
  info->flags = MIXED_INPLACE;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_EQUALIZER_BAND,
                 MIXED_BUFFER_POINTER, 8, MIXED_SEGMENT | MIXED_SET,
                 "Access the 8 equalization band factors.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_MIX,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "How much of the output to mix with the input.");

  set_info_field(field++, MIXED_BYPASS,
                 MIXED_BOOL, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Bypass the segment's processing.");
  
  clear_info_field(field++);
  return 1;
}

int equalizer_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_MIX: *((float *)value) = data->mix; break;
  case MIXED_BYPASS: *((bool *)value) = (segment->mix == equalizer_segment_mix_bypass); break;
  case MIXED_EQUALIZER_BAND: memcpy(value, data->bands, sizeof(float)*8); break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int equalizer_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct equalizer_segment_data *data = (struct equalizer_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
    free_fft_window_data(&data->fft_window_data);
    if(!make_fft_window_data(2048, 4, data->samplerate, &data->fft_window_data)){
      return 0;
    }
    break;
  case MIXED_EQUALIZER_BAND:
    memcpy(data->bands, value, sizeof(float)*8);
    break;
  case MIXED_MIX:
    if(*(float *)value < 0 || 1 < *(float *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->mix = *(float *)value;
    if(data->mix == 0){
      bool bypass = 1;
      return equalizer_segment_set(MIXED_BYPASS, &bypass, segment);
    }else{
      bool bypass = 0;
      return equalizer_segment_set(MIXED_BYPASS, &bypass, segment);
    }
    break;
  case MIXED_BYPASS:
    if(*(bool *)value){
      segment->mix = equalizer_segment_mix_bypass;
    }else{
      segment->mix = equalizer_segment_mix;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_equalizer(float bands[8], uint32_t samplerate, struct mixed_segment *segment){
  struct equalizer_segment_data *data = mixed_calloc(1, sizeof(struct equalizer_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  if(!make_fft_window_data(2048, 4, samplerate, &data->fft_window_data)){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  memcpy(data->bands, bands, sizeof(float)*8);
  data->samplerate = samplerate;
  data->mix = 1.0;
  
  segment->free = equalizer_segment_free;
  segment->start = equalizer_segment_start;
  segment->mix = equalizer_segment_mix;
  segment->set_in = equalizer_segment_set_in;
  segment->set_out = equalizer_segment_set_out;
  segment->info = equalizer_segment_info;
  segment->get = equalizer_segment_get;
  segment->set = equalizer_segment_set;
  segment->data = data;
  return 1;

 cleanup:
  free_fft_window_data(&data->fft_window_data);
  mixed_free(data);
  return 0;
}

int __make_equalizer(void *args, struct mixed_segment *segment){
  return mixed_make_segment_equalizer(ARG(float *, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(equalizer, __make_equalizer, 2, {
  {.description = "bands", .type = MIXED_POINTER},
  {.description = "samplerate", .type = MIXED_UINT32}})
