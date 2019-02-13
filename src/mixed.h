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
    MIXED_NO_ERROR = 0,
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
    // The value given for the field is in an invalid
    // range.
    MIXED_INVALID_VALUE,
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
    MIXED_LADSPA_INSTANTIATION_FAILED,
    // A libsamplerate operation failed.
    MIXED_RESAMPLE_FAILED
  };

  // This enum describes the possible sample encodings.
  MIXED_EXPORT enum mixed_encoding{
    MIXED_INT8 = 1,
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

  #define MIXED_ENCODING_COUNT 10

  // This enum describes the possible channel layouts.
  MIXED_EXPORT enum mixed_layout{
    // Channels are stored in alternating format.
    // Essentially: (C₁C₂...)ⁿ
    MIXED_ALTERNATING = 1,
    // Channels are stored in sequential format.
    // Essentially: C₁ⁿC₂ⁿ...
    MIXED_SEQUENTIAL
  };

  // This enum describes all possible flags of the
  // standard segments this library provides.
  MIXED_EXPORT enum mixed_segment_fields{
    // Access the backing buffer for this in/out.
    // The value must be a mixed_buffer struct.
    MIXED_BUFFER,
    // Access to the segment providing the data in
    // the buffer of this input. The source is an
    // attribute of the buffer and as such can only
    // be set once the buffer has been set, and is
    // also automatically removed when the buffer is
    // removed.
    // The value must be a mixed_segment struct.
    MIXED_SOURCE,
    // Whether to bypass the segment and effectively
    // disable its effects. The value is a bool.
    MIXED_BYPASS,
    // Access the sample rate by which the segment operates.
    MIXED_SAMPLERATE,
    // Access the volume of the segment.
    // The volume should be positive. Setting the volume
    // to values higher than one will result in distortion.
    // The type of this field should be a float.
    MIXED_VOLUME,
    // Access to the resampling quality of the audio pack.
    // The value must be from the mixed_resample_type enum.
    // The default is MIXED_SINC_FASTEST
    MIXED_PACKED_AUDIO_RESAMPLE_TYPE,
    // Access the panning of the general segment as a float.
    // The pan should be in the range of [-1.0, +1.0] where
    // -1 is all the way on the left and +1 is on the right.
    MIXED_VOLUME_CONTROL_PAN,
    // Access the starting volume for the fading as a float.
    // See MIXED_GENERAL_VOLUME
    MIXED_FADE_FROM,
    // Access the ending volume for the fading as a float.
    // See MIXED_GENERAL_VOLUME
    MIXED_FADE_TO,
    // Access the time (in seconds) it takes to fade
    // between the FROM and TO values as a float.
    MIXED_FADE_TIME,
    // Acccess the type of fading function that is used.
    // See mixed_fade_type
    MIXED_FADE_TYPE,
    // Access the frequency (in Hz) of the wave produced.
    // The type of this field must be a size_t.
    MIXED_GENERATOR_FREQUENCY,
    // Access the type of wave the generator produces.
    // The type of this field must match the enum.
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
    // The type of this field must be a float.
    MIXED_SPACE_SOUNDSPEED,
    // Access the doppler factor value as a float.
    // Changing this can exaggerate or dampen the doppler
    // effect's potency.
    // The default is 1.
    MIXED_SPACE_DOPPLER_FACTOR,
    // Access the minimal distance as a float.
    // Any distance lower than this will make the sound
    // appear at its maximal volume.
    // The default is 10.
    MIXED_SPACE_MIN_DISTANCE,
    // Access the maximal distance as a float.
    // Any distance greater than this will make the sound
    // appear at its minimal volume.
    // The default is 100000.
    MIXED_SPACE_MAX_DISTANCE,
    // Access the rolloff factor as a float.
    // This factor influences the curve of the attenuation
    // function and should be in the range of [0.0, 1.0].
    // The default is 0.5.
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
    // The default is attenuation_exponential.
    MIXED_SPACE_ATTENUATION,
    // Access the time, in seconds, by which the sound is
    // delayed.
    MIXED_DELAY_TIME,
    // Access the change of the pitch as a float.
    MIXED_PITCH_SHIFT,
    // Access the opening threshold for the gate as a float.
    // The threshold is in dB.
    // The default is -24dB
    MIXED_GATE_OPEN_THRESHOLD,
    // Access the closing threshold for the gate as a float.
    // The threshold is in dB.
    // The default is -32dB
    MIXED_GATE_CLOSE_THRESHOLD,
    // Access the attack time for the gate as a float.
    // The attack time is in seconds.
    // The default is 0.025s
    MIXED_GATE_ATTACK,
    // Access the hold time for the gate as a float.
    // The hold time is in seconds.
    // The default is 0.2s
    MIXED_GATE_HOLD,
    // Access the release time for the gate as a float.
    // The release time is in seconds.
    // The default is 0.15s
    MIXED_GATE_RELEASE,
    // Access the type of noise that is generated.
    MIXED_NOISE_TYPE,
    // Access how long the repeated segment is as a float.
    // The repeat time is in seconds.
    MIXED_REPEAT_TIME,
    // Access the current mode the repeater segment is in.
    MIXED_REPEAT_MODE,
    // Access the frequency cutoff of the low/high-pass filter.
    // This value is a size_t in Hertz.
    MIXED_FREQUENCY_CUTOFF,
    // Access whether to pass frequencies above or below the cutoff.
    // This value is an enum mixed_frequency_pass.
    MIXED_FREQUENCY_PASS,
    // Access the number of input buffers the segment can hold.
    // The value is a size_t.
    // The default is 2.
    MIXED_IN_COUNT,
    // Access the number of output buffers the segment can hold.
    // The value is a size_t.
    // The default is 2.
    MIXED_OUT_COUNT,
    // Returns the current segment in the queue.
    // The value is a pointer to a struct mixed_segment.
    MIXED_CURRENT_SEGMENT,
  };

  // This enum descripbes the possible resampling quality options.
  // 
  // The options are the same as in SRC/libsamplerate and the
  // descriptions are copied from its documentation.
  MIXED_EXPORT enum mixed_resample_type{
    // This is a bandlimited interpolator derived from the
    // mathematical sinc function and this is the highest
    // quality sinc based converter, providing a worst case
    // Signal-to-Noise Ratio (SNR) of 97 decibels (dB) at a
    // bandwidth of 97%.
    MIXED_SINC_BEST_QUALITY = 0,
    // This is another bandlimited interpolator much like
    // the previous one. It has an SNR of 97dB and a
    // bandwidth of 90%. The speed of the conversion is
    // much faster than the previous one.
    MIXED_SINC_MEDIUM_QUALITY,
    // This is the fastest bandlimited interpolator and has
    // an SNR of 97dB and a bandwidth of 80%.
    MIXED_SINC_FASTEST,
    // A Zero Order Hold converter (interpolated value is
    // equal to the last value). The quality is poor but the
    // conversion speed is blindlingly fast.
    MIXED_ZERO_ORDER_HOLD,
    // A linear converter. Again the quality is poor, but
    // the conversion speed is blindingly fast.
    MIXED_LINEAR_INTERPOLATION
  };

  // This enum describes the possible preset attenuation functions.
  MIXED_EXPORT enum mixed_attenuation{
    MIXED_NO_ATTENUATION = 1,
    MIXED_INVERSE_ATTENUATION,
    MIXED_LINEAR_ATTENUATION,
    MIXED_EXPONENTIAL_ATTENUATION
  };

  // This enum describes the possible fade easing function types.
  MIXED_EXPORT enum mixed_fade_type{
    MIXED_LINEAR = 1,
    MIXED_CUBIC_IN,
    MIXED_CUBIC_OUT,
    MIXED_CUBIC_IN_OUT
  };

  // This enum describes the possible generator wave types.
  MIXED_EXPORT enum mixed_generator_type{
    MIXED_SINE = 1,
    MIXED_SQUARE,
    MIXED_TRIANGLE,
    MIXED_SAWTOOTH
  };

  // This enum describes the possible noise types.
  MIXED_EXPORT enum mixed_noise_type{
    MIXED_WHITE_NOISE = 1,
    MIXED_PINK_NOISE,
    MIXED_BROWN_NOISE
  };

  // This enum describes the possible repeater segment modes.
  MIXED_EXPORT enum mixed_repeat_mode{
    MIXED_RECORD = 1,
    MIXED_PLAY
  };

  // This enum describes the possible pass directions.
  MIXED_EXPORT enum mixed_frequency_pass{
    MIXED_PASS_LOW = 1,
    MIXED_PASS_HIGH
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
    MIXED_GET = 0x10,
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

  // This enum holds type descriptors for the segment fields.
  MIXED_EXPORT enum mixed_segment_field_type{
    MIXED_UNKNOWN = 0, /* See MIXED_ENCODING
    MIXED_INT8 = 1,
    MIXED_UINT8,
    MIXED_INT16,
    MIXED_UINT16,
    MIXED_INT24,
    MIXED_UINT24,
    MIXED_INT32,
    MIXED_UINT32,
    MIXED_FLOAT,
    MIXED_DOUBLE */
    MIXED_BOOL = 11,
    MIXED_SIZE_T,
    MIXED_STRING,
    MIXED_FUNCTION,
    MIXED_POINTER,
    MIXED_SEGMENT_POINTER,
    MIXED_BUFFER_POINTER,
    MIXED_PACKED_AUDIO_POINTER,
    MIXED_SEGMENT_SEQUENCE_POINTER,
    MIXED_LOCATION_ENUM,
    MIXED_FREQUENCY_PASS_ENUM,
    MIXED_REPEAT_MODE_ENUM,
    MIXED_NOISE_TYPE_ENUM,
    MIXED_GENERATOR_TYPE_ENUM,
    MIXED_FADE_TYPE_ENUM,
    MIXED_ATTENUATION_ENUM,
    MIXED_LAYOUT_ENUM,
    MIXED_ENCODING_ENUM,
    MIXED_ERROR_ENUM,
    MIXED_RESAMPLE_TYPE_ENUM,
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
  MIXED_EXPORT struct mixed_packed_audio{
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
    // The base element type of the value contained in the
    // field. Note that while the get and set functions
    // always take a pointer, the dereferenced value is
    // interpreted as being of the type indicated by this
    // field.
    enum mixed_segment_field_type type;
    // In case the field stores an array of values, this
    // defines the number of values that can be dererefenced.
    size_t type_count;
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
    int (*info)(struct mixed_segment_info *info, struct mixed_segment *segment);
    int (*start)(struct mixed_segment *segment);
    int (*mix)(size_t samples, struct mixed_segment *segment);
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
  MIXED_EXPORT struct mixed_segment_sequence{
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

  // Convert the packed data to buffer data.
  //
  // This appropriately converts sample format and channel layout.
  // You are responsible for passing in an array of buffers that is
  // at least as long as the channel's channel count.
  // The volume is a multiplier you can pass to adjust the volume
  // in the resulting buffers.
  MIXED_EXPORT int mixed_buffer_from_packed_audio(struct mixed_packed_audio *in, struct mixed_buffer **outs, size_t samples, float volume);

  // Convert buffer data to the packed data.
  //
  // This appropriately converts sample format and channel layout.
  // You are responsible for passing in an array of buffers that is
  // at least as long as the channel's channel count.
  // The volume is a multiplier you can pass to adjust the volume
  // in the resulting channel.
  MIXED_EXPORT int mixed_buffer_to_packed_audio(struct mixed_buffer **ins, struct mixed_packed_audio *out, size_t samples, float volume);

  // Copy a buffer to another.
  //
  // This only copies as many samples as viable, meaning that if
  // one buffer is smaller than the other, only as many samples as
  // the smaller buffer can carry are copied.
  MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to);

  // Clear the buffer to make it empty again.
  // 
  // This clears the entire buffer to hold samples of all zeroes.
  MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer);

  // Resize the buffer to a new size.
  //
  // If the resizing operation fails due to a lack of memory, the
  // old data is preserved and the buffer is not changed.
  MIXED_EXPORT int mixed_buffer_resize(size_t size, struct mixed_buffer *buffer);

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
  // See mixed_segment_sequence_start
  MIXED_EXPORT int mixed_segment_start(struct mixed_segment *segment);

  // Run the segment for the given number of samples.
  //
  // If this returns zero, then the segment did not produce any
  // output and can't be expected to produce any until it is
  // restarted. This happens if the segment is some kind of
  // finite source and has ended, or if an internal error
  // occurred that prevents the segment from operating.
  //
  // See mixed_segment_sequence_mix
  MIXED_EXPORT int mixed_segment_mix(size_t samples, struct mixed_segment *segment);

  // End the segment's mixing process.
  //
  // If the method is not implemented, the error is set to
  // MIXED_NOT_IMPLEMENTED.
  //
  // See mixed_segment_sequence_end
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
  MIXED_EXPORT int mixed_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment);

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

  // An audio unpacker.
  //
  // This segment convers the data from the channel's data
  // array to the appropriate format as expected by the internal
  // mixing buffers. This involves channel layout separation and
  // sample format conversion.
  //
  // The sample rate given denotes the target sample rate of the
  // buffers connected to the outputs of this segment. The source
  // sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_unpacker(struct mixed_packed_audio *packed, size_t samplerate, struct mixed_segment *segment);

  // An audio packer.
  //
  // This segment converts the data from a number of buffers to
  // the appropriate format as expected by the given channel.
  // This involves channel layout packing and sample format
  // conversion.
  //
  // The sample rate given denotes the source sample rate of the
  // buffers connected to the inputs of this segment. The target
  // sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_packer(struct mixed_packed_audio *packed, size_t samplerate, struct mixed_segment *segment);

  // A basic, additive mixer
  //
  // This segment simply linearly mixes every input together into
  // output buffers. Each source must connect as many inputs as
  // there are channels. This means that if you have a channel
  // count of two and connect two sources A and B, the input
  // buffers must be connected as follows: A⁰ A¹ B⁰ B¹
  // The buffers must be set in a monotonically increasing order,
  // meaning you cannot leave gaps in the input location.
  // If you set a buffer to 0 it is removed and the buffers above
  // it are shifted downwards.
  //
  // Depending on how many sources you have, adding and removing
  // sources at random locations may prove expensive. Especially
  // adding more sources might involve allocations, which may not
  // be suitable for real-time behaviour. Aside from this caveat,
  // sources can be added or changed at any point in time.
  MIXED_EXPORT int mixed_make_segment_basic_mixer(size_t channels, struct mixed_segment *segment);

  // A very basic volume control segment
  //
  // This segment can be used to regulate the volume and pan of the
  // source. It is a strictly stereo segment and as such needs two
  // inputs and two outputs.
  //
  // The fields of this segment may be changed at any time.
  MIXED_EXPORT int mixed_make_segment_volume_control(float volume, float pan, struct mixed_segment *segment);

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

  // A space (3D) processed mixer
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
  MIXED_EXPORT int mixed_make_segment_space_mixer(size_t samplerate, struct mixed_segment *segment);

  // A delay segment
  //
  // This segment will simply delay the incoming samples to the output by
  // a specified amount of time. To do this, it will keep an internal
  // buffer of time*samplerate samples to store the incoming samples before
  // outputting them again once the delay time has been passed.
  //
  // This buffering means that delaying for a long time may take a lot of
  // memory, so watch out for that.
  MIXED_EXPORT int mixed_make_segment_delay(float time, size_t samplerate, struct mixed_segment *segment);

  // A repeat segment
  //
  // This segment allows you to repeat some input from a buffer, and then
  // simply loops the recorded bit to its output continuously. During the
  // recording time, the input is simply passed through.
  //
  // Since the repeated audio data needs to be recorded, a long repeat time
  // may take a lot of memory.
  MIXED_EXPORT int mixed_make_segment_repeat(float time, size_t samplerate, struct mixed_segment *segment);

  // A pitch shift segment
  //
  // This segment will shift the pitch of the incoming samples by a specified
  // amount. The pitch should be a float in the range ]0, infty[, where 1.0
  // means no change in pitch, 0.5 means half the pitch, 2.0 means double the
  // pitch and so on.
  MIXED_EXPORT int mixed_make_segment_pitch(float pitch, size_t samplerate, struct mixed_segment *segment);

  // A noise gate segment
  //
  // The gate has an open threshold, which if exceeded opens the gate. Audio
  // does not immediately pass through the gate upon opening though. It first
  // goes through an attack time during which the volume is linearly scaled up.
  // If the volume then ever goes below the close threshold, the gate stays
  // open for the duration of the hold time, after which it goes through a
  // linear fade out for the duration of the release time.
  MIXED_EXPORT int mixed_make_segment_gate(size_t samplerate, struct mixed_segment *segment);

  // A noise generator segment.
  //
  // This segment can generate white, pink, and brown noise.
  MIXED_EXPORT int mixed_make_segment_noise(enum mixed_noise_type type, struct mixed_segment *segment);

  // A frequency filter segment.
  //
  // The cutoff cannot be larger than the samplerate. Generally, if the cutoff
  // frequency is larger than half of the samplerate, major distortion will
  // occur, so tread carefully.
  MIXED_EXPORT int mixed_make_segment_frequency_pass(enum mixed_frequency_pass pass, size_t cutoff, size_t samplerate, struct mixed_segment *segment);

  // A queue segment for inner segments.
  //
  // The queue will delegate mixing to the first segment in its list until that
  // segment is explicitly removed, or returns zero from its mix function.
  // When a new segment is added to the queue, the queue will connect as many
  // of its buffers as appropriate. When the segment is removed, the buffers
  // are also unset.
  //
  // If the queue is empty or is being bypassed, it will simply copy the
  // contents of the input buffers to corresponding output buffers. If there are
  // more output buffers than input buffers, the remaining are cleared to zero.
  //
  // The number of bufers that can be connected to the queue is controlled by
  // the MIXED_IN_COUNT and MIXED_OUT_COUNT fields.
  //
  // The queue's info will reflect the capabilities of the first segment, if any,
  // and the queue's maximal capabilities otherwise.
  MIXED_EXPORT int mixed_make_segment_queue(struct mixed_segment *segment);
  MIXED_EXPORT int mixed_queue_add(struct mixed_segment *new, struct mixed_segment *queue);
  MIXED_EXPORT int mixed_queue_remove(struct mixed_segment *old, struct mixed_segment *queue);
  MIXED_EXPORT int mixed_queue_remove_at(size_t pos, struct mixed_segment *queue);
  MIXED_EXPORT int mixed_queue_clear(struct mixed_segment *queue);

  // Free the associated sequence data.
  MIXED_EXPORT void mixed_free_segment_sequence(struct mixed_segment_sequence *mixer);

  // Add a new segment to the mixer.
  //
  // This always adds to the end of the queue. You are responsible for
  // adding the segments in the proper order to ensure that their inputs
  // are satisfied.
  MIXED_EXPORT int mixed_segment_sequence_add(struct mixed_segment *segment, struct mixed_segment_sequence *mixer);

  // Remove a segment from the mixer.
  //
  // Segments after it will be shifted down as necessary.
  MIXED_EXPORT int mixed_segment_sequence_remove(struct mixed_segment *segment, struct mixed_segment_sequence *mixer);

  // Start the mixing process.
  //
  // This function must be called before you call mixed_segment_sequence_mix,
  // and it should be called relatively close to the first mix call. Changing
  // the in/outs or fields of a segment after start has been called will
  // result in undefined behaviour. If you do need to change the segments
  // or add new segments to the mixer, you must first call mixed_segment_sequence_end.
  //
  // Note that some segments may optionally support changing their in/outs
  // or flags while mixing has already been started, but this property is
  // not guaranteed. See the documentation of the individual segment.
  MIXED_EXPORT int mixed_segment_sequence_start(struct mixed_segment_sequence *mixer);

  // Performs the mixing of the given number of samples.
  //
  // In effect this calls the mix function of every segment in the mixer
  // in sequence. This function does not check for errors in any way.
  MIXED_EXPORT void mixed_segment_sequence_mix(size_t samples, struct mixed_segment_sequence *mixer);

  // End the mixing process.
  //
  // This function must be called after you are done mixing elements.
  // Once you have called end, you must call mixed_segment_sequence_start
  // again before you are allowed to mix.
  MIXED_EXPORT int mixed_segment_sequence_end(struct mixed_segment_sequence *mixer);

  // Return the size of a sample in the given encoding in bytes.
  MIXED_EXPORT uint8_t mixed_samplesize(enum mixed_encoding encoding);

  // Return the current error code.
  //
  // The error code is thread-local in order to allow multiple operations
  // at the same time.
  MIXED_EXPORT int mixed_error();

  // Return the error string for the given error code.
  //
  // If the error code is less than zero, the error string for the
  // error code returned by mixed_error(); is returned instead.
  MIXED_EXPORT char *mixed_error_string(int error_code);

  // Returns the version string of the library.
  MIXED_EXPORT char *mixed_version();

#ifdef __cplusplus
}
#endif
#endif
