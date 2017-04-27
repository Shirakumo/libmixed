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

  enum mixed_error{
    MIXED_NO_ERROR,
    MIXED_OUT_OF_MEMORY,
    MIXED_UNKNOWN_ENCODING,
    MIXED_UNKNOWN_LAYOUT,
    MIXED_INPUT_TAKEN,
    MIXED_INPUT_MISSING,
    MIXED_GRAPH_CYCLE,
    
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

  struct mixed_channel{
    void *data;
    size_t size;
    mixed_encoding encoding;
    uint8_t channels;
    mixed_layout layout;
  };

  struct mixed_buffer{
    float *data;
    size_t size;
  };

  struct mixed_segment{
    int (*mix)(size_t samples);
    struct mixed_buffer *inputs;
    struct mixed_buffer *outputs;
  };

  struct mixed_connection{
    struct mixed_segment *from;
    uint8_t from_output;
    struct mixed_segment *to;
    uint8_t to_input;
  };

  struct mixed_pipeline{
    struct mixed_segment *segments;
    size_t segment_size;
    struct mixed_connection *connections;
    size_t connection_size;
  };

  struct mixed_mixer{
    struct mixed_channel *inputs;
    struct mixed_channel *outputs;
    struct mixed_buffer *buffers;
    struct mixed_segment *segments;
  };

  int mixed_connect(struct mixed_segment *source, uint8_t out, struct mixed_segment *target, uint8_t in, struct mixed_pipeline *pipeline);
  int mixed_disconnect(struct mixed_segment *source, uint8_t out, struct mixed_segment *target, uint8_t in, struct mixed_pipeline *pipeline);
  int mixed_connect_all(struct mixed_segment *source, struct mixed_segment *target, struct mixed_pipeline *pipeline);
  int mixed_remove_segment(struct mixed_segment *segment, struct mixed_pipeline *pipeline);
  int mixed_remove_segment_mending(struct mixed_segment *segment, struct mixed_pipeline *pipeline);
  
  int mixed_free_mixer(struct mixed_mixer *mixer);
  int mixed_pack(struct mixed_pipeline *pipeline, struct mixed_mixer *mixer);
  int mixed_mix(struct mixed_mixer *mixer, size_t samples);

  int mixed_to_buffer(struct mixed_channel *in, struct mixed_buffer *outs);
  int mixed_to_channel(struct mixed_buffer *ins, struct mixed_channel *out);

  enum mixed_error mixed_error();
  char *mixed_error_string(enum mixed_error error_code);

#ifdef __cplusplus
}
#endif
