#include "../internal.h"

struct space_source{
  struct mixed_buffer *buffer;
  float location[3];
  float velocity[3];
  float min_distance;
  float max_distance;
  float rolloff;
};

struct space_mixer_data{
  struct space_source **sources;
  uint32_t count;
  uint32_t size;
  struct mixed_buffer *left;
  struct mixed_buffer *right;
  struct fft_window_data fft_window_data;
  float location[3];
  float velocity[3];
  float direction[3];
  float up[3];
  float soundspeed;
  float doppler_factor;
  float min_distance;
  float max_distance;
  float rolloff;
  float volume;
  float (*attenuation)(float min, float max, float dist, float roll);
};

int space_mixer_free(struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  if(data){
    free_fft_window_data(&data->fft_window_data);
    mixed_free(data->sources);
    mixed_free(data);
  }
  segment->data = 0;
  return 1;
}

int space_mixer_start(struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  if(data->left == 0 || data->right == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

float attenuation_none(float min, float max, float dist, float roll){
  IGNORE(min, max, dist, roll);
  return 1.0;
}

float attenuation_inverse(float min, float max, float dist, float roll){
  IGNORE(max);
  return min / (min + roll * (dist-min));
}

float attenuation_linear(float min, float max, float dist, float roll){
  return 1.0 - roll * (dist - min) / (max - min);
}

float attenuation_exponential(float min, float max, float dist, float roll){
  IGNORE(max);
  return 1.0/pow(dist / min, roll);
}

static inline float dot(float a[3], float b[3]){
  return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

static inline float mag(float a[3]){
  return sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

static inline float dist(float a[3], float b[3]){
  float r[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  return mag(r);
}

static inline float *norm(float a[3]){
  float Mag = mag(a);
  if(Mag != 0.0){
    a[0] /= Mag; a[1] /= Mag; a[2] /= Mag;
  }
  return a;
}

static inline float *cross(float a[3], float b[3], float r[3]){
  r[0] = (a[1] * b[2]) - (a[2] * b[1]);
  r[1] = (a[2] * b[1]) - (a[0] * b[2]);
  r[2] = (a[0] * b[1]) - (a[1] * b[0]);
  return r;
}

static inline float min(float a, float b){
  return (a < b)? a : b;
}

static inline float clamp(float l, float v, float r){
  return (v < l)? l : ((v < r)? v : r);
}

static inline float calculate_pan(float S[3], float L[3], float D[3], float U[3]){
  float t1[3], t2[3] = {L[0] - S[0], L[1] - S[1], L[2] - S[2]};
  return dot(norm(cross(U, D, t1)), norm(t2));
}

static inline float calculate_phase(float S[3], float L[3], float D[3]){
  float t2[3] = {S[0] - L[0], S[1] - L[1], S[2] - L[2]};
  return dot(norm(D), norm(t2));
}

VECTORIZE static inline void calculate_volumes(float *lvolume, float *rvolume, struct space_source *source, struct space_mixer_data *data){
  float min = source->min_distance;
  float max = source->max_distance;
  float roll = source->rolloff;
  float div = data->volume;
  float distance = clamp(min, dist(source->location, data->location), max);
  float volume = div * data->attenuation(min, max, distance, roll);
  float pan = (distance <= min)
    ? 0.0
    : calculate_pan(source->location, data->location, data->direction, data->up);
  *lvolume = volume * ((0.0<pan)?(1.0f-pan):1.0f);
  *rvolume = volume * ((pan<0.0)?(1.0f+pan):1.0f);
  if(calculate_phase(source->location, data->location, data->direction) < 0){
    *rvolume *= -1.0;
  }
}

VECTORIZE static inline float calculate_pitch_shift(struct space_mixer_data *listener, struct space_source *source){
  if(listener->doppler_factor <= 0.0) return 1.0;
  // See OpenAL1.1 specification §3.5.2
  float SL[3] = {listener->location[0] - source->location[0],
                 listener->location[1] - source->location[1],
                 listener->location[2] - source->location[2]};
  float *SV = source->velocity;
  float *LV = listener->velocity;
  float SS = listener->soundspeed;
  float DF = listener->doppler_factor;
  float Mag = mag(SL);
  float vls = dot(SL, LV) * Mag;
  float vss = dot(SL, SV) * Mag;
  float SS_DF = SS/DF;
  vss = min(vss, SS_DF);
  vls = min(vls, SS_DF);
  return (SS - DF*vls) / (SS - DF*vss);
}

VECTORIZE int space_mixer_mix(struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  float *restrict left, *restrict right, *restrict in;
  uint32_t samples = UINT32_MAX;
  
  // Compute sample counts
  mixed_buffer_request_write(&left, &samples, data->left);
  mixed_buffer_request_write(&right, &samples, data->right);
  for(uint32_t s=0; s<data->count; ++s){
    struct space_source *source = data->sources[s];
    if(!source) continue;

    mixed_buffer_request_read(&in, &samples, source->buffer);
    if(samples == 0) break;
  }

  if(0 < samples){
    memset(left, 0, samples*sizeof(float));
    memset(right, 0, samples*sizeof(float));
    for(uint32_t s=0; s<data->count; ++s){
      struct space_source *source = data->sources[s];
      if(!source) continue;
      
      float lvolume, rvolume;
      mixed_buffer_request_read(&in, &samples, source->buffer);
      calculate_volumes(&lvolume, &rvolume, source, data);
      float pitch = clamp(0.5, calculate_pitch_shift(data, source), 2.0);
      if(pitch != 1.0)
        fft_window(in, in, samples, &data->fft_window_data, fft_pitch_shift, &pitch);
      for(uint32_t i=0; i<samples; ++i){
        float sample = in[i];
        left[i] += sample * lvolume;
        right[i] += sample * rvolume;
      }
      mixed_buffer_finish_read(samples, source->buffer);
    }
  }
  mixed_buffer_finish_write(samples, data->left);
  mixed_buffer_finish_write(samples, data->right);
  return 1;
}

int space_mixer_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: data->left = (struct mixed_buffer *)buffer; return 1;
    case MIXED_RIGHT: data->right = (struct mixed_buffer *)buffer; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int space_mixer_get_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    switch(location){
    case MIXED_LEFT: *(struct mixed_buffer **)buffer = data->left; return 1;
    case MIXED_RIGHT: *(struct mixed_buffer **)buffer = data->right; return 1;
    default: mixed_err(MIXED_INVALID_LOCATION); return 0;
    }
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int space_mixer_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(buffer){ // Add or set an element
      struct space_source *source = 0;
      if(location < data->count)
        source = data->sources[location];
      if(!source){
        source = mixed_calloc(1, sizeof(struct space_source));
        if(!source){
          mixed_err(MIXED_OUT_OF_MEMORY);
          return 0;
        }
        source->min_distance = data->min_distance;
        source->max_distance = data->max_distance;
        source->rolloff = data->rolloff;
        source->velocity[0] = data->velocity[0];
        source->velocity[1] = data->velocity[1];
        source->velocity[2] = data->velocity[2];
        source->location[0] = data->location[0];
        source->location[1] = data->location[1];
        source->location[2] = data->location[2];
      }
      source->buffer = (struct mixed_buffer *)buffer;
      if(location < data->count) data->sources[location] = source;
      else return vector_add_pos(location, source, (struct vector *)data);
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_INVALID_LOCATION);
        return 0;
      }
      mixed_free(data->sources[location]);
      data->sources[location] = 0;
    }
    return 1;
  case MIXED_SPACE_MIN_DISTANCE:
  case MIXED_SPACE_MAX_DISTANCE:
  case MIXED_SPACE_ROLLOFF:
  case MIXED_SPACE_LOCATION:
  case MIXED_SPACE_VELOCITY:
    if(data->count <= location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    struct space_source *source = data->sources[location];
    float *value = (float *)buffer;
    switch(field){
    case MIXED_SPACE_MIN_DISTANCE:
      source->min_distance = *(float *)buffer;
      break;
    case MIXED_SPACE_MAX_DISTANCE:
      source->max_distance = *(float *)buffer;
      break;
    case MIXED_SPACE_ROLLOFF:
      source->rolloff = *(float *)buffer;
      break;
    case MIXED_SPACE_LOCATION:
      source->location[0] = value[0];
      source->location[1] = value[1];
      source->location[2] = value[2];
      break;
    case MIXED_SPACE_VELOCITY:
      source->velocity[0] = value[0];
      source->velocity[1] = value[1];
      source->velocity[2] = value[2];
      break;
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int space_mixer_get_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  
  if(data->count <= location){
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  }
  
  struct space_source *source = data->sources[location];
  if(source == 0){
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  }

  switch(field){
  case MIXED_BUFFER:
    *(struct mixed_buffer **)buffer = source->buffer;
    return 1;
  case MIXED_SPACE_MIN_DISTANCE:
    *(float *)buffer = source->min_distance;
    return 1;
  case MIXED_SPACE_MAX_DISTANCE:
    *(float *)buffer = source->max_distance;
    return 1;
  case MIXED_SPACE_ROLLOFF:
    *(float *)buffer = source->rolloff;
    return 1;
  case MIXED_SPACE_LOCATION:
  case MIXED_SPACE_VELOCITY:{
    float *value = (float *)buffer;
    switch(field){
    case MIXED_SPACE_LOCATION:
      value[0] = source->location[0];
      value[1] = source->location[1];
      value[2] = source->location[2];
      break;
    case MIXED_SPACE_VELOCITY:
      value[0] = source->velocity[0];
      value[1] = source->velocity[1];
      value[2] = source->velocity[2];
      break;
    }}
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int space_mixer_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  float *parts = (float *)value;
  switch(field){
  case MIXED_VOLUME:
    *((float *)value) = data->volume;
    break;
  case MIXED_SPACE_LOCATION:
    parts[0] = data->location[0];
    parts[1] = data->location[1];
    parts[2] = data->location[2];
    break;
  case MIXED_SPACE_VELOCITY:
    parts[0] = data->velocity[0];
    parts[1] = data->velocity[1];
    parts[2] = data->velocity[2];
    break;
  case MIXED_SPACE_DIRECTION:
    parts[0] = data->direction[0];
    parts[1] = data->direction[1];
    parts[2] = data->direction[2];
    break;
  case MIXED_SPACE_UP:
    parts[0] = data->up[0];
    parts[1] = data->up[1];
    parts[2] = data->up[2];
    break;
  case MIXED_SPACE_SOUNDSPEED:
    *(float *)value = data->soundspeed;
    break;
  case MIXED_SPACE_DOPPLER_FACTOR:
    *(float *)value = data->doppler_factor;
    break;
  case MIXED_SPACE_MIN_DISTANCE:
    *(float *)value = data->min_distance;
    break;
  case MIXED_SPACE_MAX_DISTANCE:
    *(float *)value = data->max_distance;
    break;
  case MIXED_SPACE_ROLLOFF:
    *(float *)value = data->rolloff;
    break;
  case MIXED_SPACE_ATTENUATION:
    if(data->attenuation == attenuation_none){
      *(int *)value = MIXED_NO_ATTENUATION;
    }else if(data->attenuation == attenuation_inverse){
      *(int *)value = MIXED_INVERSE_ATTENUATION;
    }else if(data->attenuation == attenuation_linear){
      *(int *)value = MIXED_LINEAR_ATTENUATION;
    }else if(data->attenuation == attenuation_exponential){
      *(int *)value = MIXED_EXPONENTIAL_ATTENUATION;
    }else{
      *(float (**)(float min, float max, float dist, float roll))value = data->attenuation;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

int space_mixer_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct space_mixer_data *data = (struct space_mixer_data *)segment->data;
  float *parts = (float *)value;
  switch(field){
  case MIXED_VOLUME:
    data->volume = *((float *)value);
    break;
  case MIXED_SPACE_LOCATION:
    data->location[0] = parts[0];
    data->location[1] = parts[1];
    data->location[2] = parts[2];
    break;
  case MIXED_SPACE_VELOCITY:
    data->velocity[0] = parts[0];
    data->velocity[1] = parts[1];
    data->velocity[2] = parts[2];
    break;
  case MIXED_SPACE_DIRECTION:
    data->direction[0] = parts[0];
    data->direction[1] = parts[1];
    data->direction[2] = parts[2];
    break;
  case MIXED_SPACE_UP:
    data->up[0] = parts[0];
    data->up[1] = parts[1];
    data->up[2] = parts[2];
    break;
  case MIXED_SPACE_SOUNDSPEED:
    data->soundspeed = *(float *)value;
    break;
  case MIXED_SPACE_DOPPLER_FACTOR:
    data->doppler_factor = *(float *)value;
    break;
  case MIXED_SPACE_MIN_DISTANCE:
    data->min_distance = *(float *)value;
    break;
  case MIXED_SPACE_MAX_DISTANCE:
    data->max_distance = *(float *)value;
    break;
  case MIXED_SPACE_ROLLOFF:
    data->rolloff = *(float *)value;
    break;
  case MIXED_SPACE_ATTENUATION:
    switch(*(uint32_t *)value){
    case MIXED_NO_ATTENUATION:
      data->attenuation = attenuation_none;
      break;
    case MIXED_INVERSE_ATTENUATION:
      data->attenuation = attenuation_inverse;
      break;
    case MIXED_LINEAR_ATTENUATION:
      data->attenuation = attenuation_linear;
      break;
    case MIXED_EXPONENTIAL_ATTENUATION:
      data->attenuation = attenuation_exponential;
      break;
    default:
      data->attenuation = (float (*)(float min, float max, float dist, float roll))value;
      break;
    }
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

int space_mixer_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  IGNORE(segment);
  
  info->name = "space_mixer";
  info->description = "Mixes multiple sources while simulating 3D space.";
  info->flags = MIXED_MODIFIES_INPUT;
  info->min_inputs = 0;
  info->max_inputs = -1;
  info->outputs = 2;
    
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_VOLUME,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The volume scaling factor for the output.");

  set_info_field(field++, MIXED_SPACE_LOCATION,
                 MIXED_FLOAT, 3, MIXED_IN | MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The location of the source or segment (listener) in space.");

  set_info_field(field++, MIXED_SPACE_VELOCITY,
                 MIXED_FLOAT, 3, MIXED_IN | MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The velocity of the source or segment (listener) in space.");

  set_info_field(field++, MIXED_SPACE_DIRECTION,
                 MIXED_FLOAT, 3, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The direction the segment is facing in space.");

  set_info_field(field++, MIXED_SPACE_UP,
                 MIXED_FLOAT, 3, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The vector designating 'upwards' in space.");

  set_info_field(field++, MIXED_SPACE_SOUNDSPEED,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The speed of sound. This also implicitly declares the unit size.");

  set_info_field(field++, MIXED_SPACE_DOPPLER_FACTOR,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The doppler factor. You can use this to exaggerate or dampen the effect.");

  set_info_field(field++, MIXED_SPACE_MIN_DISTANCE,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Any distance lower than this will make the sound appear at its maximal volume.");

  set_info_field(field++, MIXED_SPACE_MAX_DISTANCE,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "Any distance greater than this will make the sound appear at its minimal volume.");

  set_info_field(field++, MIXED_SPACE_ROLLOFF,
                 MIXED_FLOAT, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "This factor influences the curve of the attenuation function.");

  set_info_field(field++, MIXED_SPACE_ATTENUATION,
                 MIXED_FUNCTION, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The function that calculates the attenuation curve that defines the volume of a source by its distance.");

  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_space_mixer(uint32_t samplerate, struct mixed_segment *segment){
  struct space_mixer_data *data = mixed_calloc(1, sizeof(struct space_mixer_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  // These factors might need tweaking for efficiency/quality.
  if(!make_fft_window_data(2048, 4, samplerate, &data->fft_window_data)){
    mixed_free(data);
    return 0;
  }

  data->direction[2] = 1.0;      // Facing in Z+ direction
  data->up[1] = 1.0;             // OpenGL-like. Y+ is up.
  data->soundspeed = 34330.0;    // Means units are in [cm].
  data->doppler_factor = 0.0;
  data->min_distance = 10.0;      // That's 10 centimetres.
  data->max_distance = 100000.0;  // That's a kilometre.
  data->rolloff = 0.5;
  data->attenuation = attenuation_exponential;
  data->volume = 1.0;
  
  segment->free = space_mixer_free;
  segment->info = space_mixer_info;
  segment->start = space_mixer_start;
  segment->mix = space_mixer_mix;
  segment->set_in = space_mixer_set_in;
  segment->get_in = space_mixer_get_in;
  segment->set_out = space_mixer_set_out;
  segment->get_out = space_mixer_get_out;
  segment->set = space_mixer_set;
  segment->get = space_mixer_get;
  segment->data = data;
  return 1;
}

int __make_space_mixer(void *args, struct mixed_segment *segment){
  return mixed_make_segment_space_mixer(ARG(uint32_t, 0), segment);
}

REGISTER_SEGMENT(space_mixer, __make_space_mixer, 1, {
      {.description = "samplerate", .type = MIXED_UINT32}})
