#ifndef __LIBMIXED_H__
#define __LIBMIXED_H__
#ifdef __cplusplus
extern "C" {
#endif

#if defined MIXED_STATIC
#  define MIXED_EXPORT
#elif defined _MSC_VER
#  if defined MIXED_BUILD
#    define MIXED_EXPORT __declspec(dllexport)
#  else
#    define MIXED_EXPORT __declspec(dllimport)
#  endif
#elif defined __GNUC__
#  if defined MIXED_BUILD
#    define MIXED_EXPORT __attribute__((visibility("default")))
#  else
#    define MIXED_EXPORT
#  endif
#else
#  define MIXED_EXPORT
#endif
#include <stdint.h>
#include <stdlib.h>
#include "encoding.h"

  // This enum describes all possible error codes.
  MIXED_EXPORT enum mixed_error{
    // No error has occurred yet.
    MIXED_NO_ERROR,
    // An allocation failed. It's likely that you
    // are out of memory and it is game over.
    MIXED_OUT_OF_MEMORY,
    // The sample encoding you specified is unknown.
    // Please see the mixed_encoding enum.
    MIXED_UNKNOWN_ENCODING,
    // The channel layout you specified is unknown.
    // Please see the mixed_layout enum.
    MIXED_UNKNOWN_LAYOUT,
    // The mixing process failed for some reason.
    MIXED_MIXING_FAILED,
    // The segment does not implement the method you
    // called.
    MIXED_NOT_IMPLEMENTED,
    // The segment is not yet initialised and a call
    // to this method is erroneous in this condition.
    MIXED_NOT_INITIALIZED,
    // The buffer location you used is invalid for
    // this segment.
    MIXED_INVALID_LOCATION,
    // The field you tried to set is invalid for this
    // segment and/or buffer of this segment.
    MIXED_INVALID_FIELD,
    // You tried to start a segment again after it had
    // already been started before.
    MIXED_SEGMENT_ALREADY_STARTED,
    // You tried to end a segment again before it had
    // been started.
    MIXED_SEGMENT_ALREADY_ENDED,
    // The LADSPA library failed to be opened. It is
    // likely the file does not exist or is not readable.
    MIXED_LADSPA_OPEN_FAILED,
    // The LADSPA library did not seem to contain a valid
    // plugin.
    MIXED_LADSPA_BAD_LIBRARY,
    // The index into the library you gave did not resolve
    // to a valid LADSPA plugin descriptor.
    MIXED_LADSPA_NO_PLUGIN_AT_INDEX,
    // The instantiation of the LADSPA plugin handle failed
    // for some reason.
    MIXED_LADSPA_INSTANTIATION_FAILED
  };

  // This enum describes the possible sample encodings.
  MIXED_EXPORT enum mixed_encoding{
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

  // This enum describes the possible channel layouts.
  MIXED_EXPORT enum mixed_layout{
    // Channels are stored in alternating format.
    // Essentially: (C₁C₂...)ⁿ
    MIXED_ALTERNATING,
    // Channels are stored in sequential format.
    // Essentially: C₁ⁿC₂ⁿ...
    MIXED_SEQUENTIAL
  };

  // This enum describes all possible flags of the
  // standard segments this library provides.
  MIXED_EXPORT enum mixed_segment_fields{
    // Access the backing buffer for this in/out.
    MIXED_BUFFER,
    // Access to the resampling function of the channel.
    // The resampling function must have the same signature
    // as mixed_resample_linear below.
    MIXED_CHANNEL_RESAMPLER,
    // Access the volume of the general segment.
    // The volume should be positive. Setting the volume
    // to values higher than one will result in distortion.
    MIXED_GENERAL_VOLUME,
    // Access the panning of the general segment.
    // The pan should be in the range of [-1.0, +1.0] where
    // -1 is all the way on the left and +1 is on the right.
    MIXED_GENERAL_PAN,
    // Access the starting volume for the fading.
    // See MIXED_GENERAL_VOLUME
    MIXED_FADE_FROM,
    // Access the ending volume for the fading.
    // See MIXED_GENERAL_VOLUME
    MIXED_FADE_TO,
    // Access the time (in seconds) it takes to fade
    // between the FROM and TO values.
    MIXED_FADE_TIME,
    // Acccess the type of fading function that is used.
    // See mixed_fade_type
    MIXED_FADE_TYPE,
    // Access the frequency (in Hz) of the wave produced.
    MIXED_GENERATOR_FREQUENCY,
    // Access the type of wave the generator produces.
    // See mixed_generator_type
    MIXED_GENERATOR_TYPE,
    // Access the location of the source or listener.
    // The value should be an array of three floats.
    // The default is 0,0,0.
    MIXED_SPACE_LOCATION,
    // Access the direction of the listener.
    // The value should be an array of three floats.
    // The default is 0,0,1.
    MIXED_SPACE_DIRECTION,
    // Access the velocity of the source or listener.
    // The value should be an array of three floats.
    // The default is 0,0,0.
    MIXED_SPACE_VELOCITY,
    // Access the "up" vector of the listener.
    // The value should be an array of three floats.
    // The default is 0,1,0.
    MIXED_SPACE_UP,
    // Access the speed of sound in space. This also
    // implicitly sets the units used for the calculations.
    // The default is 34330.0, meaning the unit is centi-
    // meters and the speed is for clean air at ~20°C.
    MIXED_SPACE_SOUNDSPEED,
    // Access the doppler factor value. Changing this can
    // exaggerate or dampen the doppler effect.
    MIXED_SPACE_DOPPLER_FACTOR,
    // Access the minimal distance.
    // Any distance lower than this will make the sound
    // appear at its maximal volume.
    MIXED_SPACE_MIN_DISTANCE,
    // Access the maximal distance.
    // Any distance greater than this will make the sound
    // appear at its minimal volume.
    MIXED_SPACE_MAX_DISTANCE,
    // Access the rolloff factor.
    // This factor influences the curve of the attenuation
    // function.
    MIXED_SPACE_ROLLOFF,
    // Access the attenuation function.
    // this function calculates how quickly the sound gets
    // louder or quieter based on its distance from the
    // listener. You may set either one of the preset
    // attenuation functions using the mixed_attenuation
    // enum, or you may set your own function pointer. This
    // function should accept four values, all of them floats:
    //
    // * minimal distance
    // * maximal distance
    // * actual distance
    // * rolloff factor
    //
    // The function should return a float describing the
    // volume multiplier of the source.
    MIXED_SPACE_ATTENUATION
  };

  // This enum describes the possible preset attenuation functions.
  MIXED_EXPORT enum mixed_attenuation{
    MIXED_NO_ATTENUATION,
    MIXED_INVERSE_ATTENUATION,
    MIXED_LINEAR_ATTENUATION,
    MIXED_EXPONENTIAL_ATTENUATION
  };

  // This enum describes the possible fade easing function types.
  MIXED_EXPORT enum mixed_fade_type{
    MIXED_LINEAR,
    MIXED_CUBIC_IN,
    MIXED_CUBIC_OUT,
    MIXED_CUBIC_IN_OUT
  };

  // This enum describes the possible generator wave types.
  MIXED_EXPORT enum mixed_generator_type{
    MIXED_SINE,
    MIXED_SQUARE,
    MIXED_TRIANGLE,
    MIXED_SAWTOOTH
  };

  // This enum holds property flags for segments.
  MIXED_EXPORT enum mixed_segment_info_flags{
    // This means that the segment's output and input
    // buffers may be the same, as it processes the samples
    // in place.
    MIXED_INPLACE = 0x1,
    // This means that the segment will modify the samples in
    // its input buffers, making them potentially unusable for
    // an input buffer for other segments.
    MIXED_MODIFIES_INPUT = 0x2,
    // The field is available for inputs.
    MIXED_IN = 0x1,
    // The field is available for outputs.
    MIXED_OUT = 0x2,
    // The field is available for segments.
    MIXED_SEGMENT = 0x4,
    // The field can be set.
    MIXED_SET = 0x8,
    // The field can be get.
    MIXED_GET = 0x10
  };

  // Convenience enum to map common speaker channels to buffer locations.
  //
  // It is not required that a segment follow this channel
  // layout scheme, but it is heavily recommended.
  MIXED_EXPORT enum mixed_location{
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

  // An internal audio data buffer.
  //
  // The sample array is always stored in floats.
  MIXED_EXPORT struct mixed_buffer{
    float *data;
    size_t size;
  };

  // Information struct to encapsulate a "channel"
  //
  // Channels are representing external audio sources or
  // drains. If you have a sound system or some kind of
  // sound file reader, you will want to represent them
  // using this struct.
  //
  // Using this struct you can then create a converter
  // segment to include the external audio into the mix.
  MIXED_EXPORT struct mixed_channel{
    // Pointer to the encoded sample data that this channel
    // consumes or provides.
    void *data;
    // The length of the data array in bytes.
    size_t size;
    // The sample encoding in the byte array.
    enum mixed_encoding encoding;
    // The number of channels that are packed into the array.
    uint8_t channels;
    // The layout the channels have within the array.
    enum mixed_layout layout;
    // The sample rate at which data is encoded in Hz.
    size_t samplerate;
  };

  // Metadata struct for a segment's field.
  //
  // This struct can be used to figure out what kind of
  // fields are recognised on the segment and what you can
  // do with it.
  MIXED_EXPORT struct mixed_segment_field_info{
    // The actual field index.
    size_t field;
    // A human-readable description of the data accessed.
    char *description;
    // An OR-combination of flags that describe the field's
    // properties, usually about whether it is valid for
    // inputs, outputs, or the segment itself, and whether
    // it can be set or get.
    enum mixed_segment_info_flags flags;
  };

  // Metadata struct for a segment.
  //
  // This struct contains useful information that describes
  // the behaviour and capabilities of the segment. You may
  // want to look at this if you want to provide a generic
  // interface to segments.
  MIXED_EXPORT struct mixed_segment_info{
    // The name of the segment type as a string.
    // Must be alphanumeric ASCII.
    const char *name;
    // A human-readable description of the segment's purpose.
    // Must be UTF-8 encoded.
    const char *description;
    // An OR combination of segment info flags.
    enum mixed_segment_info_flags flags;
    // The minimal number of inputs that this segment requires.
    size_t min_inputs;
    // The maximal number of inputs that this segment can support.
    size_t max_inputs;
    // The number of outputs that this segment provides.
    size_t outputs;
    // A null-terminated array of possible fields that this
    // segment supports.
    struct mixed_segment_field_info fields[32];
  };

  // The primary segment struct.
  //
  // Segments are what make up the mixing pipeline in libmixed.
  // Each segment is responsible for performing some kind of audio
  // operation like generating signals, outputting them, or
  // transforming them in some way.
  //
  // Each segment should be created by first allocating an instance
  // of this struct, zeroing it out, and then calling the 
  // appropriate "make" function to fill in the fields of the
  // struct as required.
  //
  // A segment's method is considered "not implemented" if its
  // struct field is set to zero.
  //
  // Each segment must implement the mix method, and at least
  // either the set_in or set_out method. For the documentation
  // of each of the methods, see the corresponding shorthand
  // function below.
  MIXED_EXPORT struct mixed_segment{
    int (*free)(struct mixed_segment *segment);
    struct mixed_segment_info *(*info)(struct mixed_segment *segment);
    int (*start)(struct mixed_segment *segment);
    void (*mix)(size_t samples, struct mixed_segment *segment);
    int (*end)(struct mixed_segment *segment);
    int (*set_in)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*set_out)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*get_in)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*get_out)(size_t field, size_t location, void *value, struct mixed_segment *segment);
    int (*set)(size_t field, void *value, struct mixed_segment *segment);
    int (*get)(size_t field, void *value, struct mixed_segment *segment);
    // An opaque pointer to internal data for the segment.
    void *data;
  };

  // The primary mixer control unit.
  //
  // This merely holds the segments in the order that they should
  // be processed and acts as a shorthand to perform bulk operations
  // on them like starting, mixing, or ending.
  //
  // You should not modify any of its fields directly.
  MIXED_EXPORT struct mixed_mixer{
    struct mixed_segment **segments;
    size_t count;
    size_t size;
  };

  // Note that while this API deals with sound and you will probably
  // want to use threads to handle the playback, it is in itself not
  // thread safe and does not do any kind of locking or mutual
  // exclusion for you. Calling any combination of functions on the
  // same instance in parallel is very likely going to land you in a
  // world of pain very quickly.

  // Most functions in this API return an int, which will be either
  // 1 for success, or 0 for failure. In the case of failure you
  // should immediately call mixed_err to get the corresponding error
  // code.

  // All of the make* functions in this API expect to be passed a
  // pointer to a struct. This struct has to be alloced by you and it
  // must be zeroed out. Failure to zero the struct out will lead to
  // problems very quickly.

  // All of the free* functions in this API should be safe to call
  // multiple times on the same instance. Performing any other kind
  // of operation on the instance after it has been freed will lead
  // to undefined behaviour. If you want to recycle the instance, you
  // will have to zero out the struct and call the correct make
  // function again.

  // Allocate the buffer's internal storage array.
  MIXED_EXPORT int mixed_make_buffer(size_t size, struct mixed_buffer *buffer);

  // Free the buffer's internal storage array.
  MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer);

  // Convert the channel data to buffer data.
  //
  // This appropriately converts sample format and channel layout.
  // You are responsible for passing in an array of buffers that is
  // at least as long as the channel's channel count.
  MIXED_EXPORT int mixed_buffer_from_channel(struct mixed_channel *in, struct mixed_buffer **outs, size_t samples);

  // Convert buffer data to the channel data.
  //
  // This appropriately converts sample format and channel layout.
  // You are responsible for passing in an array of buffers that is
  // at least as long as the channel's channel count.
  MIXED_EXPORT int mixed_buffer_to_channel(struct mixed_buffer **ins, struct mixed_channel *out, size_t samples);

  // Resample the buffer using nearest-neighbor.
  //
  // This is the fastest and most primitive resampling you could
  // possibly do. It will most likely sound pretty horrid if you
  // have to upsample. Downsampling might be alright.
  MIXED_EXPORT int mixed_resample_nearest(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples);

  // Resample the buffer using linear interpolation.
  //
  // Linear interpolation should give "ok" results as long as the
  // resampling factor is not too big.
  MIXED_EXPORT int mixed_resample_linear(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples);

  // Resample the buffer using a cubic hermite spline.
  //
  // Cubic hermite spline should give fairly good interpolation
  // results. Naturally you can't expect magic either, as the
  // interpolation will always remain an interpolation and not a
  // CSI enhance. This resampling will become costly at large
  // buffer sizes and might not be suitable for real-time systems.
  MIXED_EXPORT int mixed_resample_cubic(struct mixed_buffer *in, size_t in_samplerate, struct mixed_buffer *out, size_t out_samplerate, size_t out_samples);

  // Copy a buffer to another.
  //
  // This only copies as many samples as viable, meaning that if
  // one buffer is smaller than the other, only as many samples as
  // the smaller buffer can carry are copied.
  MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to);

  // Free the segment's internal data.
  //
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // After this function has been run on a segment, it becomes
  // useless and must be discarded. Using any other segment
  // method after it has been freed results in undefined
  // behaviour.
  MIXED_EXPORT int mixed_free_segment(struct mixed_segment *segment);
  
  // Start the segment's mixing process.
  //
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_mixer_start
  MIXED_EXPORT int mixed_segment_start(struct mixed_segment *segment);

  // Run the segment for the given number of samples.
  //
  // See mixed_mixer_mix
  MIXED_EXPORT void mixed_segment_mix(size_t samples, struct mixed_segment *segment);

  // End the segment's mixing process.
  // 
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_mixer_end
  MIXED_EXPORT int mixed_segment_end(struct mixed_segment *segment);

  // Set the value of an input buffer field.
  //
  // If the location does not designate a valid input buffer, the
  // error code is set to MIXED_INVALID_LOCATION.
  // If implemented, at least the field MIXED_BUFFER must be
  // recognised.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_segment_set
  MIXED_EXPORT int mixed_segment_set_in(size_t field, size_t location, void *value, struct mixed_segment *segment);

  // Set the value of an output buffer field.
  //
  // If the location does not designate a valid output buffer, the
  // error code is set to MIXED_INVALID_LOCATION.
  // If implemented, at least the field MIXED_BUFFER must be
  // recognised.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_segment_set
  MIXED_EXPORT int mixed_segment_set_out(size_t field, size_t location, void *value, struct mixed_segment *segment);

  // Get the value of an input buffer field.
  //
  // If the location does not designate a valid input buffer, the
  // error code is set to MIXED_INVALID_LOCATION.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_segment_get
  MIXED_EXPORT int mixed_segment_get_in(size_t field, size_t location, void *value, struct mixed_segment *segment);

  // Get the value of an output buffer field.
  //
  // If the location does not designate a valid output buffer, the
  // error code is set to MIXED_INVALID_LOCATION.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  // 
  // See mixed_segment_get.
  MIXED_EXPORT int mixed_segment_get_out(size_t field, size_t location, void *value, struct mixed_segment *segment);

  // Return a pointer to a struct containing general information about the segment.
  //
  // This is mostly useful for introspective / reflective purposes.
  // You must free the returned struct yourself.
  //
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED and a null pointer is returned.
  MIXED_EXPORT struct mixed_segment_info *mixed_segment_info(struct mixed_segment *segment);

  // Set the value of a field in the segment.
  //
  // The value must be a pointer to a place that can be
  // resolved to the data that the field should be set to. It is
  // your responsibility to make sure that the value contains the
  // appropriate data as expected by the segment. Consult the
  // documentation of  the individual fields and segments.
  //
  // If the field is not recognised, the error is set to
  // MIXED_INVALID_FIELD.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_set(size_t field, void *value, struct mixed_segment *segment);

  // Get the value of a field in the segment.
  //
  // The value must be a pointer to a place that can be set to
  // the data that is fetched from the field in the segment.
  // It is your responsibility to make sure that there is enough
  // space available. Consult the documentation of the individual
  // fields and segments.
  //
  // If the field is not recognised, the error is set to
  // MIXED_INVALID_FIELD.
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_get(size_t field, void *value, struct mixed_segment *segment);
  
  // A source channel converter
  //
  // This segment convers the data from the channel's data
  // array to the appropriate format as expected by the internal
  // mixing buffers. This involves channel layout separation and
  // sample format conversion.
  //
  // The sample rate given denotes the target sample rate of the
  // buffers connected to the outputs of this segment. The source
  // sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_source(struct mixed_channel *channel, size_t samplerate, struct mixed_segment *segment);
  
  // A drain channel converter
  //
  // This segment converts the data from a number of buffers to
  // the appropriate format as expected by the given channel.
  // This involves channel layout packing and sample format
  // conversion.
  //
  // The sample rate given denotes the source sample rate of the
  // buffers connected to the inputs of this segment. The target
  // sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_drain(struct mixed_channel *channel, size_t samplerate, struct mixed_segment *segment);
  
  // A linear mixer
  //
  // This segment simply linearly mixes every input together into
  // a single output. As such, it accepts an arbitrary number of
  // inputs and a single output. The input buffers must be set in
  // strictly monotonically increasing order. If you set a buffer
  // to zero, the corresponding input is removed and all inputs
  // above it are shifted down.
  //
  // Depending on how many sources you have, adding and removing
  // sources at random locations may prove expensive. Especially
  // adding more sources might involve allocations, which may not
  // be suitable for real-time behaviour. Aside from this caveat,
  // sources can be added or changed at any point in time.
  //
  // The buffers argument can be a null-delimited array of pointers
  // to buffers, or null if you want to add the buffers later. The
  // array is not shared.
  MIXED_EXPORT int mixed_make_segment_mixer(struct mixed_buffer **buffers, struct mixed_segment *segment);
  
  // A very basic effect segment
  //
  // This segment can be used to regulate the volume and pan of the
  // source. It is a strictly stereo segment and as such needs two
  // inputs and two outputs.
  //
  // The fields of this segment may be changed at any time.
  MIXED_EXPORT int mixed_make_segment_general(float volume, float pan, struct mixed_segment *segment);
  
  // A volume fade in/out effect
  //
  // If you need to smoothly fade in a track, this segment is for you.
  // The fade type determines the easing function used to fade the
  // input in.
  //
  // The segment takes an arbitrary number of inputs and outputs, but
  // the number of them must be matched up, as each input will simply
  // be faded to the output at the same index.
  //
  // The fields of this segment may be changed at any time on the fly,
  // but there will not be any smooth transitioning between them and
  // there is thus little reason to ever do this, aside from perhaps
  // re-using the same segment to fade the input out again at the
  // appropriate moment.
  MIXED_EXPORT int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, size_t samplerate, struct mixed_segment *segment);
  
  // A simple wave generator source
  //
  // This segment simply produces a constant signal of the requested
  // frequency and wave form type. You may change the frequency and
  // wave form type at any time. Potentially this could be used to
  // create a very primitive synthesizer.
  MIXED_EXPORT int mixed_make_segment_generator(enum mixed_generator_type type, size_t frequency, size_t samplerate, struct mixed_segment *segment);
  
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
  MIXED_EXPORT int mixed_make_segment_ladspa(char *file, size_t index, size_t samplerate, struct mixed_segment *segment);
  
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
  MIXED_EXPORT int mixed_make_segment_space(size_t samplerate, struct mixed_segment *segment);

  // Free the associated mixer data.
  MIXED_EXPORT void mixed_free_mixer(struct mixed_mixer *mixer);
  
  // Add a new segment to the mixer.
  //
  // This always adds to the end of the queue. You are responsible for
  // adding the segments in the proper order to ensure that their inputs
  // are satisfied.
  MIXED_EXPORT int mixed_mixer_add(struct mixed_segment *segment, struct mixed_mixer *mixer);

  // Remove a segment from the mixer.
  //
  // Segments after it will be shifted down as necessary.
  MIXED_EXPORT int mixed_mixer_remove(struct mixed_segment *segment, struct mixed_mixer *mixer);
  
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
  MIXED_EXPORT int mixed_mixer_start(struct mixed_mixer *mixer);
  
  // Performs the mixing of the given number of samples.
  //
  // In effect this calls the mix function of every segment in the mixer
  // in sequence. This function does not check for errors in any way.
  MIXED_EXPORT void mixed_mixer_mix(size_t samples, struct mixed_mixer *mixer);

  // End the mixing process.
  //
  // This function must be called after you are done mixing elements.
  // Once you have called end, you must call mixed_mixer_start again
  // before you are allowed to mix.
  MIXED_EXPORT int mixed_mixer_end(struct mixed_mixer *mixer);

  // Return the size of a sample in the given encoding in bytes.
  MIXED_EXPORT uint8_t mixed_samplesize(enum mixed_encoding encoding);

  // Return the current error code.
  MIXED_EXPORT int mixed_error();

  // Return the error string for the given error code.
  //
  // If the error code is less than zero, the error string for the
  // error code returned by mixed_error(); is returned instead.
  MIXED_EXPORT char *mixed_error_string(int error_code);

#ifdef __cplusplus
}
#endif
#endif
