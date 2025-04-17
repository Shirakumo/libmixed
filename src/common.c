#ifdef _WIN32
#  include <windows.h>
#elif MIXED_DL
#  include <dlfcn.h>
#endif
#ifndef MIXED_VERSION
#  define MIXED_VERSION "unknown"
#endif
#include "internal.h"
#include "spiral_fft.h"

MIXED_EXPORT uint8_t mixed_samplesize(enum mixed_encoding encoding){
  switch(encoding){
  case MIXED_INT8: return 1;
  case MIXED_UINT8: return 1;
  case MIXED_INT16: return 2;
  case MIXED_UINT16: return 2;
  case MIXED_INT24: return 3;
  case MIXED_UINT24: return 3;
  case MIXED_INT32: return 4;
  case MIXED_UINT32: return 4;
  case MIXED_FLOAT: return 4;
  case MIXED_DOUBLE: return 8;
  default: return -1;
  }
}

MIXED_EXPORT uint8_t mixed_byte_stride(mixed_channel_t channels, enum mixed_encoding encoding){
  return mixed_samplesize(encoding)*channels;
}

int mix_noop(struct mixed_segment *segment){
  IGNORE(segment);
  return 1;
}


MIXED_EXPORT int mixed_fwd_fft(uint16_t framesize, float *in, float *out){
  switch(spiral_fft_float(framesize, -1, in, out)){
  case SPIRAL_OK:
    return 1;
  case SPIRAL_SIZE_NOT_SUPPORTED:
  case SPIRAL_INVALID_PARAM:
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  case SPIRAL_OUT_OF_MEMORY:
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  default:
    return 0;
  }
}

MIXED_EXPORT int mixed_inv_fft(uint16_t framesize, float *in, float *out){
  switch(spiral_fft_float(framesize, +1, in, out)){
  case SPIRAL_OK:
    return 1;
  case SPIRAL_SIZE_NOT_SUPPORTED:
  case SPIRAL_INVALID_PARAM:
    mixed_err(MIXED_INVALID_VALUE);
    return 0;
  case SPIRAL_OUT_OF_MEMORY:
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  default:
    return 0;
  }
}

MIXED_EXPORT extern inline float mixed_from_db(mixed_decibel_t volume);
MIXED_EXPORT extern inline  mixed_decibel_t mixed_to_db(float volume);

thread_local int errorcode = 0;

void mixed_err(int code){
  errorcode = code;
}

MIXED_EXPORT int mixed_error(void){
  return errorcode;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wswitch-enum"
MIXED_EXPORT const char *mixed_error_string(int code){
  if(code < 0) code = errorcode;
  switch((enum mixed_error)code){
  case MIXED_NO_ERROR:
    return "No error has occurred.";
  case MIXED_OUT_OF_MEMORY:
    return "An allocation has failed. You are likely out of memory.";
  case MIXED_UNKNOWN_ENCODING:
    return "The specified sample encoding is unknown.";
  case MIXED_MIXING_FAILED:
    return "An error occurred during the mixing of a segment.";
  case MIXED_NOT_IMPLEMENTED:
    return "The segment function you tried to call was not provided.";
  case MIXED_NOT_INITIALIZED:
    return "An attempt was made to use an object without initializing it properly first.";
  case MIXED_INVALID_LOCATION:
    return "Cannot set the field at the specified location in the segment.";
  case MIXED_INVALID_FIELD:
    return "A field that the segment does not recognise was requested.";
  case MIXED_INVALID_VALUE:
    return "A value that is not within the valid range for a field was attempted to be set.";
  case MIXED_SEGMENT_ALREADY_STARTED:
    return "Cannot start the segment as it is already started.";
  case MIXED_SEGMENT_ALREADY_ENDED:
    return "Cannot end the segment as it is already ended.";
  case MIXED_DYNAMIC_OPEN_FAILED:
    return "Failed to open the plugin library. Most likely the file could not be read.";
  case MIXED_BAD_DYNAMIC_LIBRARY:
    return "The plugin library was found but does not seem to be a valid library as it is missing its initialisation function.";
  case MIXED_LADSPA_NO_PLUGIN_AT_INDEX:
    return "The LADSPA plugin library does not have a plugin at the requested index.";
  case MIXED_LADSPA_INSTANTIATION_FAILED:
    return "Instantiation of the LADSPA plugin has failed.";
  case MIXED_RESAMPLE_FAILED:
    return "An error happened in the libsamplerate library.";
  case MIXED_BUFFER_EMPTY:
    return "A read was requested on a buffer with no committed write data.";
  case MIXED_BUFFER_FULL:
    return "A write was requested on a buffer with no available space.";
  case MIXED_BUFFER_OVERCOMMIT:
    return "More data was attempted to be committed to the buffer than was requested.";
  case MIXED_BAD_RESAMPLE_FACTOR:
    return "The requested change in the sample rates is too big.";
  case MIXED_BAD_CHANNEL_CONFIGURATION:
    return "An unsupported channel conversion configuration was requested.";
  case MIXED_BUFFER_ALLOCATED:
    return "An allocated buffer was passed when an unallocated one was expected.";
  case MIXED_BUFFER_MISSING:
    return "An input or output port is missing a buffer.";
  case MIXED_DUPLICATE_SEGMENT:
    return "A segment with the requested name had already been registered.";
  case MIXED_BAD_SEGMENT:
    return "A segment with the requested name is not registered.";
  case MIXED_BAD_ARGUMENT_COUNT:
    return "A bad number of arguments was specified.";
  case MIXED_BAD_NAME:
    return "A bad name was specified.";
  case MIXED_BUFFER_TOO_SMALL:
    return "The given buffer is too small for the required task.";
  case MIXED_INTERNAL_ERROR:
    return "An internal error occurred. This points towards a bug in libmixed.";
  default:
    return "Unknown error code.";
  }
}

MIXED_EXPORT const char *mixed_type_string(int code){
  switch(code){
  case MIXED_UNKNOWN:
    return "unknown";
  case MIXED_INT8:
    return "int8";
  case MIXED_UINT8:
    return "uint8";
  case MIXED_INT16:
    return "int16";
  case MIXED_UINT16:
    return "uint16";
  case MIXED_INT24:
    return "int24";
  case MIXED_UINT24:
    return "uint24";
  case MIXED_INT32:
    return "int32";
  case MIXED_UINT32:
    return "uint32";
  case MIXED_FLOAT:
    return "float";
  case MIXED_DOUBLE:
    return "double";
  case MIXED_BOOL:
    return "bool";
  case MIXED_SIZE_T:
    return "size_t";
  case MIXED_STRING:
    return "string";
  case MIXED_FUNCTION:
    return "function";
  case MIXED_POINTER:
    return "pointer";
  case MIXED_SEGMENT_POINTER:
    return "segment pointer";
  case MIXED_BUFFER_POINTER:
    return "buffer pointer";
  case MIXED_PACK_POINTER:
    return "pack pointer";
  case MIXED_SEGMENT_SEQUENCE_POINTER:
    return "segment sequence pointer";
  case MIXED_LOCATION_ENUM:
    return "location";
  case MIXED_BIQUAD_FILTER_ENUM:
    return "frequency pass";
  case MIXED_REPEAT_MODE_ENUM:
    return "repeat mode";
  case MIXED_NOISE_TYPE_ENUM:
    return "noise type";
  case MIXED_GENERATOR_TYPE_ENUM:
    return "generator type";
  case MIXED_FADE_TYPE_ENUM:
    return "fade type";
  case MIXED_ATTENUATION_ENUM:
    return "attenuation";
  case MIXED_ENCODING_ENUM:
    return "encoding";
  case MIXED_ERROR_ENUM:
    return "error";
  case MIXED_RESAMPLE_TYPE_ENUM:
    return "resample type";
  case MIXED_CHANNEL_T:
    return "channel count";
  case MIXED_DECIBEL_T:
    return "volume (dB)";
  case MIXED_DURATION_T:
    return "duration (s)";
  case MIXED_CHANNEL_CONFIGURATION_POINTER:
    return "channel configuration pointer";
  default:
    return "unknown";
  }
}

MIXED_EXPORT const char *mixed_segment_field_string(int code){
  switch((enum mixed_segment_fields)code){
  case MIXED_BUFFER:
    return "buffer";
  case MIXED_BYPASS:
    return "bypass";
  case MIXED_SAMPLERATE:
    return "samplerate";
  case MIXED_VOLUME:
    return "volume";
  case MIXED_RESAMPLE_TYPE:
    return "resample type";
  case MIXED_VOLUME_CONTROL_PAN:
    return "pan";
  case MIXED_FADE_FROM:
    return "fade start";
  case MIXED_FADE_TO:
    return "fade end";
  case MIXED_FADE_TIME:
    return "fade duration";
  case MIXED_FADE_TYPE:
    return "fade function";
  case MIXED_GENERATOR_FREQUENCY:
    return "frequency";
  case MIXED_GENERATOR_TYPE:
    return "wave type";
  case MIXED_SPACE_LOCATION:
    return "location";
  case MIXED_SPACE_DIRECTION:
    return "direction";
  case MIXED_SPACE_VELOCITY:
    return "velocity";
  case MIXED_SPACE_UP:
    return "up";
  case MIXED_SPACE_SOUNDSPEED:
    return "soundspeed";
  case MIXED_SPACE_DOPPLER_FACTOR:
    return "doppler factor";
  case MIXED_SPACE_MIN_DISTANCE:
    return "minimum distance";
  case MIXED_SPACE_MAX_DISTANCE:
    return "maximum distance";
  case MIXED_SPACE_ROLLOFF:
    return "rolloff factor";
  case MIXED_SPACE_ATTENUATION:
    return "attenuation factor";
  case MIXED_DELAY_TIME:
    return "delay time";
  case MIXED_PITCH_SHIFT:
    return "pitch shift factor";
  case MIXED_GATE_OPEN_THRESHOLD:
    return "open threshold";
  case MIXED_GATE_CLOSE_THRESHOLD:
    return "close threshold";
  case MIXED_GATE_ATTACK:
    return "attack";
  case MIXED_GATE_HOLD:
    return "hold";
  case MIXED_GATE_RELEASE:
    return "release";
  case MIXED_NOISE_TYPE:
    return "noise type";
  case MIXED_REPEAT_TIME:
    return "repeat duration";
  case MIXED_REPEAT_MODE:
    return "repetition mode";
  case MIXED_FREQUENCY:
    return "frequency";
  case MIXED_BIQUAD_FILTER:
    return "filter type";
  case MIXED_GAIN:
    return "gain";
  case MIXED_Q:
    return "Q";
  case MIXED_IN_COUNT:
    return "input count";
  case MIXED_OUT_COUNT:
    return "output count";
  case MIXED_CURRENT_SEGMENT:
    return "current segment";
  case MIXED_SPEED_FACTOR:
    return "speed factor";
  case MIXED_QUANTIZE_STEPS:
    return "quantization steps";
  case MIXED_MIX:
    return "mix";
  case MIXED_PLANE_LOCATION:
    return "location";
  case MIXED_PLANE_VELOCITY:
    return "velocity";
  case MIXED_COMPRESSOR_PREGAIN:
    return "pregain";
  case MIXED_COMPRESSOR_THRESHOLD:
    return "threshold";
  case MIXED_COMPRESSOR_KNEE:
    return "knee";
  case MIXED_COMPRESSOR_RATIO:
    return "ratio";
  case MIXED_COMPRESSOR_ATTACK:
    return "attack";
  case MIXED_COMPRESSOR_RELEASE:
    return "release";
  case MIXED_COMPRESSOR_PREDELAY:
    return "predelay";
  case MIXED_COMPRESSOR_RELEASEZONE:
    return "release zone";
  case MIXED_COMPRESSOR_POSTGAIN:
    return "postgain";
  case MIXED_COMPRESSOR_GAIN:
    return "gain";
  case MIXED_CHANNEL_COUNT_IN:
    return "input channel count";
  case MIXED_CHANNEL_COUNT_OUT:
    return "output channel count";
  case MIXED_REPEAT_POSITION:
    return "reptition position";
  case MIXED_FRAMESIZE:
    return "frame size";
  case MIXED_OVERSAMPLING:
    return "oversampling factor";
  case MIXED_BUFFER_SIZE_HINT:
    return "buffer size hint";
  case MIXED_FIR:
    return "finite input response";
  case MIXED_CHANNEL_CONFIGURATION:
    return "channel configuration";
  default:
    return "unknown";
  }
}

MIXED_EXPORT const char *mixed_location_string(int code){
  switch((enum mixed_location)code){
  case MIXED_LEFT_FRONT_BOTTOM:
    return "left front bottom";
  case MIXED_RIGHT_FRONT_BOTTOM:
    return "right front bottom";
  case MIXED_LEFT_REAR_BOTTOM:
    return "left rear bottom";
  case MIXED_RIGHT_REAR_BOTTOM:
    return "right rear bottom";
  case MIXED_CENTER_FRONT:
    return "center front";
  case MIXED_SUBWOOFER:
    return "subwoofer";
  case MIXED_LEFT_SIDE:
    return "left side";
  case MIXED_RIGHT_SIDE:
    return "right side";
  case MIXED_LEFT_FRONT_TOP:
    return "left front top";
  case MIXED_RIGHT_FRONT_TOP:
    return "right front top";
  case MIXED_LEFT_REAR_TOP:
    return "left rear top";
  case MIXED_RIGHT_REAR_TOP:
    return "right rear top";
  case MIXED_CENTER_REAR:
    return "center rear";
  case MIXED_LEFT_SIDE_TOP:
    return "left side top";
  case MIXED_RIGHT_SIDE_TOP:
    return "right side top";
  case MIXED_SUBWOOFER_2:
    return "subwoofer 2";
  case MIXED_LEFT_FRONT_WIDE:
    return "left front wide";
  case MIXED_RIGHT_FRONT_WIDE:
    return "right front wide";
  case MIXED_CENTER_FRONT_TOP:
    return "center front top";
  case MIXED_CENTER_REAR_TOP:
    return "center rear top";
  case MIXED_LEFT_REAR_CENTER:
    return "left rear center";
  case MIXED_RIGHT_REAR_CENTER:
    return "right rear center";
  case MIXED_SUBWOOFER_LEFT:
    return "left subwoofer";
  case MIXED_SUBWOOFER_RIGHT:
    return "right subwoofer";
  case MIXED_LEFT_FRONT_HIGH:
    return "left front high";
  case MIXED_RIGHT_FRONT_HIGH:
    return "right front high";
  case MIXED_CENTER_FRONT_HIGH:
    return "center front high";
  case MIXED_CENTER_BOTTOM:
    return "center bottom";
  case MIXED_LEFT_CENTER_BOTTOM:
    return "left center bottom";
  case MIXED_RIGHT_CENTER_BOTTOM:
    return "right center bottom";
  case MIXED_LEFT_FRONT_CENTER_TOP:
    return "left front center top";
  case MIXED_RIGHT_FRONT_CENTER_BOTTOM:
    return "right front center bottom";
  case MIXED_MAX_SPEAKER_COUNT:
  default:
    return "unknown";
  }
}

MIXED_EXPORT const char *mixed_info_flag_string(int code){
  switch((enum mixed_segment_info_flags)code){
  case MIXED_IN:
    return "input";
  case MIXED_OUT:
    return "output";
  case MIXED_SEGMENT:
    return "segment";
  case MIXED_SET:
    return "settable";
  case MIXED_GET:
    return "gettable";
  case MIXED_CLEARS_OUTPUT:
    return "clears output";
  default:
    return "unknown";
  }
}

#pragma GCC diagnostic pop

MIXED_EXPORT const char *mixed_version(void){
  return MIXED_VERSION;
}

#ifndef MIXED_NO_CUSTOM_ALLOCATOR
void *(*mixed_calloc)(size_t num, size_t size) = calloc;
void (*mixed_free)(void *ptr) = free;
void *(*mixed_realloc)(void *ptr, size_t size) = realloc;
#endif

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size){
  size_t newsize = newcount*size;
  size_t oldsize = oldcount*size;
  ptr = mixed_realloc(ptr, newsize);
  if(ptr && oldsize < newsize){
    memset(((char*)ptr)+oldsize, 0, newsize-oldsize);
  }
  return ptr;
}

void set_info_field(struct mixed_segment_field_info *info, uint32_t field, enum mixed_segment_field_type type, uint32_t count, enum mixed_segment_info_flags flags, const char*description){
  info->field = field;
  info->description = description;
  info->flags = flags;
  info->type = type;
  info->type_count = count;
}

void clear_info_field(struct mixed_segment_field_info *info){
  info->field = 0;
  info->description = 0;
  info->flags = 0;
  info->type = 0;
  info->type_count = 0;
}

void *open_library(const char *file){
#ifdef _WIN32
  void *lib = LoadLibrary(file);
  if(!lib){
    mixed_err(MIXED_DYNAMIC_OPEN_FAILED);
    return 0;
  }
  return lib;
#elif MIXED_DL
  void *lib = dlopen(file, RTLD_NOW);
  if(!lib){
    fprintf(stderr, "MIXED: DYLD error: %s\n", dlerror());
    mixed_err(MIXED_DYNAMIC_OPEN_FAILED);
    return 0;
  }
  dlerror();
  return lib;
#else
  IGNORE(file);
  mixed_err(MIXED_DYNAMIC_OPEN_FAILED);
  return 0;
#endif
}

void close_library(void *handle){
  if(handle)
#ifdef _WIN32
    FreeLibrary(handle);
#elif MIXED_DL
    dlclose(handle);
#else
    return;
#endif
}

void *load_symbol(void *handle, const char *name){
#ifdef _WIN32
  void *function = GetProcAddress(handle, "mixed_make_plugin");
  if(!function){
    mixed_err(MIXED_BAD_DYNAMIC_LIBRARY);
    return 0;
  }
  return function;
#elif MIXED_DL
  void *function = dlsym(handle, name);
  char *error = dlerror();
  if(error != 0){
    fprintf(stderr, "MIXED: DYLD error: %s\n", error);
    mixed_err(MIXED_BAD_DYNAMIC_LIBRARY);
    return 0;
  }
  return function;
#else
  IGNORE(handle, name);
  mixed_err(MIXED_BAD_DYNAMIC_LIBRARY);
  return 0;
#endif
}

unsigned int hash_rng_pos = 1;
unsigned int hash_rng_seed = 0x42574223;

unsigned int mixed_random_int(void){
  const unsigned int BIT_NOISE1 = 0x68E31DA4;
  const unsigned int BIT_NOISE2 = 0xB5297A4D;
  const unsigned int BIT_NOISE3 = 0x1B56C4E9;
  unsigned int mangled;

 retry:
  mangled = hash_rng_pos;
  if(!atomic_cas(hash_rng_pos, mangled, mangled+1))
    goto retry;
  
  mangled *= BIT_NOISE1;
  mangled += hash_rng_seed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled << 8);
  mangled *= BIT_NOISE3;
  mangled ^= (mangled >> 8);
  return mangled;
}

float mixed_random(void){
  return ((float)mixed_random_int())/((float)0xFFFFFFFF);
}

static void init(void) __attribute__((constructor));
void init(void){
  hash_rng_seed = time(NULL);
}
