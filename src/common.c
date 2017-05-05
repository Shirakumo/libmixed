#include "internal.h"

uint8_t mixed_samplesize(enum mixed_encoding encoding){
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

int errorcode = 0;

void mixed_err(int code){
  errorcode = code;
}

int mixed_error(){
  return errorcode;
}

char *mixed_error_string(int code){
  if(code < 0) code = errorcode;
  switch(code){
  case MIXED_NO_ERROR:
    return "No error has occurred.";
  case MIXED_OUT_OF_MEMORY:
    return "An allocation has failed. You are likely out of memory.";
  case MIXED_UNKNOWN_ENCODING:
    return "The specified sample encoding is unknown.";
  case MIXED_UNKNOWN_LAYOUT:
    return "The specified sample layout is unknown.";
  case MIXED_MIXING_FAILED:
    return "An error occurred during the mixing of a segment.";
  case MIXED_NOT_IMPLEMENTED:
    return "The segment function you tried to call was not provided.";
  case MIXED_NOT_INITIALIZED:
    return "An attempt was made to use an object without initializing it properly first.";
  case MIXED_INVALID_BUFFER_LOCATION:
    return "Cannot set the buffer at the specified location in the segment.";
  case MIXED_INVALID_FIELD:
    return "A field that the segment does not recognise was requested.";
  case MIXED_SEGMENT_ALREADY_STARTED:
    return "Cannot start the segment as it is already started.";
  case MIXED_SEGMENT_ALREADY_ENDED:
    return "Cannot end the segment as it is already ended.";
  case MIXED_LADSPA_OPEN_FAILED:
    return "Failed to open the LADSPA plugin library. Most likely the file could not be read.";
  case MIXED_LADSPA_BAD_LIBRARY:
    return "The LADSPA plugin library was found but does not seem to be a valid LADSPA 1.1 library.";
  case MIXED_LADSPA_NO_PLUGIN_AT_INDEX:
    return "The LADSPA plugin library does not have a plugin at the requested index.";
  case MIXED_LADSPA_INSTANTIATION_FAILED:
    return "Instantiation of the LADSPA plugin has failed.";
  default:
    return "Unknown error code.";
  }
}

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size){
  size_t newsize = newcount*size;
  size_t oldsize = oldcount*size;
  ptr = realloc(ptr, newsize);
  if(ptr && oldsize < newsize){
    memset(ptr+oldsize, 0, newsize-oldsize);
  }
  return ptr;
}
