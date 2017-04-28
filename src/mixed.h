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
#include "encoding.h"

  enum mixed_error{
    MIXED_NO_ERROR,
    MIXED_OUT_OF_MEMORY,
    MIXED_UNKNOWN_ENCODING,
    MIXED_UNKNOWN_LAYOUT,
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
    MIXED_FLOAT,
    MIXED_DOUBLE
  };

  enum mixed_layout{
    MIXED_ALTERNATING,
    MIXED_SEQUENTIAL
  };

  enum mixed_space_field{
    MIXED_LISTENER_LOCATION,
    MIXED_SOURCE_LOCATION
  };

  struct mixed_buffer{
    float *data;
    size_t size;
  };
  
  struct mixed_channel{
    void *data;
    size_t size;
    enum mixed_encoding encoding;
    uint8_t channels;
    enum mixed_layout layout;
    size_t samplerate;
  };

  struct mixed_segment_info{
    char *name;
    char *description;
    size_t min_inputs;
    size_t max_inputs;
    size_t outputs;
  };

  struct mixed_segment{
    // Mix samples
    int (*start)(struct mixed_segment *segment);
    int (*mix)(size_t samples, size_t samplerate, struct mixed_segment *segment);
    int (*end)(struct mixed_segment *segment);
    // Clean up
    int (*free)(struct mixed_segment *segment);
    // Connect input
    int (*connect)(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment);
    // Disconnect input
    int (*disconnect)(size_t location, struct mixed_segment *segment);
    // Request info
    struct mixed_segment_info (*info)(struct mixed_segment *segment);
    // Opaque fields
    int (*get)(size_t field, void **value, struct mixed_segment *segment);
    int (*set)(size_t field, void *value, struct mixed_segment *segment);
    // User data
    void *data;
  };

  struct mixed_mixer{
    struct mixed_segment **segments;
    size_t samplerate;
  };
  
  int mixed_make_buffer(struct mixed_buffer *buffer);
  void mixed_free_buffer(struct mixed_buffer *buffer);
  int mixed_buffer_from_channel(struct mixed_channel *in, struct mixed_buffer **outs);
  int mixed_buffer_to_channel(struct mixed_buffer **ins, struct mixed_channel *out);
  int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to);

  // For a source channel converter
  int mixed_make_segment_source(struct mixed_channel *channel, struct mixed_segment *segment);
  // For a drain channel converter
  int mixed_make_segment_drain(struct mixed_channel *channel, struct mixed_segment *segment);
  // For a linear mixer
  int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment);
  // For a basic effect change
  int mixed_make_segment_general(float volume, float pan, float pitch, struct mixed_segment *segment);
  // For a space (3D) processed effect
  int mixed_make_segment_space(struct mixed_buffer *segment);
  // For a LADSPA-based step
  int mixed_make_segment_ladspa(char *file, size_t index, struct mixed_segment *segment);
  int mixed_free_segment(struct mixed_segment *segment);

  int mixed_mixer_start(struct mixed_mixer *mixer);
  int mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer);
  int mixed_mixer_end(struct mixed_mixer *mixer);

  enum mixed_error mixed_error();
  char *mixed_error_string(enum mixed_error error_code);

#ifdef __cplusplus
}
#endif
#endif
