#ifndef __LIBMIXED_H__
#define __LIBMIXED_H__
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#  ifdef MIXED_STATIC_DEFINE
#    define MIXED_EXPORT
#  else
#    ifndef MIXED_EXPORT
#      define MIXED_EXPORT __declspec(dllexport)
#    endif
#  endif
#else
#  define MIXED_EXPORT
#endif
#include <stdint.h>
#include <stdlib.h>
#include "conversion.h"

  enum mixed_error{
    MIXED_NO_ERROR,
    MIXED_OUT_OF_MEMORY,
    MIXED_UNKNOWN_ENCODING,
    MIXED_UNKNOWN_LAYOUT,
    MIXED_INPUT_TAKEN,
    MIXED_INPUT_MISSING,
    MIXED_GRAPH_CYCLE,
    MIXED_MIXING_FAILED
  };

  enum mixed_encoding{
    MIXED_INT8,
    MIXED_UINT8,
    MIXED_INT16,
    MIXED_UINT16,
    MIXED_INT24,
    MIXED_UINT24,
    MIXED_INT32,
    MIXED_UINT32,
    MIXED_INT64,
    MIXED_UINT64,
    MIXED_FLOAT,
    MIXED_DOUBLE
  };

  enum mixed_layout{
    MIXED_ALTERNATING,
    MIXED_SEQUENTIAL
  };

  enum mixed_channel_type{
    MIXED_SOURCE,
    MIXED_DRAIN
  };

  struct mixed_channel{
    enum mixed_channel_type type;
    void *data;
    size_t size;
    enum mixed_encoding encoding;
    uint8_t channels;
    enum mixed_layout layout;
    size_t samplerate;
  };

  struct mixed_buffer{
    float *data;
    size_t size;
    size_t samplerate;
  };

  struct mixed_segment{
    int (*mix)(size_t samples, struct mixed_segment *segment);
    struct mixed_buffer *inputs;
    struct mixed_buffer *outputs;
    void *data;
  };

  struct mixed_connection{
    void *from;
    uint8_t from_output;
    void *to;
    uint8_t to_input;
  };

  struct mixed_vector{
    void *data;
    size_t count;
    size_t size;
  };

  struct mixed_graph{
    struct mixed_vector *connections;
  };

  struct mixed_mixer{
    struct mixed_buffer *buffers;
    struct mixed_segment **segments;
    size_t samplerate;
  };
  
  int mixed_buffer_make(struct mixed_buffer *buffer);
  void mixed_buffer_free(struct mixed_buffer *buffer);
  int mixed_to_buffers(struct mixed_channel *in, struct mixed_buffer **outs);
  int mixed_to_channel(struct mixed_buffer **ins, struct mixed_channel *out);

  int mixed_vector_make(struct mixed_vector *vector);
  void mixed_vector_free(struct mixed_vector *vector);
  int mixed_vector_push(void *element, struct mixed_vector *vector);
  int mixed_vector_pushnew(void *element, struct mixed_vector *vector);
  void *mixed_vector_pop(struct mixed_vector *vector);
  int mixed_vector_remove(void *element, struct mixed_vector *vector);

  int mixed_graph_make(struct mixed_graph *graph);
  void mixed_graph_free(struct mixed_graph *graph);
  int mixed_graph_connect(void *source, uint8_t out, void *target, uint8_t in, struct mixed_graph *graph);
  int mixed_graph_disconnect(void *source, uint8_t out, void *target, uint8_t in, struct mixed_graph *graph);
  int mixed_graph_remove(void *segment, struct mixed_graph *graph);
  int mixed_graph_elements(struct mixed_vector *vector, struct mixed_graph *graph);
  
  int mixed_free_mixer(struct mixed_mixer *mixer);
  int mixed_pack_mixer(struct mixed_graph *graph, struct mixed_mixer *mixer);
  int mixed_mix(size_t samples, struct mixed_mixer *mixer);

  enum mixed_error mixed_error();
  char *mixed_error_string(enum mixed_error error_code);

#ifdef __cplusplus
}
#endif
#endif
