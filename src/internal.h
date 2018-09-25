#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "mixed.h"
#ifdef __RDRND__
#include <cpuid.h>
#include <immintrin.h>
#endif
#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( defined _MSC_VER || defined __ICL || defined __DMC__ || defined __BORLANDC__ )
#  define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
#  define thread_local __thread
# else
#  define thread_local
# endif
#endif
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
int vector_clear(struct vector *vector);

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
  long framesize;
  long oversampling;
  long overlap;
  long samplerate;
};

void free_pitch_data(struct pitch_data *data);
int make_pitch_data(size_t framesize, size_t oversampling, size_t samplerate, struct pitch_data *data);
void pitch_shift(float pitch, float *in, float *out, size_t samples, struct pitch_data *data);

int mix_noop(size_t samples, struct mixed_segment *segment);

void mixed_err(int errorcode);

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size);

void set_info_field(struct mixed_segment_field_info *info, size_t field, enum mixed_segment_field_type type, size_t count, enum mixed_segment_info_flags flags, char*description);
void clear_info_field(struct mixed_segment_field_info *info);

size_t smin(size_t a, size_t b);

extern float (*mixed_random)();
