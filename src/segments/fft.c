#include "../internal.h"
#include "spiral_fft.h"

struct fft_segment_data{
  struct mixed_buffer *in;
  struct mixed_buffer *out;
  uint32_t samplerate;
  uint32_t framesize;
  uint32_t oversampling;
  long overlap;
  float *fifo;
  float *fft_workspace;
  float *phase;
  float *accumulator;
};

int fft_realloc(struct fft_segment_data *data){
  uint32_t framesize = data->framesize;
  float *mem = data->fifo;
  uint32_t size = framesize+framesize*2+framesize/2+1+framesize*2;
  if(mem) mem = mixed_realloc(mem, size*sizeof(float));
  else mem = mixed_calloc(size, sizeof(float));
  if(!mem){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  data->fifo = mem; mem += framesize;
  data->fft_workspace = mem; mem += framesize*2;
  data->phase = mem; mem += framesize/2+1;
  data->accumulator = mem; mem += framesize*2;
  return 1;
}

int fft_segment_free(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  if(segment->data){
    FREE(data->fifo);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int fft_segment_start(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  if(data->out == 0 || data->in == 0){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }
  return 1;
}

int fft_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

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

int fft_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;

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

int fft_segment_fwd(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  float *in_fifo = data->fifo;
  float *fft_workspace = data->fft_workspace;
  float *last_phase = data->phase;

  float *in, *out;
  uint32_t in_samples = UINT32_MAX;
  mixed_buffer_request_read(&in, &in_samples, data->in);

  double magnitude, phase, tmp, window, real, imag;
  long i, k, qpd;
  long framesize2 = framesize/2;
  long step = framesize/oversampling;
  double bin_frequencies = (double)data->samplerate/(double)framesize;
  double expected = 2.*M_PI*(double)step/(double)framesize;
  long fifo_latency = framesize-step;
  if (data->overlap == 0) data->overlap = fifo_latency;

  for(i=0; i<in_samples; i++){
    in_fifo[data->overlap] = in[i];
    data->overlap++;
    if(data->overlap >= framesize){
      uint32_t out_samples = framesize;
      mixed_buffer_request_write(&out, &out_samples, data->out);
      if(out_samples < framesize){
        mixed_buffer_finish_write(0, data->out);
        break;
      }
      
      data->overlap = fifo_latency;

      for(k=0; k<framesize; k++){
        window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        fft_workspace[2*k] = in_fifo[k] * window;
        fft_workspace[2*k+1] = 0.;
      }

      spiral_fft_float(framesize, -1, fft_workspace, fft_workspace);

      for(k=0; k<=framesize2; k++){
        real = fft_workspace[2*k];
        imag = fft_workspace[2*k+1];
        magnitude = 2.*sqrt(real*real + imag*imag);
        phase = atan2(imag,real);
        
        tmp = phase - last_phase[k];
        last_phase[k] = phase;
        tmp -= (double)k*expected;

        qpd = tmp/M_PI;
        if (qpd >= 0) qpd += qpd&1;
        else qpd -= qpd&1;
        tmp -= M_PI*(double)qpd;

        tmp = oversampling*tmp/(2.*M_PI);
        tmp = (double)k*bin_frequencies + tmp*bin_frequencies;

        out[k*2] = tmp;
        out[k*2+1] = magnitude;
      }
      mixed_buffer_finish_write(out_samples, data->out);
      
      for (k = 0; k < fifo_latency; k++) in_fifo[k] = in_fifo[k+step];
    }
  }
  
  mixed_buffer_finish_read(in_samples, data->in);
  return 1;
}

int fft_segment_inv(struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  float *out_fifo = data->fifo;
  float *fft_workspace = data->fft_workspace;
  float *phase_sum = data->phase;
  float *output_accumulator = data->accumulator;

  float *in, *out;
  uint32_t in_samples = framesize;
  uint32_t out_samples = UINT32_MAX;

  double magnitude, phase, tmp, window;
  long i, k;
  long framesize2 = framesize/2;
  long step = framesize/oversampling;
  double bin_frequencies = (double)data->samplerate/(double)framesize;
  double expected = 2.*M_PI*(double)step/(double)framesize;
  long fifo_latency = framesize-step;
  if (data->overlap == 0) data->overlap = fifo_latency;

  mixed_buffer_request_write(&out, &out_samples, data->out);
  for(i=0; i<out_samples; i++){
    out[i] = out_fifo[data->overlap-fifo_latency];
    data->overlap++;

    if(data->overlap >= framesize){
      data->overlap = fifo_latency;
      
      if(!mixed_buffer_request_read(&in, &in_samples, data->in))
        break;
      mixed_buffer_finish_read(in_samples, data->in);
      for(k=0; k<=framesize2; k++){
        tmp = in[k*2];
        magnitude = in[k*2+1];

        tmp -= (double)k*bin_frequencies;
        tmp /= bin_frequencies;
        tmp = 2.*M_PI*tmp/oversampling;
        tmp += (double)k*expected;
        phase_sum[k] += tmp;
        phase = phase_sum[k];

        fft_workspace[2*k] = magnitude*cos(phase);
        fft_workspace[2*k+1] = magnitude*sin(phase);
      } 

      for(k=framesize+2; k<2*framesize; k++) fft_workspace[k] = 0.;

      spiral_fft_float(framesize, +1, fft_workspace, fft_workspace);

      for(k=0; k<framesize; k++){
        window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        output_accumulator[k] += 2.*window*fft_workspace[2*k]/(framesize2*oversampling);
      }
      for(k=0; k<step; k++) out_fifo[k] = output_accumulator[k];

      memmove(output_accumulator, output_accumulator+step, framesize*sizeof(float));
    }
  }
  mixed_buffer_finish_write(i, data->out);

  return 1;
}

int fft_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  info->name = (segment->mix == fft_segment_fwd)? "fwd_fft" : "inv_fft";
  info->description = "Perform a Fast Fourier Transform";
  info->flags = 0;
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = 1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");

  set_info_field(field++, MIXED_SAMPLERATE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The samplerate at which the segment operates.");

  set_info_field(field++, MIXED_FRAMESIZE,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The FFT moving window frame size.");

  set_info_field(field++, MIXED_OVERSAMPLING,
                 MIXED_UINT32, 1, MIXED_SEGMENT | MIXED_SET | MIXED_GET,
                 "The FFT oversampling rate.");
  
  clear_info_field(field++);
  return 1;
}

int fft_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE: *((uint32_t *)value) = data->samplerate; break;
  case MIXED_FRAMESIZE: *((uint32_t *)value) = data->framesize; break;
  case MIXED_OVERSAMPLING: *((uint32_t *)value) = data->oversampling; break;
  default: mixed_err(MIXED_INVALID_FIELD); return 0;
  }
  return 1;
}

int fft_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct fft_segment_data *data = (struct fft_segment_data *)segment->data;
  switch(field){
  case MIXED_SAMPLERATE:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->samplerate = *(uint32_t *)value;
    break;
  case MIXED_FRAMESIZE:
    if(*(uint32_t *)value <= 0 || 1<<13 < *(uint32_t *)value){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->framesize = *(uint32_t *)value;
    break;
  case MIXED_OVERSAMPLING:
    if(*(uint32_t *)value <= 0){
      mixed_err(MIXED_INVALID_VALUE);
      return 0;
    }
    data->oversampling = *(uint32_t *)value;
    break;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
  if(!fft_realloc(data)){
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_fwd_fft(uint32_t samplerate, struct mixed_segment *segment){
  struct fft_segment_data *data = mixed_calloc(1, sizeof(struct fft_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->samplerate = samplerate;
  data->framesize = 2048;
  data->oversampling = 4;

  if(!fft_realloc(data)){
    return 0;
  }
  
  segment->free = fft_segment_free;
  segment->start = fft_segment_start;
  segment->mix = fft_segment_fwd;
  segment->set_in = fft_segment_set_in;
  segment->set_out = fft_segment_set_out;
  segment->info = fft_segment_info;
  segment->get = fft_segment_get;
  segment->set = fft_segment_set;
  segment->data = data;
  return 1;
}

MIXED_EXPORT int mixed_make_segment_inv_fft(uint32_t samplerate, struct mixed_segment *segment){
  int ret = mixed_make_segment_fwd_fft(samplerate, segment);
  segment->mix = fft_segment_inv;
  return ret;
}

int __make_fwd_fft(void *args, struct mixed_segment *segment){
  return mixed_make_segment_fwd_fft(ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(fwd_fft, __make_fwd_fft, 1, {
    {.description = "samplerate", .type = MIXED_UINT32}})

int __make_inv_fft(void *args, struct mixed_segment *segment){
  return mixed_make_segment_inv_fft(ARG(uint32_t, 1), segment);
}

REGISTER_SEGMENT(inv_fft, __make_inv_fft, 1, {
    {.description = "samplerate", .type = MIXED_UINT32}})
