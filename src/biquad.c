#include "internal.h"
#include <math.h>

// Adapted from https://github.com/velipso/sndfilter/blob/master/src/biquad.c

extern inline float biquad_sample(float sample, struct biquad_data *state);

VECTORIZE void biquad_process(struct mixed_buffer *input, struct mixed_buffer *output, struct biquad_data *state){
  float b0 = state->b[0];
  float b1 = state->b[1];
  float b2 = state->b[2];
  float a1 = state->a[0];
  float a2 = state->a[1];
  float xn1 = state->x[0];
  float xn2 = state->x[1];
  float yn1 = state->y[0];
  float yn2 = state->y[1];

  with_mixed_buffer_transfer(i, samples, inv, input, outv, output, {
      float xn0 = inv[i];
      float L =
        b0 * xn0 +
        b1 * xn1 +
        b2 * xn2 -
        a1 * yn1 -
        a2 * yn2;

      xn2 = xn1;
      xn1 = xn0;
      yn2 = yn1;
      yn1 = L;
      outv[i] = L;
    });

  state->x[0] = xn1;
  state->x[1] = xn2;
  state->y[0] = yn1;
  state->y[1] = yn2;
}

extern inline void biquad_reset(struct biquad_data *state);

static inline void state_scale(struct biquad_data *state, float amt){
  state->b[0] = amt;
  state->b[1] = 0.0f;
  state->b[2] = 0.0f;
  state->a[0] = 0.0f;
  state->a[1] = 0.0f;
}

static inline void state_passthrough(struct biquad_data *state){
  state_scale(state, 1.0f);
}

static inline void state_zero(struct biquad_data *state){
  state_scale(state, 0.0f);
}

void biquad_lowpass(uint32_t rate, float cutoff, float resonance, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  cutoff /= nyquist;

  if (cutoff >= 1.0f)
    state_passthrough(state);
  else if (cutoff <= 0.0f)
    state_zero(state);
  else{
    resonance = powf(10.0f, resonance * 0.05f); // convert resonance from dB to linear
    float theta = (float)M_PI * 2.0f * cutoff;
    float alpha = sinf(theta) / (2.0f * resonance);
    float cosw  = cosf(theta);
    float beta  = (1.0f - cosw) * 0.5f;
    float a0inv = 1.0f / (1.0f + alpha);
    state->b[0] = a0inv * beta;
    state->b[1] = a0inv * 2.0f * beta;
    state->b[2] = a0inv * beta;
    state->a[0] = a0inv * -2.0f * cosw;
    state->a[1] = a0inv * (1.0f - alpha);
  }
}

void biquad_highpass(uint32_t rate, float cutoff, float resonance, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  cutoff /= nyquist;

  if (cutoff >= 1.0f)
    state_zero(state);
  else if (cutoff <= 0.0f)
    state_passthrough(state);
  else{
    resonance = powf(10.0f, resonance * 0.05f); // convert resonance from dB to linear
    float theta = (float)M_PI * 2.0f * cutoff;
    float alpha = sinf(theta) / (2.0f * resonance);
    float cosw  = cosf(theta);
    float beta  = (1.0f + cosw) * 0.5f;
    float a0inv = 1.0f / (1.0f + alpha);
    state->b[0] = a0inv * beta;
    state->b[1] = a0inv * -2.0f * beta;
    state->b[2] = a0inv * beta;
    state->a[0] = a0inv * -2.0f * cosw;
    state->a[1] = a0inv * (1.0f - alpha);
  }
}

void biquad_bandpass(uint32_t rate, float freq, float Q, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq <= 0.0f || freq >= 1.0f)
    state_zero(state);
  else if (Q <= 0.0f)
    state_passthrough(state);
  else{
    float w0    = (float)M_PI * 2.0f * freq;
    float alpha = sinf(w0) / (2.0f * Q);
    float k     = cosf(w0);
    float a0inv = 1.0f / (1.0f + alpha);
    state->b[0] = a0inv * alpha;
    state->b[1] = 0;
    state->b[2] = a0inv * -alpha;
    state->a[0] = a0inv * -2.0f * k;
    state->a[1] = a0inv * (1.0f - alpha);
  }
}

void biquad_notch(uint32_t rate, float freq, float Q, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq <= 0.0f || freq >= 1.0f)
    state_passthrough(state);
  else if (Q <= 0.0f)
    state_zero(state);
  else{
    float w0    = (float)M_PI * 2.0f * freq;
    float alpha = sinf(w0) / (2.0f * Q);
    float k     = cosf(w0);
    float a0inv = 1.0f / (1.0f + alpha);
    state->b[0] = a0inv;
    state->b[1] = a0inv * -2.0f * k;
    state->b[2] = a0inv;
    state->a[0] = a0inv * -2.0f * k;
    state->a[1] = a0inv * (1.0f - alpha);
  }
}

void biquad_peaking(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq <= 0.0f || freq >= 1.0f){
    state_passthrough(state);
    return;
  }

  float A = powf(10.0f, gain * 0.025f); // square root of gain converted from dB to linear

  if (Q <= 0.0f){
    state_scale(state, A * A); // scale by A squared
    return;
  }

  float w0    = (float)M_PI * 2.0f * freq;
  float alpha = sinf(w0) / (2.0f * Q);
  float k     = cosf(w0);
  float a0inv = 1.0f / (1.0f + alpha / A);
  state->b[0] = a0inv * (1.0f + alpha * A);
  state->b[1] = a0inv * -2.0f * k;
  state->b[2] = a0inv * (1.0f - alpha * A);
  state->a[0] = a0inv * -2.0f * k;
  state->a[1] = a0inv * (1.0f - alpha / A);
}

void biquad_allpass(uint32_t rate, float freq, float Q, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq <= 0.0f || freq >= 1.0f)
    state_passthrough(state);
  else if (Q <= 0.0f)
    state_scale(state, -1.0f); // invert the sample
  else{
    float w0    = (float)M_PI * 2.0f * freq;
    float alpha = sinf(w0) / (2.0f * Q);
    float k     = cosf(w0);
    float a0inv = 1.0f / (1.0f + alpha);
    state->b[0] = a0inv * (1.0f - alpha);
    state->b[1] = a0inv * -2.0f * k;
    state->b[2] = a0inv * (1.0f + alpha);
    state->a[0] = a0inv * -2.0f * k;
    state->a[1] = a0inv * (1.0f - alpha);
  }
}

// WebAudio hardcodes Q=1
void biquad_lowshelf(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq <= 0.0f || Q == 0.0f){
    state_passthrough(state);
    return;
  }

  float A = powf(10.0f, gain * 0.025f); // square root of gain converted from dB to linear

  if (freq >= 1.0f){
    state_scale(state, A * A); // scale by A squared
    return;
  }

  float w0    = (float)M_PI * 2.0f * freq;
  float ainn  = (A + 1.0f / A) * (1.0f / Q - 1.0f) + 2.0f;
  if (ainn < 0)
    ainn = 0;
  float alpha = 0.5f * sinf(w0) * sqrtf(ainn);
  float k     = cosf(w0);
  float k2    = 2.0f * sqrtf(A) * alpha;
  float Ap1   = A + 1.0f;
  float Am1   = A - 1.0f;
  float a0inv = 1.0f / (Ap1 + Am1 * k + k2);
  state->b[0] = a0inv * A * (Ap1 - Am1 * k + k2);
  state->b[1] = a0inv * 2.0f * A * (Am1 - Ap1 * k);
  state->b[2] = a0inv * A * (Ap1 - Am1 * k - k2);
  state->a[0] = a0inv * -2.0f * (Am1 + Ap1 * k);
  state->a[1] = a0inv * (Ap1 + Am1 * k - k2);
}

// WebAudio hardcodes Q=1
void biquad_highshelf(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state){
  biquad_reset(state);
  float nyquist = rate * 0.5f;
  freq /= nyquist;

  if (freq >= 1.0f || Q == 0.0f){
    state_passthrough(state);
    return;
  }

  float A = powf(10.0f, gain * 0.025f); // square root of gain converted from dB to linear

  if (freq <= 0.0f){
    state_scale(state, A * A); // scale by A squared
    return;
  }

  float w0    = (float)M_PI * 2.0f * freq;
  float ainn  = (A + 1.0f / A) * (1.0f / Q - 1.0f) + 2.0f;
  if (ainn < 0)
    ainn = 0;
  float alpha = 0.5f * sinf(w0) * sqrtf(ainn);
  float k     = cosf(w0);
  float k2    = 2.0f * sqrtf(A) * alpha;
  float Ap1   = A + 1.0f;
  float Am1   = A - 1.0f;
  float a0inv = 1.0f / (Ap1 - Am1 * k + k2);
  state->b[0] = a0inv * A * (Ap1 + Am1 * k + k2);
  state->b[1] = a0inv * -2.0f * A * (Am1 + Ap1 * k);
  state->b[2] = a0inv * A * (Ap1 + Am1 * k - k2);
  state->a[0] = a0inv * 2.0f * (Am1 - Ap1 * k);
  state->a[1] = a0inv * (Ap1 - Am1 * k - k2);
}
