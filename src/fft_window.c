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
  data->last_phase = 0;
  data->phase_sum = 0;
  data->output_accumulator = 0;
  data->analyzed_frequency = 0;
  data->analyzed_magnitude = 0;
  data->synthesized_frequency = 0;
  data->synthesized_magnitude = 0;
}

int make_fft_window_data(uint32_t framesize, uint32_t oversampling, uint32_t samplerate, struct fft_window_data *data){
  // FIXME: determine which of these can be static and which actually
  //        need to be retained for processing over contiguous buffers
  float *mem = mixed_calloc(framesize+
                            framesize+
                            framesize*2+
                            framesize/2+1+
                            framesize/2+1+
                            framesize*2+
                            framesize+
                            framesize+
                            framesize+
                            framesize, sizeof(float));

  if(!mem){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  data->in_fifo = mem; mem += framesize;
  data->out_fifo = mem; mem += framesize;
  data->fft_workspace = mem; mem += framesize*2;
  data->last_phase = mem; mem += framesize/2+1;
  data->phase_sum = mem; mem += framesize/2+1;
  data->output_accumulator = mem; mem += framesize*2;
  data->analyzed_frequency = mem; mem += framesize;
  data->analyzed_magnitude = mem; mem += framesize;
  data->synthesized_frequency = mem; mem += framesize;
  data->synthesized_magnitude = mem; mem += framesize;

  data->framesize = framesize;
  data->oversampling = oversampling;
  data->samplerate = samplerate;
  return 1;
}

VECTORIZE void fft_window(float *in, float *out, uint32_t samples, struct fft_window_data *data, fft_window_process process, void *user){
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  float *in_fifo = data->in_fifo;
  float *out_fifo = data->out_fifo;
  float *fft_workspace = data->fft_workspace;
  float *output_accumulator = data->output_accumulator;
  double window;
  long i, k;

  long framesize2 = framesize/2;
  long step = framesize/oversampling;
  long fifo_latency = framesize-step;
  if (data->overlap == 0) data->overlap = fifo_latency;

  for (i = 0; i < samples; i++){
    in_fifo[data->overlap] = in[i];
    out[i] = out_fifo[data->overlap-fifo_latency];
    data->overlap++;
    if (data->overlap >= framesize) {
      data->overlap = fifo_latency;

      /* do windowing and re,im interleave */
      for (k = 0; k < framesize;k++) {
        window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        fft_workspace[2*k] = in_fifo[k] * window;
        fft_workspace[2*k+1] = 0.;
      }

      spiral_fft_float(framesize, -1, fft_workspace, fft_workspace);
      process(data, user);
      spiral_fft_float(framesize, +1, fft_workspace, fft_workspace);

      /* do windowing and add to output accumulator */
      for(k=0; k < framesize; k++) {
        window = -.5*cos(2.*M_PI*(double)k/(double)framesize)+.5;
        output_accumulator[k] += 2.*window*fft_workspace[2*k]/(framesize2*oversampling);
      }
      for (k = 0; k < step; k++) out_fifo[k] = output_accumulator[k];

      memmove(output_accumulator, output_accumulator+step, framesize*sizeof(float));

      for (k = 0; k < fifo_latency; k++) in_fifo[k] = in_fifo[k+step];
    }
  }
}

VECTORIZE void fft_pitch_shift(struct fft_window_data *data, void *user){
  uint32_t framesize = data->framesize;
  uint32_t oversampling = data->oversampling;
  uint32_t framesize2 = framesize/2;
  long step = framesize/oversampling;
  double bin_frequencies = (double)data->samplerate/(double)framesize;
  double expected = 2.*M_PI*(double)step/(double)framesize;
  float *fft_workspace = data->fft_workspace;
  float *last_phase = data->last_phase;
  float *phase_sum = data->phase_sum;
  float *analyzed_frequency = data->analyzed_frequency;
  float *analyzed_magnitude = data->analyzed_magnitude;
  float *synthesized_frequency = data->synthesized_frequency;
  float *synthesized_magnitude = data->synthesized_magnitude;

  float pitch_shift = *((float*)user);

  for(long k = 0; k <= framesize2; k++){
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
  for(long k = 0; k <= framesize2; k++){
    long index = k*pitch_shift;
    if(index <= framesize2){
      synthesized_magnitude[index] += analyzed_magnitude[k];
      synthesized_frequency[index] = analyzed_frequency[k] * pitch_shift;
    }
  }

  for(long k = 0; k <= framesize2; k++){
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

  for(long k = framesize+2; k < 2*framesize; k++)
    fft_workspace[k] = 0.;
}

VECTORIZE void fft_convolve(struct fft_window_data *data, void *user){
  uint32_t framesize = data->framesize;
  float *fft_workspace = data->fft_workspace;
  float *fir = (float *)user;
  
  for(long k = 0; k < framesize; k+= 2){
    float reA = fft_workspace[k+0];
    float imA = fft_workspace[k+1];
    float reB = fir[k+0];
    float imB = fir[k+1];

    float re = reA * reB - imA * imB;
    float im = reA * imB + imA * reB;

    fft_workspace[k+0] += re;
    fft_workspace[k+1] += im;
  }
}
