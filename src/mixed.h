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
    MIXED_MIXING_FAILED,
    MIXED_NOT_IMPLEMENTED,
    MIXED_NOT_INITIALIZED,
    MIXED_INVALID_LOCATION,
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

  enum mixed_segment_fields{
    MIXED_BUFFER,
    MIXED_GENERAL_VOLUME,
    MIXED_GENERAL_PAN,
    MIXED_FADE_FROM,
    MIXED_FADE_TO,
    MIXED_FADE_TIME,
    MIXED_FADE_TYPE,
    MIXED_GENERATOR_FREQUENCY,
    MIXED_GENERATOR_TYPE,
    MIXED_SPACE_LOCATION,
    MIXED_SPACE_DIRECTION,
    MIXED_SPACE_VELOCITY,
    MIXED_SPACE_UP,
    MIXED_SPACE_SOUNDSPEED,
    MIXED_SPACE_DOPPLER_FACTOR,
    MIXED_SPACE_MIN_DISTANCE,
    MIXED_SPACE_MAX_DISTANCE,
    MIXED_SPACE_ROLLOFF,
    MIXED_SPACE_ATTENUATION
  };

  enum mixed_attenuation{
    MIXED_NO_ATTENUATION,
    MIXED_INVERSE_ATTENUATION,
    MIXED_LINEAR_ATTENUATION,
    MIXED_EXPONENTIAL_ATTENUATION
  };

  enum mixed_fade_type{
    MIXED_LINEAR,
    MIXED_CUBIC_IN,
    MIXED_CUBIC_OUT,
    MIXED_CUBIC_IN_OUT
  };

  enum mixed_generator_type{
    MIXED_SINE,
    MIXED_SQUARE,
    MIXED_TRIANGLE,
    MIXED_SAWTOOTH
  };

  enum mixed_segment_info_flags{
    MIXED_INPLACE = 0x1,
    MIXED_MODIFIES_SOURCE = 0x2
  };

  enum mixed_location{
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
    // FIXME: fields
  };

  struct mixed_segment{
    int (*free)(struct mixed_segment *segment);
    struct mixed_segment_info (*info)(struct mixed_segment *segment);
    int (*start)(struct mixed_segment *segment);
    int (*mix)(size_t samples, struct mixed_segment *segment);
    int (*end)(struct mixed_segment *segment);
    int (*set_in)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*set_out)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*get_in)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*get_out)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*set)(size_t field, void *value, struct mixed_segment *segment);
    int (*get)(size_t field, void *value, struct mixed_segment *segment);
    void *data;
  };

  struct mixed_mixer{
    struct mixed_segment **segments;
    size_t count;
    size_t size;
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
  int mixed_segment_set_in(size_t field, size_t location, void *value, struct mixed_segment *segment);
  int mixed_segment_set_out(size_t field, size_t location, void *value, struct mixed_segment *segment);
  int mixed_segment_get_in(size_t field, size_t location, void *value, struct mixed_segment *segment);
  int mixed_segment_get_out(size_t field, size_t location, void *value, struct mixed_segment *segment);
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
  
  // A simple wave generator source
  //
  // 
  int mixed_make_segment_generator(enum mixed_generator_type type, size_t frequency, size_t samplerate, struct mixed_segment *segment);
  
  // A LADSPA plugin segment
  //
  // LADSPA (Linux Audio Developers Simple Plugin API) is a standard for
  // sound plugins, effects, and generators. You can find a lot of plugins
  // for direct use to download on the internet. Using this segment, you
  // can then integrate such a plugin into your mixer pipeline.
  //
  // Note that while libmixed makes a hard distinction between fields and
  // input and output buffers, LADSPA does not. A LADSPA plugin merely has
  // a list of "ports", each of which can be an input or an output, and
  // a control or an audio port. All input audio ports are mapped to the
  // set_in function, starting from 0 and increasing by one for each port.
  // The same goes for output audio ports, except naturally being mapped on
  // set_out. The control ports are mapped to set/get respectively. In
  // essence what this means is that the following list of ports of a
  // LADSPA plugin
  //
  // * 0 in audio (mono in)
  // * 1 out audio (mono out)
  // * 2 in control (pitch)
  // * 3 in control (rate)
  //
  // Would be mapped to the libmixed API as follows:
  //
  // * in MIXED_BUFFER 0
  // * out MIXED_BUFFER 0
  // * set 0
  // * set 1
  //
  // So each "category" of port receives its own counting scheme.
  //
  // Note that a lot of plugins expect their control ports to be set to
  // some kind of value beforehand. Libmixed will bind all of them and
  // default their values to zero. It is your responsibility to set them
  // to the desired, proper values. Also note that all fields for this
  // plugin expect pointers to floats for their values. Thus, setting a
  // field will usually look like this:
  //
  //   float value = 1.0;
  //   mixed_segment_set(0, &value, &segment);
  //
  // This segment does not expressly allow you to change fields and
  // buffers while mixing has already been started. Generally, LADSPA
  // requires you to stop mixing before being able to change any property
  // but some plugins may nevertheless work despite that. Thus, consult
  // your plugin's source or documentation.
  int mixed_make_segment_ladspa(char *file, size_t index, size_t samplerate, struct mixed_segment *segment);
  
  // A space (3D) processed effect
  // 
  // This segment is capable of mixing sources according to their position
  // and movement in space. It thus simulates the behaviour of sound in a
  // 3D environment. This segment takes an arbitrary number of mono inputs
  // and has two outputs (left and right). Each input has two additional
  // fields aside from the buffer:
  //
  // * MIXED_SPACE_LOCATION
  // * MIXED_SPACE_VELOCITY
  //
  // The position, velocity, and general properties of the space mixing
  // are configured through the general field set/get functions. The
  // following flags are understood:
  //
  // * MIXED_SPACE_LOCATION
  // * MIXED_SPACE_VELOCITY
  // * MIXED_SPACE_DIRECTION
  // * MIXED_SPACE_UP
  // * MIXED_SPACE_SOUNDSPEED
  // * MIXED_SPACE_DOPPLER_FACTOR
  // * MIXED_SPACE_MIN_DISTANCE
  // * MIXED_SPACE_MAX_DISTANCE
  // * MIXED_SPACE_ROLLOFF
  // * MIXED_SPACE_ATTENUATION
  //
  // See the MIXED_FIELDS enum for the documentation of each field.
  // This segment does allow you to change fields and buffers while the
  // mixing has already been started.
  int mixed_make_segment_space(size_t samplerate, struct mixed_segment *segment);

  // Free the associated mixer data.
  int mixed_free_mixer(struct mixed_mixer *mixer);
  
  // Add a new segment to the mixer.
  //
  // This always adds to the end of the queue. You are responsible for
  // adding the segments in the proper order to ensure that their inputs
  // are satisfied.
  int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer);

  // Remove a segment from the mixer.
  //
  // Segments after it will be shifted down as necessary.
  int mixed_mixer_remove(struct mixed_segment *segment, struct mixed_mixer *mixer);
  
  // Start the mixing process.
  //
  // This function must be called before you call mixed_mixer_mix, and it
  // should be called relatively close to the first mix call. Changing
  // the in/outs or fields of a segment after start has been called will
  // result in undefined behaviour. If you do need to change the segments
  // or add new segments to the mixer, you must first call mixed_mixer_end.
  //
  // Note that some segments may optionally support changing their in/outs
  // or flags while mixing has already been started, but this property is
  // not guaranteed. See the documentation of the individual segment.
  int mixed_mixer_start(struct mixed_mixer *mixer);
  
  // Performs the mixing of the given number of samples.
  //
  // In effect this calls the mix function of every segment in the mixer
  // in sequence. The mixing is aborted on the first segment whose mix
  // function returns false.
  int mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer);

  // End the mixing process.
  //
  // This function must be called after you are done mixing elements.
  // Once you have called end, you must call mixed_mixer_start again
  // before you are allowed to mix.
  int mixed_mixer_end(struct mixed_mixer *mixer);

  // Return the size of a sample in the given encoding in bytes.
  uint8_t mixed_samplesize(enum mixed_encoding encoding);

  // Return the current error code.
  int mixed_error();

  // Return the error string for the given error code.
  //
  // If the error code is less than zero, the error string for the
  // error code returned by mixed_error(); is returned instead.
  char *mixed_error_string(int error_code);

#ifdef __cplusplus
}
#endif
#endif
