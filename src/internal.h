#include <stdio.h>
#include "mixed.h"
#define BASE_VECTOR_SIZE 32

struct vector{
  void **data;
  size_t count;
  size_t size;
};

int vector_add(void *element, struct vector *vector);
int vector_remove_pos(size_t i, struct vector *vector);
int vector_remove_item(void *element, struct vector *vector);

void free_pitch_data(void *data);
int make_pitch_data(size_t framesize, size_t oversampling, size_t samplerate, void *data);
void pitch_shift(float pitchShift, float *in, float *out, size_t samples, void *data);

void mixed_err(int errorcode);

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size);
