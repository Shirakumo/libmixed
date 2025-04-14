/////
// Adapted from http://blogs.zynaptiq.com/bernsee/repo/smbPitchShift.cpp
//

#include "internal.h"
#include "spiral_fft.h"

void free_fft_window_data(struct fft_window_data *data){
  if(data->in_fifo)
    mixed_free(data->in_fifo);
  data->in_fifo = 0;
  data->out_fifo = 0;
  data->fft_workspace = 0;
  data->output_accumulator = 0;
  data->last_phase = 0;
  data->phase_sum = 0;
}

int make_fft_window_data(uint32_t framesize, uint32_t oversampling, uint32_t samplerate, struct fft_window_data *data){
  float *mem = mixed_calloc(framesize+
                            framesize+
                            framesize*2+
                            framesize*2+
                            framesize/2+1+
                            framesize/2+1, sizeof(float));

  if(!mem){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->in_fifo = mem; mem += framesize;
  data->out_fifo = mem; mem += framesize;
  data->fft_workspace = mem; mem += framesize*2;
  data->output_accumulator = mem; mem += framesize*2;
  data->last_phase = mem; mem += framesize/2+1;
  data->phase_sum = mem; mem += framesize/2+1;

  data->framesize = framesize;
  data->oversampling = oversampling;
  data->samplerate = samplerate;
  return 1;
}

VECTORIZE void fft_window(float *restrict in, float *restrict out, uint32_t samples, struct fft_window_data *data, fft_window_process process, void *user){
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  float *restrict in_fifo = data->in_fifo;
  float *restrict out_fifo = data->out_fifo;
  float *restrict fft_workspace = data->fft_workspace;
  float *output_accumulator = data->output_accumulator;

  long framesize2 = framesize/2;
  long step = framesize/oversampling;
  long fifo_latency = framesize-step;
  if(data->overlap == 0)
    data->overlap = fifo_latency;

  for(uint32_t i = 0; i < samples; i++){
    in_fifo[data->overlap] = in[i];
    out[i] = out_fifo[data->overlap-fifo_latency];
    data->overlap++;
    if(data->overlap >= framesize){
      data->overlap = fifo_latency;

      /* do windowing and re,im interleave */
      for(uint32_t k = 0; k < framesize; k++) {
        double window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        fft_workspace[2*k] = in_fifo[k] * window;
        fft_workspace[2*k+1] = 0.;
      }

      spiral_fft_float(framesize, -1, fft_workspace, fft_workspace);
      process(data, user);
      spiral_fft_float(framesize, +1, fft_workspace, fft_workspace);

      /* do windowing and add to output accumulator */
      for(uint32_t k = 0; k < framesize; k++) {
        double window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        output_accumulator[k] += 2.*window*fft_workspace[2*k]/(framesize2*oversampling);
      }
      for(uint32_t k = 0; k < step; k++)
        out_fifo[k] = output_accumulator[k];

      memmove(output_accumulator, output_accumulator+step, framesize*sizeof(float));

      for(uint32_t k = 0; k < fifo_latency; k++)
        in_fifo[k] = in_fifo[k+step];
    }
  }
}

VECTORIZE void fft_pitch_shift(struct fft_window_data *data, void *user){
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  uint32_t framesize2 = framesize/2;
  float *restrict fft_workspace = data->fft_workspace;
  float *restrict last_phase = data->last_phase;
  float *restrict phase_sum = data->phase_sum;
  float analyzed_frequency[framesize];
  float analyzed_magnitude[framesize];
  float synthesized_frequency[framesize];
  float synthesized_magnitude[framesize];

  uint32_t step = framesize/oversampling;
  double bin_frequencies = (double)data->samplerate/(double)framesize;
  double expected = 2.*M_PI*(double)step/(double)framesize;
  float pitch_shift = *((float*)user);

  for(uint32_t k = 0; k <= framesize2; k++){
    float real = fft_workspace[2*k];
    float imag = fft_workspace[2*k+1];

    float magnitude = 2.*sqrt(real*real + imag*imag);
    float phase = atan2(imag,real);
    float tmp = phase - last_phase[k];
    last_phase[k] = phase;
    tmp -= (double)k*expected;
    long qpd = tmp/M_PI;
    if (qpd >= 0) qpd += qpd&1;
    else qpd -= qpd&1;
    tmp -= M_PI*(double)qpd;
    tmp = oversampling*tmp/(2.*M_PI);
    tmp = (double)k*bin_frequencies + tmp*bin_frequencies;

    analyzed_magnitude[k] = magnitude;
    analyzed_frequency[k] = tmp;
  }

  memset(synthesized_magnitude, 0, framesize*sizeof(float));
  memset(synthesized_frequency, 0, framesize*sizeof(float));
  for(uint32_t k = 0; k <= framesize2; k++){
    uint32_t index = k*pitch_shift;
    if(index <= framesize2){
      synthesized_magnitude[index] += analyzed_magnitude[k];
      synthesized_frequency[index] = analyzed_frequency[k] * pitch_shift;
    }
  }

  for(uint32_t k = 0; k <= framesize2; k++){
    float magnitude = synthesized_magnitude[k];
    float tmp = synthesized_frequency[k];

    tmp -= (double)k*bin_frequencies;
    tmp /= bin_frequencies;
    tmp = 2.*M_PI*tmp/oversampling;
    tmp += (double)k*expected;
    phase_sum[k] += tmp;

    float phase = phase_sum[k];
    fft_workspace[2*k] = magnitude*cos(phase);
    fft_workspace[2*k+1] = magnitude*sin(phase);
  }

  for(uint32_t k = framesize+2; k < 2*framesize; k++)
    fft_workspace[k] = 0.;
}
