#include "internal.h"

struct space_source{
  struct mixed_buffer *buffer;
  float location[3];
  float velocity[3];
};

struct space_source_vector{
  struct space_source **el;
  size_t count;
  size_t size;
};

struct space_output{
  struct mixed_buffer *buffer;
  float location[3];
};

struct space_output_vector{
  struct space_output **el;
  size_t count;
  size_t size;
};

struct space_segment_data{
  struct space_source_vector sources;
  struct space_output_vector outputs;
  struct pitch_data pitch_data;
  float location[3];
  float velocity[3];
  float direction[3];
  float up[3];
  float soundspeed;
  float doppler_factor;
  float min_distance;
  float max_distance;
  float rolloff;
  float (*attenuation)(float min, float max, float dist, float roll);
};

int space_segment_free(struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  if(data){
    free_vector((struct vector *)&data->sources);
    free_vector((struct vector *)&data->outputs);
    free(data);
  }
  segment->data = 0;
  return 1;
}

float attenuation_none(float min, float max, float dist, float roll){
  return 1.0;
}

float attenuation_inverse(float min, float max, float dist, float roll){
  return min / (min + roll * (dist-min));
}

float attenuation_linear(float min, float max, float dist, float roll){
  return 1.0 - roll * (dist - min) / (max - min);
}

float attenuation_exponential(float min, float max, float dist, float roll){
  return 1.0/pow(dist / min, roll);
}

extern inline float dot(float a[3], float b[3]){
  return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

extern inline float mag(float a[3]){
  return sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

extern inline float dist(float a[3], float b[3]){
  float r[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  return mag(r);
}

extern inline float min(float a, float b){
  return (a < b)? a : b;
}

extern inline float clamp(float l, float v, float r){
  return (v < l)? l : ((v < r)? v : r);
}

float calculate_pitch_shift(struct space_segment_data *listener, struct space_source *source){
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

void mix_output(struct space_output *output, size_t samples, struct space_segment_data *data){
  float *out = output->buffer->data;
  float *out_location = output->location;
  if(data->sources.count == 0){
    memset(out, 0, samples*sizeof(float));
  }else{
    float loc[3] = {data->location[0] + out_location[0],
                    data->location[1] + out_location[1],
                    data->location[2] + out_location[2]};
    float min = data->min_distance;
    float max = data->max_distance;
    float roll = data->rolloff;
    float div = 1.0/((float)data->sources.count);
    // Mix the first source directly to avoid a clearing loop
    struct space_source *source = data->sources.el[0];
    float *in = source->buffer->data;
    float distance = clamp(min, dist(loc, source->location), max);
    float attenuation = data->attenuation(min, max, distance, roll);
    // FIXME: determine damping due to "ear direction"
    for(size_t i=0; i<samples; ++i){
      out[i] = div * in[i] * attenuation;
    }
    // Mix the rest of the sources additively.
    for(size_t s=1; s<data->sources.count; ++s){
      source = data->sources.el[s];
      in = source->buffer->data;
      distance = dist(loc, source->location);
      attenuation = data->attenuation(min, max, distance, roll);
      for(size_t i=0; i<samples; ++i){
        out[i] += div * in[i] * attenuation;
      }
    }
  }
}

int space_segment_mix(size_t samples, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  // Shift frequencies
  // NOTE: If you wanted to be entirely correct, you'd have to calculate a
  //       different pitch for each channel and also include a rotational
  //       velocity for the listener, as that too would influence the pitch
  //       of the source for each ear. For simplicity and efficiency reasons
  //       we don't do this.
  for(size_t s=0; s<data->sources.count; ++s){
    struct space_source *source = data->sources.el[s];
    float pitch = calculate_pitch_shift(data, source);
    if(pitch != 1.0){
      struct mixed_buffer *buffer = source->buffer;
      // FIXME: I don't know if I can re-use the pitch_data buffer for every
      //        source like this, or if I need one per-source.
      pitch_shift(pitch, buffer->data, buffer->data, buffer->size, &data->pitch_data);
    }
  }
  // Mix sources
  for(size_t o=0; o<data->outputs.count; ++o){
    struct space_output *output = data->outputs.el[o];
    mix_output(output, samples, data);
  }
}

int space_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;

  if(buffer){ // Add or set an element
    if(location < data->outputs.count){
      data->outputs.el[location]->buffer = buffer;
    }else{
      struct space_output *output = calloc(1, sizeof(struct space_output));
      if(!output){
        mixed_err(MIXED_OUT_OF_MEMORY);
        return 0;
      }
      output->buffer = buffer;
      return vector_add(output, (struct vector *)&data->outputs);
    }
  }else{ // Remove an element
    if(data->outputs.count <= location){
      mixed_err(MIXED_INVALID_BUFFER_LOCATION);
      return 0;
    }
    free(data->outputs.el[location]);
    return vector_remove_pos(location, (struct vector *)&data->outputs);
  }
  return 1;
}

int space_segment_set_in(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;

  if(buffer){ // Add or set an element
    if(location < data->sources.count){
      data->sources.el[location]->buffer = buffer;
    }else{
      struct space_source *source = calloc(1, sizeof(struct space_source));
      if(!source){
        mixed_err(MIXED_OUT_OF_MEMORY);
        return 0;
      }
      source->buffer = buffer;
      return vector_add(source, (struct vector *)&data->sources);
    }
  }else{ // Remove an element
    if(data->sources.count <= location){
      mixed_err(MIXED_INVALID_BUFFER_LOCATION);
      return 0;
    }
    free(data->sources.el[location]);
    return vector_remove_pos(location, (struct vector *)&data->sources);
  }
  return 1;
}

int space_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  float **parts = (float **)value;
  switch(field){
  case MIXED_SPACE_LOCATION:
    *(parts[0]) = data->location[0];
    *(parts[1]) = data->location[1];
    *(parts[2]) = data->location[2];
    break;
  case MIXED_SPACE_VELOCITY:
    *(parts[0]) = data->velocity[0];
    *(parts[1]) = data->velocity[1];
    *(parts[2]) = data->velocity[2];
    break;
  case MIXED_SPACE_DIRECTION:
    *(parts[0]) = data->direction[0];
    *(parts[1]) = data->direction[1];
    *(parts[2]) = data->direction[2];
    break;
  case MIXED_SPACE_UP:
    *(parts[0]) = data->up[0];
    *(parts[1]) = data->up[1];
    *(parts[2]) = data->up[2];
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
    *(float (**)(float min, float max, float dist, float roll))value = data->attenuation;
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

int space_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  float *parts = *(float **)value;
  switch(field){
  case MIXED_SPACE_LOCATION:
    data->location[0] = parts[0];
    data->location[1] = parts[1];
    data->location[2] = parts[2];
    break;
  case MIXED_SPACE_VELOCITY:
    data->velocity[0] = parts[0];
    data->velocity[1] = parts[1];
    data->velocity[2] = parts[2];
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
    switch(*(size_t *)value){
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

int space_segment_set_source(size_t field, size_t location, void *value, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  float *parts = *(float **)value;
  
  if(data->sources.count <= location){
    mixed_err(MIXED_INVALID_BUFFER_LOCATION);
    return 0;
  }

  struct space_source *source = data->sources.el[location];
  
  switch(field){
  case MIXED_SPACE_LOCATION:
    source->location[0] = parts[0];
    source->location[1] = parts[1];
    source->location[2] = parts[2];
    break;
  case MIXED_SPACE_VELOCITY:
    source->velocity[0] = parts[0];
    source->velocity[1] = parts[1];
    source->velocity[2] = parts[2];
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

int space_segment_get_source(size_t field, size_t location, void *value, struct mixed_segment *segment){
  struct space_segment_data *data = (struct space_segment_data *)segment->data;
  float *parts = *(float **)value;
  
  if(data->sources.count <= location){
    mixed_err(MIXED_INVALID_BUFFER_LOCATION);
    return 0;
  }

  struct space_source *source = data->sources.el[location];
  
  switch(field){
  case MIXED_SPACE_LOCATION:
    source->location[0] = parts[0];
    source->location[1] = parts[1];
    source->location[2] = parts[2];
    break;
  case MIXED_SPACE_VELOCITY:
    source->velocity[0] = parts[0];
    source->velocity[1] = parts[1];
    source->velocity[2] = parts[2];
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  return 1;
}

struct mixed_segment_info space_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "space";
  info.description = "Mixes multiple sources while simulating 3D space.";
  info.flags = MIXED_MODIFIES_SOURCE;
  info.min_inputs = 0;
  info.max_inputs = -1;
  info.outputs = 2;
  return info;
}

int mixed_make_segment_space(size_t samplerate, struct mixed_segment *segment){
  struct space_segment_data *data = calloc(1, sizeof(struct space_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  // These factors might need tweaking for efficiency/quality.
  if(!make_pitch_data(2048, 16, samplerate, &data->pitch_data)){
    free(data);
    return 0;
  }

  data->direction[2] = 1.0;      // Facing in Z+ direction
  data->up[1] = 1.0;             // OpenGL-like. Y+ is up.
  data->soundspeed = 34330.0;    // Means units are in [cm].
  data->doppler_factor = 1.0;
  data->min_distance = 1.0;      // That's a centrimetre.
  data->max_distance = 10000.0;  // That's a 100 metres.
  data->rolloff = 0.5;
  data->attenuation = attenuation_exponential;
  
  segment->free = space_segment_free;
  segment->mix = space_segment_mix;
  segment->set_in = space_segment_set_in;
  segment->set_out = space_segment_set_out;
  segment->set = space_segment_set;
  segment->get = space_segment_get;
  segment->data = data;
  
  // Configure dummy stereo setup
  if(!space_segment_set_out(MIXED_LEFT, (struct mixed_buffer *)-1, segment) ||
     !space_segment_set_out(MIXED_RIGHT, (struct mixed_buffer *)-1, segment)){
    free(data);
    return 0;
  }
  // Head is usually ~15cm
  data->outputs.el[0]->location[1] = -7.5;
  data->outputs.el[1]->location[1] =  7.5;
  return 1;
}
