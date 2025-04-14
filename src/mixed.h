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
#include <math.h>
#include "mixed_encoding.h"

  /// Note that while this API deals with sound and you will probably
  /// want to use threads to handle the playback, it is in itself not
  /// thread safe and does not do any kind of locking or mutual
  /// exclusion for you. Calling any combination of functions on the
  /// same instance in parallel is very likely going to land you in a
  /// world of pain very quickly.

  /// Most functions in this API return an int, which will be either
  /// 1 for success, or 0 for failure. In the case of failure you
  /// should call mixed_err to get the corresponding error code before
  /// you call any other API functions on the same thread.

  /// All of the make* functions in this API expect to be passed a
  /// pointer to a struct. This struct has to be alloced by you and it
  /// must be zeroed out. Failure to zero the struct out will lead to
  /// problems very quickly.

  /// All of the free* functions in this API should be safe to call
  /// multiple times on the same instance. Performing any other kind
  /// of operation on the instance after it has been freed will lead
  /// to undefined behaviour. If you want to recycle the instance, you
  /// will have to zero out the struct and call the correct make
  /// function again.

  /// This enum describes all possible error codes.
  /// 
  MIXED_EXPORT enum mixed_error{
    /// No error has occurred yet.
    /// 
    MIXED_NO_ERROR = 0,
    /// An allocation failed. It's likely that you
    /// are out of memory and it is game over.
    MIXED_OUT_OF_MEMORY,
    /// The sample encoding you specified is unknown.
    /// Please see the mixed_encoding enum.
    MIXED_UNKNOWN_ENCODING,
    /// The mixing process failed for some reason.
    /// 
    MIXED_MIXING_FAILED,
    /// The segment does not implement the method you
    /// called.
    MIXED_NOT_IMPLEMENTED,
    /// The segment is not yet initialised and a call
    /// to this method is erroneous in this condition.
    MIXED_NOT_INITIALIZED,
    /// The buffer location you used is invalid for
    /// this segment.
    MIXED_INVALID_LOCATION,
    /// The field you tried to set is invalid for this
    /// segment and/or buffer of this segment.
    MIXED_INVALID_FIELD,
    /// The value given for the field is in an invalid
    /// range.
    MIXED_INVALID_VALUE,
    /// You tried to start a segment again after it had
    /// already been started before.
    MIXED_SEGMENT_ALREADY_STARTED,
    /// You tried to end a segment again before it had
    /// been started.
    MIXED_SEGMENT_ALREADY_ENDED,
    /// The dynamic library failed to be opened. It is
    /// likely the file does not exist or is not readable.
    MIXED_DYNAMIC_OPEN_FAILED,
    /// The dynamic library did not seem to contain a valid
    /// plugin.
    MIXED_BAD_DYNAMIC_LIBRARY,
    /// The index into the library you gave did not resolve
    /// to a valid LADSPA plugin descriptor.
    MIXED_LADSPA_NO_PLUGIN_AT_INDEX,
    /// The instantiation of the LADSPA plugin handle failed
    /// for some reason.
    MIXED_LADSPA_INSTANTIATION_FAILED,
    /// A libsamplerate operation failed.
    /// 
    MIXED_RESAMPLE_FAILED,
    /// The buffer is empty and there's nothing that can be
    /// read from it.
    MIXED_BUFFER_EMPTY,
    /// The buffer is full and there's nothing that can be
    /// written to it.
    MIXED_BUFFER_FULL,
    /// The amount of data that is attempted to be committed
    /// to the buffer is more than was requested.
    MIXED_BUFFER_OVERCOMMIT,
    /// The change in the sample rates is too big.
    /// 
    MIXED_BAD_RESAMPLE_FACTOR,
    /// An unsupported channel conversion configuration was
    /// requested.
    MIXED_BAD_CHANNEL_CONFIGURATION,
    /// An allocated buffer was passed when an unallocated
    /// one was expected.
    MIXED_BUFFER_ALLOCATED,
    /// An input or output port is missing a buffer.
    /// 
    MIXED_BUFFER_MISSING,
    /// A segment with the requested name had already been
    /// registered.
    MIXED_DUPLICATE_SEGMENT,
    /// A segment with the requested name is not registered.
    /// 
    MIXED_BAD_SEGMENT,
    /// A function was specified with a bad number of arguments.
    MIXED_BAD_ARGUMENT_COUNT,
    /// A name was too long or malformed.
    MIXED_BAD_NAME,
    /// A buffer is too small
    MIXED_BUFFER_TOO_SMALL,
  };

  /// This enum describes the possible sample encodings.
  /// All encodings are little-endian.
  /// 
  MIXED_EXPORT enum mixed_encoding{
    /// Corresponds to int8_t (signed 8-bit integer)
    MIXED_INT8 = 1,
    /// Corresponds to uint8_t (unsigned 8-bit integer)
    MIXED_UINT8,
    /// Corresponds to int16_t (signed 16-bit integer)
    MIXED_INT16,
    /// Corresponds to uint16_t (unsigned 16-bit integer)
    MIXED_UINT16,
    /// Corresponds to int24_t (signed 24-bit integer)
    MIXED_INT24,
    /// Corresponds to uint24_t (unsigned 24-bit integer)
    MIXED_UINT24,
    /// Corresponds to int32_t (signed 32-bit integer)
    MIXED_INT32,
    /// Corresponds to uint32_t (unsigned 32-bit integer)
    MIXED_UINT32,
    /// Corresponds to float (IEEE 754 single precision floating point)
    MIXED_FLOAT,
    /// Corresponds to double (IEEE 754 double precision floating point)
    MIXED_DOUBLE
  };

  /// This enum describes all possible flags of the
  /// standard segments this library provides.
  MIXED_EXPORT enum mixed_segment_fields{
    /// Access the backing buffer for this in/out.
    /// The value must be a mixed_buffer struct.
    MIXED_BUFFER,
    /// Whether to bypass the segment and effectively
    /// disable its effects. The value is a bool.
    MIXED_BYPASS,
    /// Access the sample rate by which the segment operates.
    /// 
    MIXED_SAMPLERATE,
    /// Access the linear volume multiplier of the segment.
    /// The volume should be positive. Setting the volume
    /// to values higher than one will result in distortion.
    /// The type of this field should be a float.
    MIXED_VOLUME,
    /// Access to the resampling quality.
    /// The value must be from the mixed_resample_type enum.
    /// The default is MIXED_SINC_FASTEST
    MIXED_RESAMPLE_TYPE,
    /// Access the panning of the general segment as a float.
    /// The pan should be in the range of [-1.0, +1.0] where
    /// -1 is all the way on the left and +1 is on the right.
    MIXED_VOLUME_CONTROL_PAN,
    /// Access the starting volume for the fading as a float.
    /// See MIXED_GENERAL_VOLUME
    MIXED_FADE_FROM,
    /// Access the ending volume for the fading as a float.
    /// See MIXED_GENERAL_VOLUME
    MIXED_FADE_TO,
    /// Access the time (in seconds) it takes to fade
    /// between the FROM and TO values as a float.
    MIXED_FADE_TIME,
    /// Acccess the type of fading function that is used.
    /// See mixed_fade_type
    MIXED_FADE_TYPE,
    /// Access the frequency (in Hz) of the wave produced.
    /// The type of this field must be a uint32_t.
    MIXED_GENERATOR_FREQUENCY,
    /// Access the type of wave the generator produces.
    /// The type of this field must match the enum.
    /// See mixed_generator_type
    MIXED_GENERATOR_TYPE,
    /// Access the location of the source or listener.
    /// The value should be an array of three floats.
    /// The default is 0,0,0
    MIXED_SPACE_LOCATION,
    /// Access the direction of the listener.
    /// The value should be an array of three floats.
    /// The default is 0,0,1
    MIXED_SPACE_DIRECTION,
    /// Access the velocity of the source or listener.
    /// The value should be an array of three floats.
    /// The default is 0,0,0
    MIXED_SPACE_VELOCITY,
    /// Access the "up" vector of the listener.
    /// The value should be an array of three floats.
    /// The default is 0,1,0
    MIXED_SPACE_UP,
    /// Access the speed of sound in space. This also
    /// implicitly sets the units used for the calculations.
    /// The default is 34330.0, meaning the unit is centi-
    /// meters and the speed is for clean air at ~20°C.
    /// The type of this field must be a float.
    MIXED_SPACE_SOUNDSPEED,
    /// Access the doppler factor value as a float.
    /// Changing this can exaggerate or dampen the doppler
    /// effect's potency.
    /// The default is 1
    MIXED_SPACE_DOPPLER_FACTOR,
    /// Access the minimal distance as a float.
    /// Any distance lower than this will make the sound
    /// appear at its maximal volume.
    /// The default is 10
    MIXED_SPACE_MIN_DISTANCE,
    /// Access the maximal distance as a float.
    /// Any distance greater than this will make the sound
    /// appear at its minimal volume.
    /// The default is 100000
    MIXED_SPACE_MAX_DISTANCE,
    /// Access the rolloff factor as a float.
    /// This factor influences the curve of the attenuation
    /// function and should be in the range of [0.0, 1.0].
    /// The default is 0.5
    MIXED_SPACE_ROLLOFF,
    /// Access the attenuation function.
    /// this function calculates how quickly the sound gets
    /// louder or quieter based on its distance from the
    /// listener. You may set either one of the preset
    /// attenuation functions using the mixed_attenuation
    /// enum, or you may set your own function pointer. This
    /// function should accept four values, all of them floats:
    ///
    /// * minimal distance
    /// * maximal distance
    /// * actual distance
    /// * rolloff factor
    ///
    /// The function should return a float describing the
    /// volume multiplier of the source.
    /// The default is attenuation_exponential.
    MIXED_SPACE_ATTENUATION,
    /// Access the time, in seconds, by which the sound is
    /// delayed.
    MIXED_DELAY_TIME,
    /// Access the change of the pitch as a float.
    /// 
    MIXED_PITCH_SHIFT,
    /// Access the opening threshold for the gate as a float.
    /// The threshold is in dB.
    /// The default is -24dB
    MIXED_GATE_OPEN_THRESHOLD,
    /// Access the closing threshold for the gate as a float.
    /// The threshold is in dB.
    /// The default is -32dB
    MIXED_GATE_CLOSE_THRESHOLD,
    /// Access the attack time for the gate as a float.
    /// The attack time is in seconds.
    /// The default is 0.025s
    MIXED_GATE_ATTACK,
    /// Access the hold time for the gate as a float.
    /// The hold time is in seconds.
    /// The default is 0.2s
    MIXED_GATE_HOLD,
    /// Access the release time for the gate as a float.
    /// The release time is in seconds.
    /// The default is 0.15s
    MIXED_GATE_RELEASE,
    /// Access the type of noise that is generated.
    /// 
    MIXED_NOISE_TYPE,
    /// Access how long the repeated segment is as a float.
    /// The repeat time is in seconds.
    MIXED_REPEAT_TIME,
    /// Access the current mode the repeater segment is in.
    /// 
    MIXED_REPEAT_MODE,
    /// Access the frequency cutoff of the filter.
    /// This value is a float in Hertz
    MIXED_FREQUENCY,
    /// Access the type of filter this is.
    /// This value is an enum mixed_frequency_pass
    MIXED_BIQUAD_FILTER,
    /// Access the gain of the filter.
    /// This value is a float
    MIXED_GAIN,
    /// Access the Q or resonance factor of the filter.
    /// This value is a float
    MIXED_Q,
    /// Access the number of input buffers the segment holds.
    /// The value is a uint32_t.
    /// The default is 2
    MIXED_IN_COUNT,
    /// Access the number of output buffers the segment holds.
    /// The value is a uint32_t.
    /// The default is 2
    MIXED_OUT_COUNT,
    /// Returns the current segment in the queue.
    /// The value is a pointer to a struct mixed_segment.
    MIXED_CURRENT_SEGMENT,
    /// Access the speed factor
    /// 
    MIXED_SPEED_FACTOR,
    /// The number of quantization steps
    /// 
    MIXED_QUANTIZE_STEPS,
    /// For an effects segment, how strongly to mix the effects
    /// output with the input. This allows smoothly transitioning
    /// effects. Has to be in [0,1] with 0 (dry) being no effect
    /// output and 1 being full effect output (wet).
    /// Setting this may also affect the MIXED_BYPASS value.
    /// The default is 1.0
    MIXED_MIX,
    /// Access the location of the source or listener.
    /// The value should be an array of two floats.
    /// The default is 0,0
    MIXED_PLANE_LOCATION,
    /// Access the velocity of the source or listener.
    /// The value should be an array of two floats.
    /// The default is 0,0
    MIXED_PLANE_VELOCITY,
    /// The gain applied before compressing the signal, in dB.
    /// The default is 0
    MIXED_COMPRESSOR_PREGAIN,
    /// The threshold to activate compression, in dB.
    /// The default is -24
    MIXED_COMPRESSOR_THRESHOLD,
    /// The knee of the compressor curve.
    /// The default is 30
    MIXED_COMPRESSOR_KNEE,
    /// The compression ratio.
    /// The default is 12
    MIXED_COMPRESSOR_RATIO,
    /// The compressor's attack time in seconds.
    /// The default is 0.003
    MIXED_COMPRESSOR_ATTACK,
    /// The compressor's release time in seconds.
    /// The default is 0.25
    MIXED_COMPRESSOR_RELEASE,
    /// The compressor's delay before compression, in seconds.
    /// The default is 0.006
    MIXED_COMPRESSOR_PREDELAY,
    /// The compressor's release zone fields
    /// The value should be an array of four floats.
    /// The default is 0.09, 0.16, 0.42, 0.96
    MIXED_COMPRESSOR_RELEASEZONE,
    /// The gain applied after compression, in dB.
    /// The default is 0
    MIXED_COMPRESSOR_POSTGAIN,
    /// The actual gain that was applied during compression.
    /// Can only be read.
    MIXED_COMPRESSOR_GAIN,
    /// The number of channels that the upmixer should convert from.
    /// 
    MIXED_CHANNEL_COUNT_IN,
    /// The number of channels that the upmixer should convert to.
    /// 
    MIXED_CHANNEL_COUNT_OUT,
    /// Access the current position of the repeater buffer.
    /// 
    MIXED_REPEAT_POSITION,
    /// The frame size of the FFT, must be a powerof two in [2^1, 2^13]
    /// The default is 2048
    MIXED_FRAMESIZE,
    /// The oversampling rate between FFT frames.
    /// The default is 4
    MIXED_OVERSAMPLING,
    /// The suggested minimum size of buffers attached to the segment.
    /// Supplying a smaller buffer may result in degenerate
    /// performance or fail altogether.
    /// Can only be read.
    /// The default is 1
    MIXED_BUFFER_SIZE_HINT,
  };

  /// This enum descripbes the possible resampling quality options.
  /// 
  /// The options are the same as in SRC/libsamplerate and the
  /// descriptions are copied from its documentation.
  MIXED_EXPORT enum mixed_resample_type{
    /// This is a bandlimited interpolator derived from the
    /// mathematical sinc function and this is the highest
    /// quality sinc based converter, providing a worst case
    /// Signal-to-Noise Ratio (SNR) of 97 decibels (dB) at a
    /// bandwidth of 97%.
    MIXED_SINC_BEST_QUALITY = 0,
    /// This is another bandlimited interpolator much like
    /// the previous one. It has an SNR of 97dB and a
    /// bandwidth of 90%. The speed of the conversion is
    /// much faster than the previous one.
    MIXED_SINC_MEDIUM_QUALITY,
    /// This is the fastest bandlimited interpolator and has
    /// an SNR of 97dB and a bandwidth of 80%.
    MIXED_SINC_FASTEST,
    /// A Zero Order Hold converter (interpolated value is
    /// equal to the last value). The quality is poor but the
    /// conversion speed is blindlingly fast.
    MIXED_ZERO_ORDER_HOLD,
    /// A linear converter. Again the quality is poor, but
    /// the conversion speed is blindingly fast.
    MIXED_LINEAR_INTERPOLATION
  };

  /// This enum describes the possible preset attenuation functions.
  /// 
  MIXED_EXPORT enum mixed_attenuation{
    /// No distance attenuation is performed at all and the source is always at its base volume.
    ///
    ///   v = 1.0
    MIXED_NO_ATTENUATION = 1,
    /// Attenuation increases inversely correlated with distance to the minimum.
    ///
    ///   v = min / (min + roll*(dist-min))
    MIXED_INVERSE_ATTENUATION,
    /// Attenuation increases linearly with distance between the maximum and minimum.
    /// 
    ///   v = 1 - r * (dist-min) / (max-min)
    MIXED_LINEAR_ATTENUATION,
    /// Attenuation increases exponentially with distance from the minimum.
    /// 
    ///   v = 1 / (dist/min)^roll
    MIXED_EXPONENTIAL_ATTENUATION
  };

  /// This enum describes the possible fade easing function types.
  /// 
  MIXED_EXPORT enum mixed_fade_type{
    /// Fade with a constant linear curve
    ///
    ///   v = t
    MIXED_LINEAR = 1,
    /// Fade with a cubic slope
    ///
    ///   v = t^3
    MIXED_CUBIC_IN,
    /// Fade with an inverse cubic slope
    ///
    ///   v = (t-1)^3 + 1
    MIXED_CUBIC_OUT,
    /// Fade with a centred cubic slope
    ///
    ///   v = { (2t)^3 / 2         | t < 0.5
    ///       { (2(t-1))^3 / 2 + 1 | otherwise 
    MIXED_CUBIC_IN_OUT
  };

  /// This enum describes the possible generator wave types.
  /// 
  MIXED_EXPORT enum mixed_generator_type{
    /// Generate an exact sine wave
    MIXED_SINE = 1,
    /// Generate a square wave with instant switches
    MIXED_SQUARE,
    /// Generate a linear alternating triangle wave
    MIXED_TRIANGLE,
    /// Generate a linear triangle wave with instant drop
    MIXED_SAWTOOTH
  };

  /// This enum describes the possible noise types.
  /// 
  MIXED_EXPORT enum mixed_noise_type{
    /// Generate purely random white noise
    MIXED_WHITE_NOISE = 1,
    /// Generate pink fractal noise
    MIXED_PINK_NOISE,
    /// Generate Brownian noise
    MIXED_BROWN_NOISE
  };

  /// This enum describes the possible repeater segment modes.
  /// 
  MIXED_EXPORT enum mixed_repeat_mode{
    /// Record the incoming audio stream into the buffer
    MIXED_RECORD = 1,
    /// Play the recorded buffer back to the output stream
    MIXED_PLAY,
    /// Record exactly once until the buffer is full then switch to play mode
    MIXED_RECORD_ONCE
  };

  /// This enum describes the possible biquad filters.
  /// 
  MIXED_EXPORT enum mixed_biquad_filter{
    /// Perform a low frequency pass.
    /// Only frequencies below the given frequency are let through.
    /// The Q parameter acts as the resonance tuning parameter.
    MIXED_LOWPASS = 1,
    /// Perform a high frequency pass.
    /// Only frequencies above the given frequency are let through.
    /// The Q parameter acts as the resonance tuning parameter.
    MIXED_HIGHPASS,
    /// Perform a frequency band pass.
    /// Only frequencies within a band around the given frequency are let through.
    /// The Q parameter directs the band width.
    MIXED_BANDPASS,
    /// Perform a frequency band notch.
    /// Frequencies within the band around the given frequency are blocked.
    /// The Q parameter directs the band width.
    MIXED_NOTCH,
    /// Perform a frequency peak notch.
    /// Frequencies around the given frequency are attenuated, with the peak at the given frequency.
    /// The Q parameter directs the peak slope.
    MIXED_PEAKING,
    /// Perform an all-pass filter.
    /// Frequencies should not attenuated, but rather phase-shifted.
    MIXED_ALLPASS,
    /// Perform a low frequency shelf.
    /// Similar to the lowpass, but using a sloped attenuation.
    /// The Q parameter should be 1.
    MIXED_LOWSHELF,
    /// Perform a high frequency shelf.
    /// Similar to the highpass, but using a sloped attenuation.
    /// The Q parameter should be 1.
    MIXED_HIGHSHELF
  };

  /// This enum holds property flags for segments.
  /// 
  MIXED_EXPORT enum mixed_segment_info_flags{
    /// This means that the segment's output and input
    /// buffers may be the same, as it processes the samples
    /// in place.
    MIXED_INPLACE        = 0x0001,
    /// This means that the segment will modify the samples in
    /// its input buffers, making them unusable for virtual
    /// buffers.
    MIXED_MODIFIES_INPUT = 0x0002,
    /// The field is available for inputs.
    /// 
    MIXED_IN             = 0x0001,
    /// The field is available for outputs.
    /// 
    MIXED_OUT            = 0x0002,
    /// The field is available for segments.
    /// 
    MIXED_SEGMENT        = 0x0004,
    /// The field can be set.
    /// 
    MIXED_SET            = 0x0008,
    /// The field can be get.
    /// 
    MIXED_GET            = 0x0010,
    /// This means that the segment's output buffers
    /// are cleared and overwritten on mix, losing all unread
    /// output.
    MIXED_CLEARS_OUTPUT  = 0x0020,
  };

  /// Convenience enum to map common speaker channels to buffer locations.
  ///
  /// It is not required that a segment follow this channel
  /// layout scheme, but it is heavily recommended.
  MIXED_EXPORT enum mixed_location{
    /// Signifies that the location is irrelevant and this is the only channel.
    MIXED_MONO = 0,
    /// The left speaker in a stereo setup.
    MIXED_LEFT = 0,
    /// The right speaker in a stereo setup.
    MIXED_RIGHT = 1,
    /// The middle left front speaker in a 5.1 or similar setup.
    MIXED_LEFT_FRONT = 0,
    /// The middle right front speaker in a 5.1 or similar setup.
    MIXED_RIGHT_FRONT = 1,
    /// The middle left rear speaker in a 5.1 or similar setup.
    MIXED_LEFT_REAR = 2,
    /// The middle right rear speaker in a 5.1 or similar setup.
    MIXED_RIGHT_REAR = 3,
    /// The middle front center speaker in a 5.1 or similar setup.
    MIXED_CENTER = 4,
    /// The subwoofer.
    MIXED_SUBWOOFER = 5,
    /// The bottom front left speaker.
    MIXED_LEFT_FRONT_BOTTOM = 0,
    /// The bottom front right speaker.
    MIXED_RIGHT_FRONT_BOTTOM = 1,
    /// The bottom rear left speaker.
    MIXED_LEFT_REAR_BOTTOM = 2,
    /// The bottom rear right speaker.
    MIXED_RIGHT_REAR_BOTTOM = 3,
    /// The front center speaker.
    MIXED_CENTER_FRONT = 4,
    /// The left side speaker.
    MIXED_LEFT_SIDE = 6,
    /// The rigth side speaker.
    MIXED_RIGHT_SIDE = 7,
    /// The top front left speaker.
    MIXED_LEFT_FRONT_TOP = 8,
    /// The top front right speaker.
    MIXED_RIGHT_FRONT_TOP = 9,
    /// The top rear left speaker.
    MIXED_LEFT_REAR_TOP = 10,
    /// The top rear right speaker.
    MIXED_RIGHT_REAR_TOP = 11,
    /// The rear center speaker.
    MIXED_CENTER_REAR = 12,
    MIXED_LEFT_SIDE_TOP = 13,
    MIXED_RIGHT_SIDE_TOP = 14,
    MIXED_SUBWOOFER_2 = 15,
    MIXED_LEFT_FRONT_WIDE = 16,
    MIXED_RIGHT_FRONT_WIDE = 17,
    MIXED_CENTER_TOP = 18,
    MIXED_CENTER_FRONT_TOP = 18,
    MIXED_CENTER_REAR_TOP = 19,
    MIXED_LEFT_REAR_CENTER = 20,
    MIXED_RIGHT_REAR_CENTER = 21,
    MIXED_SUBWOOFER_LEFT = 22,
    MIXED_SUBWOOFER_RIGHT = 23,
    MIXED_LEFT_FRONT_HIGH = 24,
    MIXED_RIGHT_FRONT_HIGH = 25,
    MIXED_CENTER_FRONT_HIGH = 26,
    MIXED_CENTER_BOTTOM = 27,
    MIXED_LEFT_CENTER_BOTTOM = 28,
    MIXED_RIGHT_CENTER_BOTTOM = 29,
    MIXED_LEFT_FRONT_CENTER_TOP = 31,
    MIXED_RIGHT_FRONT_CENTER_BOTTOM = 32
  };

  /// This enum holds type descriptors for the segment fields.
  /// It may also hold any encoding found in mixed_encoding.
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
    /// A general boolean type, with 0 being false and anything else true.
    /// Corresponds to int
    MIXED_BOOL = 11,
    /// An unsigned integer type.
    /// Corresponds to size_t (unsigned int of register size)
    MIXED_SIZE_T,
    /// A generic human-readable utf-8-encoded string type.
    /// Corresponds to char*
    MIXED_STRING,
    /// A pointer to a callable function.
    /// Corresponds to void*
    MIXED_FUNCTION,
    /// A pointer to an arbitrary, segment-defined type of memory.
    /// Corresponds to void*
    MIXED_POINTER,
    /// A pointer to a mixed_segment
    MIXED_SEGMENT_POINTER,
    /// A pointer to a mixed_buffer
    MIXED_BUFFER_POINTER,
    /// A pointer to a mixed_pack
    MIXED_PACK_POINTER,
    /// A pointer to a mixed_segment_sequence
    MIXED_SEGMENT_SEQUENCE_POINTER,
    /// An enum mixed_location
    MIXED_LOCATION_ENUM,
    /// An enum mixed_biquad_filter
    MIXED_BIQUAD_FILTER_ENUM,
    /// An enum mixed_repeat_mode
    MIXED_REPEAT_MODE_ENUM,
    /// An enum mixed_noise_type
    MIXED_NOISE_TYPE_ENUM,
    /// An enum mixed_generator_type
    MIXED_GENERATOR_TYPE_ENUM,
    /// An enum mixed_fade_type
    MIXED_FADE_TYPE_ENUM,
    /// An enum mixed_attenuation
    MIXED_ATTENUATION_ENUM,
    /// An enum mixed_encoding
    MIXED_ENCODING_ENUM,
    /// An enum mixed_error
    MIXED_ERROR_ENUM,
    /// An enum mixed_resample_type
    MIXED_RESAMPLE_TYPE_ENUM,
    /// A channel count
    /// Corresponds to mixed_channel_t
    MIXED_CHANNEL_T,
    /// A volume in dB
    /// Corresponds to mixed_decibel_t
    MIXED_DECIBEL_T,
    /// A duration in seconds
    /// Corresponds to mixed_duration_t
    MIXED_DURATION_T,
  };

  /// Type used for channel count descriptions.
  /// 
  typedef uint8_t mixed_channel_t;

  /// Type used for decibel descriptions.
  ///
  typedef float mixed_decibel_t;

  /// Type used for time duration descriptions in seconds.
  ///
  typedef float mixed_duration_t;

  /// An internal audio data buffer.
  ///
  /// The sample array is always stored in floats.
  /// You should /not/ touch any of these fields yourself,
  /// including the data array. Use the request functions to
  /// retrieve proper pointers into the array for read/write
  /// operations.
  MIXED_EXPORT struct mixed_buffer{
    float *_data;
    uint32_t size;
    uint32_t read;
    uint32_t write;
    uint32_t reserved;
    /// Whether the buffer owns the data array.
    /// 
    char is_virtual;
  };

  /// Information struct to encapsulate a "channel"
  ///
  /// Channels are representing external audio sources or
  /// drains. If you have a sound system or some kind of
  /// sound file reader, you will want to represent them
  /// using this struct.
  ///
  /// Using this struct you can then create a converter
  /// segment to include the external audio into the mix.
  MIXED_EXPORT struct mixed_pack{
    /// Bip buffer internals
    /// 
    unsigned char *_data;
    uint32_t size;
    uint32_t read;
    uint32_t write;
    uint32_t reserved;
    /// The sample encoding in the byte array.
    /// 
    enum mixed_encoding encoding;
    /// The number of channels that are packed into the array.
    /// 
    mixed_channel_t channels;
    /// The sample rate at which data is encoded in Hz.
    /// 
    uint32_t samplerate;
  };

  /// Metadata struct for a segment's field.
  ///
  /// This struct can be used to figure out what kind of
  /// fields are recognised on the segment and what you can
  /// do with it.
  MIXED_EXPORT struct mixed_segment_field_info{
    /// The actual field index. Fields should be in order
    /// in the fields array of the info struct, but don't
    /// have to be.
    /// 
    uint32_t field;
    /// A human-readable description of the data accessed.
    /// 
    const char *description;
    /// An OR-combination of flags that describe the field's
    /// properties, usually about whether it is valid for
    /// inputs, outputs, or the segment itself, and whether
    /// it can be set or get.
    enum mixed_segment_info_flags flags;
    /// The base element type of the value contained in the
    /// field. Note that while the get and set functions
    /// always take a pointer, the dereferenced value is
    /// interpreted as being of the type indicated by this
    /// field.
    enum mixed_segment_field_type type;
    /// In case the field stores an array of values, this
    /// defines the number of values that can be dererefenced.
    uint32_t type_count;
  };

  /// Metadata struct for a segment.
  ///
  /// This struct contains useful information that describes
  /// the behaviour and capabilities of the segment. You may
  /// want to look at this if you want to provide a generic
  /// interface to segments.
  MIXED_EXPORT struct mixed_segment_info{
    /// The name of the segment type as a string.
    /// Must be alphanumeric ASCII.
    const char *name;
    /// A human-readable description of the segment's purpose.
    /// Must be UTF-8 encoded.
    const char *description;
    /// An OR combination of segment info flags.
    /// 
    enum mixed_segment_info_flags flags;
    /// The minimal number of inputs that this segment requires.
    /// 
    uint32_t min_inputs;
    /// The maximal number of outputs that this segment can support.
    /// 
    uint32_t max_inputs;
    /// The number of outputs that this segment provides.
    /// 
    uint32_t outputs;
    /// A null-terminated array of possible fields that this
    /// segment supports. Note that while the struct definition here
    /// sets the number of fields to 32, an allocated segment info
    /// structure could potentially be larger and carry many more
    /// than 32. The only proper way to test for the last field
    /// is by testing for a null.
    struct mixed_segment_field_info fields[32];
  };

  /// The primary segment struct.
  ///
  /// Segments are what make up the mixing pipeline in libmixed.
  /// Each segment is responsible for performing some kind of audio
  /// operation like generating signals, outputting them, or
  /// transforming them in some way.
  ///
  /// Each segment should be created by first allocating an instance
  /// of this struct, zeroing it out, and then calling the
  /// appropriate "make" function to fill in the fields of the
  /// struct as required.
  ///
  /// A segment's method is considered "not implemented" if its
  /// struct field is set to zero.
  ///
  /// Each segment must implement the mix method, and at least
  /// either the set_in or set_out method. For the documentation
  /// of each of the methods, see the corresponding shorthand
  /// function below.
  MIXED_EXPORT struct mixed_segment{
    int (*free)(struct mixed_segment *segment);
    int (*info)(struct mixed_segment_info *info, struct mixed_segment *segment);
    int (*start)(struct mixed_segment *segment);
    int (*mix)(struct mixed_segment *segment);
    int (*end)(struct mixed_segment *segment);
    int (*set_in)(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);
    int (*set_out)(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);
    int (*get_in)(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);
    int (*get_out)(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);
    int (*set)(uint32_t field, void *value, struct mixed_segment *segment);
    int (*get)(uint32_t field, void *value, struct mixed_segment *segment);
    /// An opaque pointer to internal data for the segment.
    /// 
    void *data;
  };

  /// Allocate a new pack.
  ///
  /// You /must/ set the pack fields encoding and channels before this.
  /// The frames designates the number of frames that can be stored in
  /// the pack's data array. Meaning a total number of bytes of:
  ///   frames*channels*mixed_samplesize(encoding)
  /// 
  /// For the write and read functions, please see the analogous buffer
  /// functions.
  MIXED_EXPORT int mixed_make_pack(uint32_t frames, struct mixed_pack *pack);

  /// Free the pack
  /// See mixed_free_buffer
  MIXED_EXPORT void mixed_free_pack(struct mixed_pack *pack);

  /// Zero out the pack data
  /// See mixed_buffer_clear
  MIXED_EXPORT int mixed_pack_clear(struct mixed_pack *pack);

  /// Return the number of available write octets
  /// See mixed_buffer_available_write
  MIXED_EXPORT uint32_t mixed_pack_available_write(struct mixed_pack *pack);

  /// Return the number of available read octets
  /// See mixed_buffer_available_read
  MIXED_EXPORT uint32_t mixed_pack_available_read(struct mixed_pack *pack);

  /// Start a write operation
  /// See mixed_buffer_request_write
  MIXED_EXPORT int mixed_pack_request_write(void *restrict *area, uint32_t *size, struct mixed_pack *pack);

  /// Complete a write operation
  /// See mixed_buffer_finish_write
  MIXED_EXPORT int mixed_pack_finish_write(uint32_t size, struct mixed_pack *pack);

  /// Start a read operation
  /// See mixed_buffer_request_read
  MIXED_EXPORT int mixed_pack_request_read(void *restrict *area, uint32_t *size, struct mixed_pack *pack);

  /// Complete a free operation
  /// See mixed_buffer_finish_read
  MIXED_EXPORT int mixed_pack_finish_read(uint32_t size, struct mixed_pack *pack); 

  /// Allocate the buffer's internal storage array.
  ///
  MIXED_EXPORT int mixed_make_buffer(uint32_t size, struct mixed_buffer *buffer);

  /// Free the buffer's internal storage array.
  ///
  MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer);

  /// Convert the packed data to buffer data.
  ///
  /// This appropriately converts sample format and channel layout.
  /// You are responsible for passing in an array of buffers that is
  /// at least as long as the channel's channel count.
  /// The volume is a linear multiplier you can pass to adjust the
  /// volume in the resulting buffers.
  /// pack.frames should be set to the number of frames in the input
  /// pack, and will be set to the number of frames that have actually
  /// been read from the packed buffer. This may be less if the
  /// output buffers do not have enough space.
  MIXED_EXPORT int mixed_buffer_from_pack(struct mixed_pack *in, struct mixed_buffer **outs, float *volume, float target_volume);

  /// Convert buffer data to the packed data.
  ///
  /// This appropriately converts sample format and channel layout.
  /// You are responsible for passing in an array of buffers that is
  /// at least as long as the channel's channel count.
  /// The volume is a linear multiplier you can pass to adjust the
  /// volume in the resulting channel.
  /// pack.frames should be set to the number of frames in the output
  /// pack, and will be set to the number of frames that have actually
  /// been written to the pack. This may be less if the input buffers
  /// do not have enough data available.
  MIXED_EXPORT int mixed_buffer_to_pack(struct mixed_buffer **ins, struct mixed_pack *out, float *volume, float target_volume);

  /// Transfers data from one buffer to the other.
  ///
  /// This is equivalent to requesting a read from the from buffer,
  /// and a write from the to buffer, then memcpying as much as
  /// possible over, and finally committing the read/write ops.
  /// Note that doing this will effectively remove all transferred
  /// data from the from buffer.
  MIXED_EXPORT int mixed_buffer_transfer(struct mixed_buffer *from, struct mixed_buffer *to);

  /// Copies data from one buffer to the other.
  ///
  /// This is equivalent to requesting a read from the from buffer,
  /// and a write from the to buffer, then memcyping as much as
  /// possible over, and finally committing the write.
  /// Note that this will /not/ remove the transferred data from
  /// the from buffer and it is up to you to finish the read and
  /// decommission the buffered data whenever appropriate.
  MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to);

  /// Clear the buffer to make it empty again.
  /// 
  /// This clears the entire buffer to hold samples of all zeroes.
  MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer);

  /// Returns the number of floats available for writing in the buffer.
  /// 
  MIXED_EXPORT uint32_t mixed_buffer_available_write(struct mixed_buffer *buffer);

  /// Returns the number of floats available for reading in the buffer.
  /// 
  MIXED_EXPORT uint32_t mixed_buffer_available_read(struct mixed_buffer *buffer);

  /// Reserve a block of memory for a write operation.
  ///
  /// size must contain the requested size of memory to reserve.
  /// If successful, area will be set to the start of the reserved
  /// memory area, ready for write. size will be set to the actual
  /// size of the allocated memory area. writing beyond the address
  /// of area+size is illegal.
  ///
  /// This operation will fail if there is no more free memory or
  /// if a previously reserved block has not yet been committed.
  /// In the case of a failure, size will be set to zero and area
  /// will be left untouched.
  ///
  /// It is safe to have one thread write to a buffer while another
  /// thread is reading from the buffer. It is however /NOT/ safe
  /// to write from multiple threads or read from multiple threads
  /// at the same time.
  MIXED_EXPORT int mixed_buffer_request_write(float *restrict *area, uint32_t *size, struct mixed_buffer *buffer);

  /// Commit a reserved block after writing to it.
  ///
  /// Attempting to commit more than has been reserved will fail.
  /// Committing less than was reserved will commit that amount and
  /// release the rest.
  /// Using the previously obtained area pointer after finishing a
  /// write is illegal.
  MIXED_EXPORT int mixed_buffer_finish_write(uint32_t size, struct mixed_buffer *buffer);

  /// Retrieve a memory block for reading.
  ///
  /// Stores the start of the block in area and the minimum between
  /// the passed value of size and the available number of samples
  /// to be read in size. If you would like to get the maximal
  /// available read size, set size to UINT32_MAX first.
  /// Reading beyond area+size is illegal.
  /// If no block has been written to, this operation will fail.
  /// In the case of a failure, size will be set to zero, and area
  /// will be left untouched.
  MIXED_EXPORT int mixed_buffer_request_read(float *restrict *area, uint32_t *size, struct mixed_buffer *buffer);

  /// Free part of a block after reading from it.
  ///
  /// This operation will fail if there is no block to free, or
  /// if the amount of space to free is more than is committed.
  /// Using the previously obtained area pointer after finishing a
  /// read is illegal.
  MIXED_EXPORT int mixed_buffer_finish_read(uint32_t size, struct mixed_buffer *buffer);

  /// Resize the buffer to a new size.
  ///
  /// If the resizing operation fails due to a lack of memory, the
  /// old data is preserved and the buffer is not changed.
  MIXED_EXPORT int mixed_buffer_resize(uint32_t size, struct mixed_buffer *buffer);

  /// Convenience macro for the common operation of transferring
  /// from one buffer to another.
  ///
  /// i, samples, inv, and outv should be variable names.
  /// in, out should be expressions for pointers to mixed_buffer
  /// structures.
  /// body should be a statement that is executed for every
  /// sample that should be transferred.
#define with_mixed_buffer_transfer(i, samples, inv, in, outv, out, body){ \
    uint32_t samples = UINT32_MAX;                                      \
    struct mixed_buffer *__in = in;                                     \
    struct mixed_buffer *__out = out;                                   \
    float *restrict inv, *restrict outv;                                \
    if(__in == __out){                                                  \
      mixed_buffer_request_read(&inv, &samples, __in);                  \
      outv = inv;                                                       \
      for(uint32_t i=0; i<samples; ++i)                                 \
        body;                                                           \
    }else{                                                              \
      mixed_buffer_request_read(&inv, &samples, __in);                  \
      mixed_buffer_request_write(&outv, &samples, __out);               \
      for(uint32_t i=0; i<samples; ++i)                                 \
        body;                                                           \
      mixed_buffer_finish_read(samples, __in);                          \
      mixed_buffer_finish_write(samples, __out);                        \
    }                                                                   \
  }

  /// Free the segment's internal data.
  ///
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  ///
  /// After this function has been run on a segment, it becomes
  /// useless and must be discarded. Using any other segment
  /// method after it has been freed results in undefined
  /// behaviour.
  MIXED_EXPORT int mixed_free_segment(struct mixed_segment *segment);

  /// Start the segment's mixing process.
  ///
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_start(struct mixed_segment *segment);

  /// Run the segment.
  ///
  /// If this returns zero, then the segment did not produce any
  /// output and can't be expected to produce any until it is
  /// restarted. This happens if the segment is some kind of
  /// finite source and has ended, or if an internal error
  /// occurred that prevents the segment from operating.
  MIXED_EXPORT int mixed_segment_mix(struct mixed_segment *segment);

  /// End the segment's mixing process.
  ///
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_end(struct mixed_segment *segment);

  /// Set the value of an input buffer field.
  ///
  /// If the location does not designate a valid input buffer, the
  /// error code is set to MIXED_INVALID_LOCATION.
  /// If implemented, at least the field MIXED_BUFFER must be
  /// recognised.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  ///
  /// See mixed_segment_set
  MIXED_EXPORT int mixed_segment_set_in(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);

  /// Set the value of an output buffer field.
  ///
  /// If the location does not designate a valid output buffer, the
  /// error code is set to MIXED_INVALID_LOCATION.
  /// If implemented, at least the field MIXED_BUFFER must be
  /// recognised.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  ///
  /// See mixed_segment_set
  MIXED_EXPORT int mixed_segment_set_out(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);

  /// Get the value of an input buffer field.
  ///
  /// If the location does not designate a valid input buffer, the
  /// error code is set to MIXED_INVALID_LOCATION.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  ///
  /// See mixed_segment_get
  MIXED_EXPORT int mixed_segment_get_in(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);

  /// Get the value of an output buffer field.
  /// 
  /// If the location does not designate a valid output buffer, the
  /// error code is set to MIXED_INVALID_LOCATION.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  /// 
  /// See mixed_segment_get.
  MIXED_EXPORT int mixed_segment_get_out(uint32_t field, uint32_t location, void *value, struct mixed_segment *segment);

  /// Return a pointer to a struct containing general information about the segment.
  /// 
  /// This is mostly useful for introspective / reflective purposes.
  /// You must free the returned struct yourself.
  /// 
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED and a null pointer is returned.
  MIXED_EXPORT int mixed_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment);

  /// Set the value of a field in the segment.
  /// 
  /// The value must be a pointer to a place that can be
  /// resolved to the data that the field should be set to. It is
  /// your responsibility to make sure that the value contains the
  /// appropriate data as expected by the segment. Consult the
  /// documentation of  the individual fields and segments.
  /// 
  /// If the field is not recognised, the error is set to
  /// MIXED_INVALID_FIELD.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_set(uint32_t field, void *value, struct mixed_segment *segment);

  /// Get the value of a field in the segment.
  /// 
  /// The value must be a pointer to a place that can be set to
  /// the data that is fetched from the field in the segment.
  /// It is your responsibility to make sure that there is enough
  /// space available. Consult the documentation of the individual
  /// fields and segments.
  /// 
  /// If the field is not recognised, the error is set to
  /// MIXED_INVALID_FIELD.
  /// If the method is not implemented, the error is set to
  /// MIXED_NOT_IMPLEMENTED.
  MIXED_EXPORT int mixed_segment_get(uint32_t field, void *value, struct mixed_segment *segment);

  /// An audio unpacker.
  ///
  /// This segment convers the data from the channel's data
  /// array to the appropriate format as expected by the internal
  /// mixing buffers. This involves channel layout separation and
  /// sample format conversion.
  ///
  /// The sample rate given denotes the target sample rate of the
  /// buffers connected to the outputs of this segment. The source
  /// sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_unpacker(struct mixed_pack *packed, uint32_t samplerate, struct mixed_segment *segment);

  /// An audio packer.
  /// 
  /// This segment converts the data from a number of buffers to
  /// the appropriate format as expected by the given channel.
  /// This involves channel layout packing and sample format
  /// conversion.
  /// 
  /// The sample rate given denotes the source sample rate of the
  /// buffers connected to the inputs of this segment. The target
  /// sample rate is the sample rate stored in the channel.
  MIXED_EXPORT int mixed_make_segment_packer(struct mixed_pack *packed, uint32_t samplerate, struct mixed_segment *segment);

  /// A basic, additive mixer
  /// 
  /// This segment simply linearly mixes every input together into
  /// output buffers. Each source must connect as many inputs as
  /// there are channels. This means that if you have a channel
  /// count of two and connect two sources A and B, the input
  /// buffers must be connected as follows: A⁰ A¹ B⁰ B¹
  /// The buffers must be set in a monotonically increasing order,
  /// meaning you cannot leave gaps in the input location.
  /// If you set a buffer to 0 it is removed and the buffers above
  /// it are shifted downwards.
  /// 
  /// Depending on how many sources you have, adding and removing
  /// sources at random locations may prove expensive. Especially
  /// adding more sources might involve allocations, which may not
  /// be suitable for real-time behaviour. Aside from this caveat,
  /// sources can be added or changed at any point in time.
  MIXED_EXPORT int mixed_make_segment_basic_mixer(mixed_channel_t channels, struct mixed_segment *segment);

  /// A dynamic compressor
  /// 
  MIXED_EXPORT int mixed_make_segment_compressor(uint32_t samplerate, struct mixed_segment *segment);

  /// A very basic volume control segment
  /// 
  /// This segment can be used to regulate the volume and pan of the
  /// source. It is a strictly stereo segment and as such needs two
  /// inputs and two outputs.
  ///
  /// The volume is a linear multiplier.
  /// 
  /// The fields of this segment may be changed at any time.
  MIXED_EXPORT int mixed_make_segment_volume_control(float volume, float pan, struct mixed_segment *segment);

  /// A volume fade in/out effect
  /// 
  /// If you need to smoothly fade in a track, this segment is for you.
  /// The fade type determines the easing function used to fade the
  /// input in.
  /// 
  /// The segment takes an arbitrary number of inputs and outputs, but
  /// the number of them must be matched up, as each input will simply
  /// be faded to the output at the same index.
  /// 
  /// The fields of this segment may be changed at any time on the fly,
  /// but there will not be any smooth transitioning between them and
  /// there is thus little reason to ever do this, aside from perhaps
  /// re-using the same segment to fade the input out again at the
  /// appropriate moment.
  MIXED_EXPORT int mixed_make_segment_fade(float from, float to, float time, enum mixed_fade_type type, uint32_t samplerate, struct mixed_segment *segment);

  /// A simple wave generator source
  /// 
  /// This segment simply produces a constant signal of the requested
  /// frequency and wave form type. You may change the frequency and
  /// wave form type at any time. Potentially this could be used to
  /// create a very primitive synthesizer.
  MIXED_EXPORT int mixed_make_segment_generator(enum mixed_generator_type type, uint32_t frequency, uint32_t samplerate, struct mixed_segment *segment);

  /// A LADSPA plugin segment
  /// 
  /// LADSPA (Linux Audio Developers Simple Plugin API) is a standard for
  /// sound plugins, effects, and generators. You can find a lot of plugins
  /// for direct use to download on the internet. Using this segment, you
  /// can then integrate such a plugin into your mixer pipeline.
  /// 
  /// Note that while libmixed makes a hard distinction between fields and
  /// input and output buffers, LADSPA does not. A LADSPA plugin merely has
  /// a list of "ports", each of which can be an input or an output, and
  /// a control or an audio port. All input audio ports are mapped to the
  /// set_in function, starting from 0 and increasing by one for each port.
  /// The same goes for output audio ports, except naturally being mapped on
  /// set_out. The control ports are mapped to set/get respectively. In
  /// essence what this means is that the following list of ports of a
  /// LADSPA plugin
  ///
  /// * 0 in audio (mono in)
  /// * 1 out audio (mono out)
  /// * 2 in control (pitch)
  /// * 3 in control (rate)
  ///
  /// Would be mapped to the libmixed API as follows:
  ///
  /// * in MIXED_BUFFER 0
  /// * out MIXED_BUFFER 0
  /// * set 0
  /// * set 1
  ///
  /// So each "category" of port receives its own counting scheme.
  ///
  /// Note that a lot of plugins expect their control ports to be set to
  /// some kind of value beforehand. Libmixed will bind all of them and
  /// default their values to zero. It is your responsibility to set them
  /// to the desired, proper values. Also note that all fields for this
  /// plugin expect pointers to floats for their values. Thus, setting a
  /// field will usually look like this:
  ///
  ///   float value = 1.0;
  ///   mixed_segment_set(0, &value, &segment);
  ///
  /// This segment does not expressly allow you to change fields and
  /// buffers while mixing has already been started. Generally, LADSPA
  /// requires you to stop mixing before being able to change any property
  /// but some plugins may nevertheless work despite that. Thus, consult
  /// your plugin's source or documentation.
  MIXED_EXPORT int mixed_make_segment_ladspa(const char *file, uint32_t index, uint32_t samplerate, struct mixed_segment *segment);

  /// A space (3D) processed mixer
  ///
  /// This segment is capable of mixing sources according to their position
  /// and movement in space. It thus simulates the behaviour of sound in a
  /// 3D environment. This segment takes an arbitrary number of mono inputs
  /// and has two outputs (left and right). Each input has two additional
  /// fields aside from the buffer:
  ///
  /// * MIXED_SPACE_LOCATION
  /// * MIXED_SPACE_VELOCITY
  ///
  /// The position, velocity, and general properties of the space mixing
  /// are configured through the general field set/get functions. The
  /// following flags are understood:
  ///
  /// * MIXED_SPACE_LOCATION
  /// * MIXED_SPACE_VELOCITY
  /// * MIXED_SPACE_DIRECTION
  /// * MIXED_SPACE_UP
  /// * MIXED_SPACE_SOUNDSPEED
  /// * MIXED_SPACE_DOPPLER_FACTOR
  /// * MIXED_SPACE_MIN_DISTANCE
  /// * MIXED_SPACE_MAX_DISTANCE
  /// * MIXED_SPACE_ROLLOFF
  /// * MIXED_SPACE_ATTENUATION
  ///
  /// See the MIXED_FIELDS enum for the documentation of each field.
  /// This segment does allow you to change fields and buffers while the
  /// mixing has already been started.
  MIXED_EXPORT int mixed_make_segment_space_mixer(uint32_t samplerate, struct mixed_segment *segment);

  /// A planar (2D) processed mixer
  ///
  /// This segment is capable of mixing sources according to their position
  /// and movement in space. It thus simulates the behaviour of sound in a
  /// 2D environment. This segment takes an arbitrary number of mono inputs
  /// and has two outputs (left and right). Each input has two additional
  /// fields aside from the buffer:
  ///
  /// * MIXED_PLANE_LOCATION
  /// * MIXED_PLANE_VELOCITY
  ///
  /// The position, velocity, and general properties of the space mixing
  /// are configured through the general field set/get functions. The
  /// following flags are understood:
  ///
  /// * MIXED_PLANE_LOCATION
  /// * MIXED_PLANE_VELOCITY
  /// * MIXED_SPACE_SOUNDSPEED
  /// * MIXED_SPACE_DOPPLER_FACTOR
  /// * MIXED_SPACE_MIN_DISTANCE
  /// * MIXED_SPACE_MAX_DISTANCE
  /// * MIXED_SPACE_ROLLOFF
  /// * MIXED_SPACE_ATTENUATION
  ///
  /// See the MIXED_FIELDS enum for the documentation of each field.
  /// This segment does allow you to change fields and buffers while the
  /// mixing has already been started.
  MIXED_EXPORT int mixed_make_segment_plane_mixer(uint32_t samplerate, struct mixed_segment *segment);

  /// A delay segment
  ///
  /// This segment will simply delay the incoming samples to the output by
  /// a specified amount of time. To do this, it will keep an internal
  /// buffer of time*samplerate samples to store the incoming samples before
  /// outputting them again once the delay time has been passed.
  ///
  /// This buffering means that delaying for a long time may take a lot of
  /// memory, so watch out for that.
  MIXED_EXPORT int mixed_make_segment_delay(float time, uint32_t samplerate, struct mixed_segment *segment);

  /// A repeat segment
  ///
  /// This segment allows you to repeat some input from a buffer, and then
  /// simply loops the recorded bit to its output continuously. During the
  /// recording time, the input is simply passed through.
  ///
  /// Since the repeated audio data needs to be recorded, a long repeat time
  /// may take a lot of memory.
  MIXED_EXPORT int mixed_make_segment_repeat(float time, uint32_t samplerate, struct mixed_segment *segment);

  /// A pitch shift segment
  ///
  /// This segment will shift the pitch of the incoming samples by a specified
  /// amount. The pitch should be a float in the range ]0, infty[, where 1.0
  /// means no change in pitch, 0.5 means half the pitch, 2.0 means double the
  /// pitch and so on.
  MIXED_EXPORT int mixed_make_segment_pitch(float pitch, uint32_t samplerate, struct mixed_segment *segment);

  /// A noise gate segment
  ///
  /// The gate has an open threshold, which if exceeded opens the gate. Audio
  /// does not immediately pass through the gate upon opening though. It first
  /// goes through an attack time during which the volume is linearly scaled up.
  /// If the volume then ever goes below the close threshold, the gate stays
  /// open for the duration of the hold time, after which it goes through a
  /// linear fade out for the duration of the release time.
  MIXED_EXPORT int mixed_make_segment_gate(uint32_t samplerate, struct mixed_segment *segment);

  /// A noise generator segment.
  ///
  /// This segment can generate white, pink, and brown noise.
  MIXED_EXPORT int mixed_make_segment_noise(enum mixed_noise_type type, struct mixed_segment *segment);

  /// A biquad filter segment.
  ///
  /// This can implement a variety of different filter effects that are attuned
  /// to a certain frequency response. See the mixed_biquad_filter enum for 
  /// possible types.
  ///
  /// The frequency cannot be larger than the samplerate. Generally, if the frequency
  /// frequency is larger than half of the samplerate, major distortion will
  /// occur, so tread carefully.
  MIXED_EXPORT int mixed_make_segment_biquad_filter(enum mixed_biquad_filter type, float frequency, uint32_t samplerate, struct mixed_segment *segment);

  /// A speed change segment.
  ///
  /// Speed should be a factor, with 1.0 being 'same speed'. Higher factors will
  /// speed it up, lower factors will slow it down. This is achieved by
  /// resampling the audio data.
  MIXED_EXPORT int mixed_make_segment_speed_change(double speed, struct mixed_segment *segment);

  /// A segment to distribute a buffer to multiple consumers.
  ///
  /// You should connect the buffer to duplicate as an input, and connect as many
  /// copies as you need as outputs. Note however that the output buffers must
  /// not be allocated, and should instead be empty structs. The segment will
  /// handle initialisation.
  MIXED_EXPORT int mixed_make_segment_distribute(struct mixed_segment *segment);

  /// A segment to perform a continuous Fast Fourier Transform.
  ///
  /// Unlike the raw mixed_fwd_fft/inv_fft functions, these segments perform the
  /// necessary input windowing logic to properly process continuous streams of
  /// samples.
  /// 
  /// For the FWD segment, the output buffer will be filled with framesize number
  /// of values, with interleaved frequency and magnitude bins, meaning there will
  /// be framesize/2 frequency bins. If processing of the output buffer is too
  /// slow, multiple frames may be written into the output buffer in one mixing step.
  /// 
  /// In order to control the framesize, use the MIXED_FRAMESIZE field.
  MIXED_EXPORT int mixed_make_segment_fwd_fft(uint32_t samplerate, struct mixed_segment *segment);

  /// A segment to perform a continuous Fast Fourier Transform.
  ///
  /// Unlike the raw mixed_fwd_fft/inv_fft functions, these segments perform the
  /// necessary input windowing logic to properly process continuous streams of
  /// samples.
  /// 
  /// For the INV segment, the input buffer must be filled with framesize number
  /// of values, with interleaved frequency and magnitude bins.
  /// 
  /// In order to control the framesize, use the MIXED_FRAMESIZE field.
  MIXED_EXPORT int mixed_make_segment_inv_fft(uint32_t samplerate, struct mixed_segment *segment);

  /// A queue segment for inner segments.
  ///
  /// The queue will delegate mixing to the first segment in its list until that
  /// segment is explicitly removed, or returns zero from its mix function.
  /// When a new segment is added to the queue, the queue will connect as many
  /// of its buffers as appropriate. When the segment is removed, the buffers
  /// are also unset.
  ///
  /// If the queue is empty or is being bypassed, it will simply copy the
  /// contents of the input buffers to corresponding output buffers. If there are
  /// more output buffers than input buffers, the remaining are cleared to zero.
  ///
  /// The number of bufers that can be connected to the queue is controlled by
  /// the MIXED_IN_COUNT and MIXED_OUT_COUNT fields.
  ///
  /// The queue's info will reflect the capabilities of the first segment, if any,
  /// and the queue's maximal capabilities otherwise.
  MIXED_EXPORT int mixed_make_segment_queue(struct mixed_segment *segment);

  /// Add a segment to the queue to be played back.
  /// 
  /// The segment is added to the end of the queue and will only start playback
  /// if there are no other segments in front of it.
  MIXED_EXPORT int mixed_queue_add(struct mixed_segment *news, struct mixed_segment *queue);
  
  /// Remove a segment from the queue.
  ///
  /// This will remove the segment regardless of its position in the queue.
  MIXED_EXPORT int mixed_queue_remove(struct mixed_segment *old, struct mixed_segment *queue);

  /// Remove the segment at the given position from the queue.
  ///
  /// Note that a segment's position within the queue changes as segments are
  /// removed from the queue.
  MIXED_EXPORT int mixed_queue_remove_at(uint32_t pos, struct mixed_segment *queue);

  /// Remove all segments from the queue.
  MIXED_EXPORT int mixed_queue_clear(struct mixed_segment *queue);

  /// A segment that throws away all of its input.
  /// 
  MIXED_EXPORT int mixed_make_segment_void(struct mixed_segment *segment);

  /// A segment that simply produces silence.
  /// 
  MIXED_EXPORT int mixed_make_segment_zero(struct mixed_segment *segment);

  /// A segment for channel conversion
  ///
  /// This segment allows changing between different channel representations.
  /// If a requested configuration is not supported, an error is created.
  MIXED_EXPORT int mixed_make_segment_channel_convert(uint8_t in, uint8_t out, uint32_t samplerate, struct mixed_segment *segment);

  /// Create a chain segment
  ///
  /// A chain segment is useful for operating on many segments at once.
  MIXED_EXPORT int mixed_make_segment_chain(struct mixed_segment *segment);

  /// A segment to quantize the amplitude
  ///
  /// The signal will be quantized into STEPS number of discrete amplitudes.
  MIXED_EXPORT int mixed_make_segment_quantize(uint32_t steps, struct mixed_segment *segment);

  /// Add a new segment to the chain's end.
  ///
  /// It is /NOT/ safe to add elements to a chain from multiple
  /// threads at once.
  MIXED_EXPORT int mixed_chain_add(struct mixed_segment *segment, struct mixed_segment *chain);

  /// Add a new segment at the specified index in the chain.
  /// elements at or after the index are pushed back to make space.
  /// It is /NOT/ safe to add elements to a chain from multiple
  /// threads at once.
  MIXED_EXPORT int mixed_chain_add_at(uint32_t i, struct mixed_segment *segment, struct mixed_segment *chain);

  /// Remove a segment from the chain.
  ///
  /// Segments after it will be shifted down as necessary.
  ///
  /// It is /NOT/ safe to remove elements from a chain from multiple
  /// threads at once.
  MIXED_EXPORT int mixed_chain_remove(struct mixed_segment *segment, struct mixed_segment *chain);

  /// Remove a segment at the specified index from the chain.
  /// 
  /// Segments after it will be shifted down as necessary.
  /// 
  /// It is /NOT/ safe to remove elements from a chain from multiple
  /// threads at once.
  MIXED_EXPORT int mixed_chain_remove_at(uint32_t i, struct mixed_segment *chain);

  /// A segment for finite input response (FIR) convolution filtering.
  ///
  /// The fir should be the raw input response signal of the channel to
  /// process, with fir_size being the number of elements in the FIR.
  /// The fir array may be freed after the call to this function.
  ///
  /// framesize must be a power of two between [2^1, 2^13] and designates
  /// the size of the FFT frames that are processed. Larger frames lead to
  /// better quality at the cost of greater latency. 2048 should be a good
  /// default.
  MIXED_EXPORT int mixed_make_segment_convolution(uint16_t framesize, float *fir, uint32_t fir_size, uint32_t samplerate, struct mixed_segment *segment);

  /// Function prototype for a plugin's segment construction function.
  ///
  /// This type of function will be called with an opaque argument list
  /// That the function should untangle to then call the proper constructor.
  typedef int (*mixed_make_segment_function)(void *args, struct mixed_segment *segment);

  /// Function prototype for a plugin's segment type registration function.
  ///
  /// The plugin should call this function to describe its segment names and
  /// construction argument types, along with the actual function to call.
  typedef int (*mixed_register_segment_function)(const char *name, uint32_t argc, struct mixed_segment_field_info *args, mixed_make_segment_function function);

  /// Function prototype for a plugin's segment type deregistration function.
  ///
  /// The plugin should call this to deregister a segment it previously registered.
  typedef int (*mixed_deregister_segment_function)(const char *name);

  /// If you write a segment plugin library, you must define an
  /// exported function of this signature named mixed_make_plugin.
  /// The function should call the given function for every
  /// segment that the library should provide.
  typedef int (*mixed_make_plugin_function)(mixed_register_segment_function registrar);

  /// If you write a segment plugin library, you must define an
  /// exported function of this signature named mixed_make_plugin.
  /// The function should call the given function for every
  /// segment that it initially registered.
  typedef int (*mixed_free_plugin_function)(mixed_deregister_segment_function registrar);

  /// Load a new plugin library.
  ///
  /// The function may fail if the library was previously loaded already,
  /// cannot be opened, does not contain the required mixed_make_plugin
  /// function, or that function fails for some reason.
  /// The file name is copied and may be deallocated again after this
  /// function has been called.
  MIXED_EXPORT int mixed_load_plugin(const char *file);

  /// Close an existing plugin library.
  ///
  /// This function may fail if the requested library was not loaded before,
  /// it does not contain the required mixed_free_plugin function, or that
  /// function fails for some reason.
  MIXED_EXPORT int mixed_close_plugin(const char *file);

  /// The maximum number of arguments that can be passed to a segment constructor
#define MIXED_MAX_MAKE_ARG_COUNT 32
  /// The maximum number of segment types that can be registered globally
#define MIXED_MAX_SEGMENT_COUNT 1024
  /// The maximum length of a segment's name
#define MIXED_MAX_SEGMENT_NAME_LENGTH 64

  /// Register a segment constructor globally.
  ///
  /// If successful, the segment constructor information is later retrievable
  /// through miked_make_segment_info, and the segment can be constructed
  /// via mixed_make_segment.
  /// The name and args are copied and may be deallocated again after this
  /// function has been called.
  /// If argc is longer than MIXED_MAX_MAKE_ARG_COUNT, MIXED_BAD_ARGUMENT_COUNT
  /// is errored.
  /// If the name is longer than MIXED_MAX_SEGMENT_NAME_LENGTH, MIXED_BAD_NAME
  /// is errored.
  /// If there are no more free segments available to register, MIXED_OUT_OF_MEMORY
  /// is errored.
  MIXED_EXPORT int mixed_register_segment(const char *name, uint32_t argc, struct mixed_segment_field_info *args, mixed_make_segment_function function);

  /// Remove a globally registered segment constructor.
  ///
  /// If successful, the given name can be registered again afterwards.
  MIXED_EXPORT int mixed_deregister_segment(const char *name);

  /// List available segments.
  ///
  /// If names is NULL, only count is updated. This allows you to allocate
  /// the correct size of array for the names.
  MIXED_EXPORT int mixed_list_segments(uint32_t *count, char **names);

  /// Retrieve information about the segment's contsructor function.
  ///
  /// argc will be set to the number of required arguments, and
  /// args will be set to an array of segment_field_info structures of
  /// that size describing the required arguments.
  MIXED_EXPORT int mixed_make_segment_info(const char *name, uint32_t *argc, const struct mixed_segment_field_info **args);

  /// Create a segment by name.
  ///
  /// args must be an array of pointers, where each pointer in the array
  /// points to a value of the type as described in the respective
  /// segment_field_info structure for the constructor.
  MIXED_EXPORT int mixed_make_segment(const char *name, void *args, struct mixed_segment *segment);

  /// Return the size of a sample in the given encoding in bytes.
  /// 
  MIXED_EXPORT uint8_t mixed_samplesize(enum mixed_encoding encoding);

  /// Return the number of bytes between two samples in a byte stream.
  /// 
  MIXED_EXPORT uint8_t mixed_byte_stride(mixed_channel_t channels, enum mixed_encoding encoding);
  
  /// A function to decode a packed sample array to the standardised float buffer format.
  /// The stride is the number of bytes between two samples in the input array.
  /// The samples is the number of samples to transfer.
  /// The volume should be the starting volume at the beginning of the transfer.
  /// The target_volume should be the volume to reach for. The function will smoothly
  /// transition to this volume as it processes, in order to avoid harsh volume changes
  /// that could introduce clicks into the audio. Once the function finishes, it returns
  /// the current volume that was reached.
  /// The volumes are linear, not in decibel.
  typedef float (*mixed_transfer_function_from)(void *in, float *out, uint8_t stride, uint32_t samples, float volume, float target_volume);

  /// A function to encode a standardised float sample buffer to a packed array format.
  /// The stride is the number of bytes between two samples in the output array.
  /// The samples is the number of samples to transfer.
  /// The volume should be the starting volume at the beginning of the transfer.
  /// The target_volume should be the volume to reach for. The function will smoothly
  /// transition to this volume as it processes, in order to avoid harsh volume changes
  /// that could introduce clicks into the audio. Once the function finishes, it returns
  /// the current volume that was reached.
  /// The volumes are linear, not in decibel.
  typedef float (*mixed_transfer_function_to)(float *in, void *out, uint8_t stride, uint32_t samples, float volume, float target_volume);

  /// Retrieve a sample format converter function that converts from a type to floats.
  /// See mixed_transfer_function_from
  ///
  MIXED_EXPORT mixed_transfer_function_from mixed_translator_from(enum mixed_encoding encoding);

  /// Retrieve a sample format converter function that converts from floats to a type.
  /// See mixed_transfer_function_to
  ///
  MIXED_EXPORT mixed_transfer_function_to mixed_translator_to(enum mixed_encoding encoding);

  /// Perform a fast fourier forward transform on a buffer of samples.
  ///
  /// framesize must be a power of two between [2^1, 2^13]
  /// in and out may be the same buffers, both with framesize number of
  /// elements. The output buffer will contain framesize/2 frequency bins
  /// as interleaved real and imaginary parts: [real, imag, real, imag, ...]
  MIXED_EXPORT int mixed_fwd_fft(uint16_t framesize, float *in, float *out);

  /// Performa a fast fourier inverse transform on a buffer of samples.
  ///
  /// framesize must be a power of two between [2^1, 2^13]
  /// in and out may be the same buffers, both with framesize number of
  /// elements. The input buffer must contain framesize/2 frequency bins
  /// as interleaved real and imaginary parts: [real, imag, real, imag, ...]
  MIXED_EXPORT int mixed_inv_fft(uint16_t framesize, float *in, float *out);

  /// Converts decibel to a linear volume multiplier.
  __attribute__((always_inline)) MIXED_EXPORT inline float mixed_from_db(mixed_decibel_t volume){
    return powf(10.0f, 0.05f * volume);
  }

  /// Converts a linear volume multiplier to decibel.
  __attribute__((always_inline)) MIXED_EXPORT inline mixed_decibel_t mixed_to_db(float volume){
    return 20.0f * log10f(volume);
  }

  /// Return the current error code.
  ///
  /// The error code is thread-local in order to allow multiple operations
  /// at the same time.
  MIXED_EXPORT int mixed_error(void);

  /// Return the ASCII error string for the given error code.
  ///
  /// If the error code is less than zero, the error string for the
  /// error code returned by mixed_error(); is returned instead.
  MIXED_EXPORT const char *mixed_error_string(int error_code);

  /// Return the ASCII textual description of the given type identifier.
  ///
  MIXED_EXPORT const char *mixed_type_string(int code);

  /// Returns the ASCII version string of the library.
  ///
  MIXED_EXPORT const char *mixed_version(void);

  //// Allow customising how libmixed allocates things.
#ifdef MIXED_NO_CUSTOM_ALLOCATOR
#define mixed_calloc calloc
#define mixed_free free
#define mixed_realloc realloc
#else
  /// Allocates a new block of memory.
  /// 
  /// mixed_calloc must return either 0 or a pointer to a memory region
  /// that is num*size octets large. It may only return 0 if the allocation
  /// failed for some reason, such as an out of memory condition.
  /// The returned memory region must be cleared, meaning all addressable
  /// octets within the region must be 0. The memory region also must not
  /// overlap with any other allocated memory regions.
  MIXED_EXPORT extern void *(*mixed_calloc)(size_t num, size_t size);

  /// Releases a previously allocated region of memory.
  ///
  /// The behaviour is undefined if a pointer is passed to this function
  /// that was not returned by a prior call to mixed_calloc, or if the pointer
  /// was already passed to mixed_free before being returned by another call
  /// to mixed_calloc.
  MIXED_EXPORT extern void (*mixed_free)(void *ptr);

  /// Resizes the allocated block of memory.
  ///
  /// Must return a pointer to a memory region that is size octets large.
  /// It may return 0 if the allocation failed for some reason, such as an
  /// out of memory condition. The returned memory region must contain the
  /// same octets as the passed ptr region, up to min(size, old_size).
  /// The content outside of this range is unspecified.
  /// The returned pointer may be the same as the one passed in.
  /// If the returned pointer is not the same as the one passed in, the
  /// behaviour should be as if the passed pointer had been passed to
  /// mixed_free.
  /// If the realloc operation fails and zero is returned, the original
  /// pointer must still be allocated and its memory intact.
  MIXED_EXPORT extern void *(*mixed_realloc)(void *ptr, size_t size);
#endif

#ifdef __cplusplus
}
#endif
#endif
