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
    return "An unknown sample encoding was specified for a channel.";
  case MIXED_UNKNOWN_LAYOUT:
    return "An unknown sample layout was specified for a channel.";
  case MIXED_MIXING_FAILED:
    return "An error occurred during the mixing of a segment.";
  case MIXED_NOT_IMPLEMENTED:
    return "The segment function you tried to call was not provided.";
  case MIXED_NOT_INITIALIZED:
    return "An attempt was made to use an object without initializing it properly first.";
  default:
    return "Unknown error code.";
  }
}
