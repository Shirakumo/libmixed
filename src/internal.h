#include <dlfcn.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "mixed.h"
#define BASE_VECTOR_SIZE 32

struct vector{
  void **data;
  size_t count;
  size_t size;
};

void free_vector(struct vector *vector);
int vector_add(void *element, struct vector *vector);
int vector_remove_pos(size_t i, struct vector *vector);
int vector_remove_item(void *element, struct vector *vector);

struct pitch_data{
  float *in_fifo;
  float *out_fifo;
  float *fft_workspace;
  float *last_phase;
  float *phase_sum;
  float *output_accumulator;
  float *analyzed_frequency;
  float *analyzed_magnitude;
  float *synthesized_frequency;
  float *synthesized_magnitude;
  size_t framesize;
  size_t oversampling;
  size_t overlap;
  size_t samplerate;
};

void free_pitch_data(struct pitch_data *data);
int make_pitch_data(size_t framesize, size_t oversampling, size_t samplerate, struct pitch_data *data);
void pitch_shift(float pitch, float *in, float *out, size_t samples, struct pitch_data *data);

void mixed_err(int errorcode);

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size);
