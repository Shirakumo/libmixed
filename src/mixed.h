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
    MIXED_NO_ERROR=1,
    MIXED_OUT_OF_MEMORY,
    MIXED_UNKNOWN_ENCODING,
    MIXED_UNKNOWN_LAYOUT,
    MIXED_MIXING_FAILED,
    MIXED_NOT_IMPLEMENTED,
    MIXED_NOT_INITIALIZED,
    MIXED_INVALID_BUFFER_LOCATION,
    MIXED_INVALID_FIELD,
    MIXED_SEGMENT_ALREADY_STARTED,
    MIXED_SEGMENT_ALREADY_ENDED,
    MIXED_LADSPA_OPEN_FAILED,
    MIXED_LADSPA_BAD_LIBRARY,
    MIXED_LADSPA_NO_PLUGIN_AT_INDEX,
    MIXED_LADSPA_INSTANTIATION_FAILED
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

  enum mixed_segment_fields{
    MIXED_GENERAL_VOLUME,
    MIXED_GENERAL_PAN,
    MIXED_FADE_FROM,
    MIXED_FADE_TO,
    MIXED_FADE_TIME,
    MIXED_FADE_TYPE
  };

  enum mixed_fade_type{
    MIXED_LINEAR,
    MIXED_CUBIC_IN,
    MIXED_CUBIC_OUT,
    MIXED_CUBIC_IN_OUT
  };

  enum mixed_segment_info_flags{
    MIXED_INPLACE = 0x1
  };

  enum mixed_channel_index{
    MIXED_MONO = 0,
    MIXED_LEFT = 0,
    MIXED_RIGHT = 1,
    MIXED_LEFT_FRONT = 0,
    MIXED_RIGHT_FRONT = 1,
    MIXED_LEFT_REAR = 2,
    MIXED_RIGHT_REAR = 3,
    MIXED_CENTER = 4,
    MIXED_SUBWOOFER = 5
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
    const char *name;
    const char *description;
    enum mixed_segment_info_flags flags;
    size_t min_inputs;
    size_t max_inputs;
    size_t outputs;
  };

  struct mixed_segment{
    // Clean up
    int (*free)(struct mixed_segment *segment);
    // Mix samples
    int (*start)(struct mixed_segment *segment);
    int (*mix)(size_t samples, struct mixed_segment *segment);
    int (*end)(struct mixed_segment *segment);
    // Connect buffer
    int (*set_in)(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment);
    int (*set_out)(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment);
    // Request info
    struct mixed_segment_info (*info)(struct mixed_segment *segment);
    // Opaque fields
    int (*set)(size_t field, void *value, struct mixed_segment *segment);
    int (*get)(size_t field, void *value, struct mixed_segment *segment);
    // User data
    void *data;
  };

  struct mixed_mixer{
    struct mixed_segment **segments;
    size_t count;
    size_t size;
    size_t samplerate;
  };
  
  int mixed_make_buffer(struct mixed_buffer *buffer);
  void mixed_free_buffer(struct mixed_buffer *buffer);
  int mixed_buffer_from_channel(struct mixed_channel *in, struct mixed_buffer **outs, size_t samples);
  int mixed_buffer_to_channel(struct mixed_buffer **ins, struct mixed_channel *out, size_t samples);
  int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to);

  int mixed_free_segment(struct mixed_segment *segment);
  int mixed_segment_start(struct mixed_segment *segment);
  int mixed_segment_mix(size_t samples, struct mixed_segment *segment);
  int mixed_segment_end(struct mixed_segment *segment);
  int mixed_segment_set_in(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment);
  int mixed_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment);
  struct mixed_segment_info mixed_segment_info(struct mixed_segment *segment);
  int mixed_segment_set(size_t field, void *value, struct mixed_segment *segment);
  int mixed_segment_get(size_t field, void *value, struct mixed_segment *segment);
  
  // For a source channel converter
  int mixed_make_segment_source(struct mixed_channel *channel, struct mixed_segment *segment);
  // For a drain channel converter
  int mixed_make_segment_drain(struct mixed_channel *channel, struct mixed_segment *segment);
  // For a linear mixer
  int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment);
  // For a basic effect change
  int mixed_make_segment_general(float volume, float pan, struct mixed_segment *segment);
  // For a volume fade in/out effect
  int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, size_t samplerate, struct mixed_segment *segment);
  // For a space (3D) processed effect
  int mixed_make_segment_space(struct mixed_buffer *segment);
  // For a LADSPA-based step
  int mixed_make_segment_ladspa(char *file, size_t index, size_t samplerate, struct mixed_segment *segment);

  int mixed_free_mixer(struct mixed_mixer *mixer);
  int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer);
  int mixed_mixer_remove(struct mixed_segment *segment, struct mixed_mixer *mixer);
  int mixed_mixer_start(struct mixed_mixer *mixer);
  int mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer);
  int mixed_mixer_end(struct mixed_mixer *mixer);

  uint8_t mixed_samplesize(enum mixed_encoding encoding);
  int mixed_error();
  char *mixed_error_string(int error_code);

#ifdef __cplusplus
}
#endif
#endif
