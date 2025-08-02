#pragma once
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
# if defined NN_NINTENDO_SDK
#  define thread_local
# elif __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
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
#define BASE_VECTOR_SIZE 128

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LERP(a, b, x) ((a)*(1-(x)) + (b)*(x))
#define CLAMP(l, v, h) ((v < l)? l : ((v < h)? v : h))

#define IGNORE(...) __ignore(0, __VA_ARGS__)
static inline void __ignore(char _, ...){(void)_;}

#ifdef MIXED_DEBUG
#define debug_log(...) {fprintf(stderr, "[libmixed] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}
#else
#define debug_log(...)
#endif

#if defined(__GNUC__) && !defined(__WIN32__)
#if defined(__x86_64__)
#define VECTORIZE __attribute__((target_clones("avx2","avx","sse4.1","default")))
#elif defined(__amd64__)
#define VECTORIZE __attribute__((target_clones("neon","default")))
#else
#define VECTORIZE
#endif
#else
#define VECTORIZE
#endif

struct bip{
  void *data;
  uint32_t size;
  uint32_t read;
  uint32_t write;
  uint32_t reserved;
};

struct vector{
  void **data;
  uint32_t count;
  uint32_t size;
};

void free_vector(struct vector *vector);
int vector_add(void *element, struct vector *vector);
int vector_add_pos(uint32_t i, void *element, struct vector *vector);
int vector_remove_count(uint32_t i, uint32_t n, struct vector *vector);
int vector_remove_pos(uint32_t i, struct vector *vector);
int vector_remove_item(void *element, struct vector *vector);
int vector_clear(struct vector *vector);

struct fft_window_data{
  float *in_fifo;
  float *out_fifo;
  float *fft_workspace;
  float *output_accumulator;
  // Extra stuff used for pitch
  float *last_phase;
  float *phase_sum;
  long framesize;
  long oversampling;
  long overlap;
  long samplerate;
};


typedef void (*fft_window_process)(struct fft_window_data *data, void *user);
void fft_pitch_shift(struct fft_window_data *data, void *user);
void fft_convolve(struct fft_window_data *data, void *user);

void free_fft_window_data(struct fft_window_data *data);
int make_fft_window_data(uint32_t framesize, uint32_t oversampling, uint32_t samplerate, struct fft_window_data *data);
void fft_window(float *in, float *out, uint32_t samples, struct fft_window_data *data, fft_window_process process, void *user);

float attenuation_none(float min, float max, float dist, float roll);
float attenuation_inverse(float min, float max, float dist, float roll);
float attenuation_linear(float min, float max, float dist, float roll);
float attenuation_exponential(float min, float max, float dist, float roll);

struct biquad_data{
  float x[2];
  float y[2];
  float a[2];
  float b[3];
};

void biquad_lowpass(uint32_t rate, float cutoff, float resonance, struct biquad_data *state);
void biquad_highpass(uint32_t rate, float cutoff, float resonance, struct biquad_data *state);
void biquad_bandpass(uint32_t rate, float freq, float Q, struct biquad_data *state);
void biquad_notch(uint32_t rate, float freq, float Q, struct biquad_data *state);
void biquad_peaking(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state);
void biquad_allpass(uint32_t rate, float freq, float Q, struct biquad_data *state);
void biquad_lowshelf(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state);
void biquad_highshelf(uint32_t rate, float freq, float Q, float gain, struct biquad_data *state);
void biquad_process(struct mixed_buffer *in, struct mixed_buffer *out, struct biquad_data *data);

inline void biquad_reset(struct biquad_data *data){
  data->x[0] = 0.0f;
  data->x[1] = 0.0f;
  data->y[0] = 0.0f;
  data->y[1] = 0.0f;
}

inline float biquad_sample(float sample, struct biquad_data *data){
  float b0 = data->b[0];
  float b1 = data->b[1];
  float b2 = data->b[2];
  float a1 = data->a[0];
  float a2 = data->a[1];
  float xn1 = data->x[0];
  float xn2 = data->x[1];
  float yn1 = data->y[0];
  float yn2 = data->y[1];

  float L =
    b0 * sample +
    b1 * xn1 +
    b2 * xn2 -
    a1 * yn1 -
    a2 * yn2;

  data->x[0] = sample;
  data->x[1] = xn1;
  data->y[0] = L;
  data->y[1] = yn1;
  return L;
}

float hilbert(float input, float *delay, uint32_t delay_size, uint32_t delay_i);

int mix_noop(struct mixed_segment *segment);

void mixed_err(int errorcode);

void *aligned_calloc(size_t alignment, size_t num, size_t size);
void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size);
void *aligned_crealloc(void *ptr, size_t alignment, size_t oldcount, size_t newcount, size_t size);

void *open_library(const char *file);
void close_library(void *handle);
void *load_symbol(void *handle, const char *name);

void set_info_field(struct mixed_segment_field_info *info, uint32_t field, enum mixed_segment_field_type type, uint32_t count, enum mixed_segment_info_flags flags, const char*description);
void clear_info_field(struct mixed_segment_field_info *info);

float mixed_random(void);
unsigned int mixed_random_int(void);

#define ARG(type, id) *(((type**)args)[id])
#define REGISTER_SEGMENT(name, function, count, ...)                    \
  static void __register_ ## name (void) __attribute__((constructor));  \
  void __register_ ## name(void){                                       \
    struct mixed_segment_field_info __args[count+1] = __VA_ARGS__;      \
    if(!mixed_register_segment(#name, count, __args, function)){        \
      fprintf(stderr, "libmixed: Failed to register segment %s: %s\n", #name, mixed_error_string(-1)); \
    }                                                                   \
  }

#define atomic_read(PLACE) __atomic_load_n(&PLACE, __ATOMIC_SEQ_CST)
#define atomic_write(PLACE, VAL) __atomic_store_n(&PLACE, VAL, __ATOMIC_SEQ_CST)
#define atomic_cas(PLACE, OLD, NEW) __sync_bool_compare_and_swap(&PLACE, OLD, NEW)
#define FREE(PLACE) if(PLACE){mixed_free(PLACE); PLACE=0;}

static inline float vec_dot(const float a[3], const float b[3]){
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static inline float vec_length(const float v[3]){
  return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static inline float vec_distance(const float a[3], const float b[3]){
  float c[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  return vec_length(c);
}

static inline float *vec_normalize(float o[3], const float v[3]){
  float length = vec_length(v);
  if(0 < length){
    o[0] = v[0]/length;
    o[1] = v[1]/length;
    o[2] = v[2]/length;
  }else{
    o[0] = 0.0;
    o[1] = 0.0;
    o[2] = 0.0;
  }
  return o;
}

static inline float *vec_normalized(float v[3]){
  return vec_normalize(v, v);
}

static inline float *vec_cross(float o[3], const float a[3], const float b[3]){
  float ax = a[0], ay = a[1], az = a[2];
  float bx = b[0], by = b[1], bz = b[2];
  o[0] = (ay * bz) - (az * by);
  o[1] = (az * bx) - (ax * bz);
  o[2] = (ax * by) - (ay * bx);
  return vec_normalize(o, o);
}

static inline float vec_angle(float a[3], float b[3]){
  float inner = vec_dot(a,b) / (vec_length(a)*vec_length(b));
  if(1.0 < inner) return 0.0;
  if(inner < -1.0) return M_PI;
  return fabs(acos(inner));
}

static inline float *vec_mul(float o[3], const float m[9], const float a[3]){
  float x = a[0], y = a[1], z = a[2];
  o[0] = x*m[0] + y*m[1] + z*m[2];
  o[1] = x*m[3] + y*m[4] + z*m[5];
  o[2] = x*m[6] + y*m[7] + z*m[8];
  return o;
}

static inline float *vec_lerp(float o[3], const float a[3], const float b[3], float x){
  float xx = 1-x;
  o[0] = a[0]*xx + b[0]*x;
  o[1] = a[1]*xx + b[1]*x;
  o[2] = a[2]*xx + b[2]*x;
  return o;
}

#define MAX_VBAP_SETS (MIXED_MAX_SPEAKER_COUNT*(MIXED_MAX_SPEAKER_COUNT-1)/2)
struct vbap_set{
  mixed_channel_t speakers[3];
  float inv_mat[9];
};

struct vbap_data{
  char dims;
  mixed_channel_t speaker_count;
  float speakers[MIXED_MAX_SPEAKER_COUNT][3];
  int set_count;
  struct vbap_set sets[MAX_VBAP_SETS];
};

int mixed_compute_gains(const float position[3], float gains[], mixed_channel_t speakers[], mixed_channel_t *count, struct vbap_data *data);
int make_vbap(float speakers[][3], mixed_channel_t speaker_count, int dim, struct vbap_data *data);
int make_vbap_from_configuration(struct mixed_channel_configuration const* configuration, struct vbap_data *data);
int make_vbap_from_channel_count(mixed_channel_t speaker_count, struct vbap_data *data);
