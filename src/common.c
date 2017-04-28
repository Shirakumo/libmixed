#include "mixed.h"

enum mixed_error errorcode = 0;

void mixed_err(enum mixed_error code){
  errorcode = code;
}

enum mixed_error mixed_error(){
  return errorcode;
}

char *mixed_error_string(enum mixed_error code){
  switch(code){
  MIXED_NO_ERROR:
    return "No error has occurred.";
  MIXED_OUT_OF_MEMORY:
    return "An allocation has failed. You are likely out of memory.";
  MIXED_UNKNOWN_ENCODING:
    return "An unknown sample encoding was specified for a channel.";
  MIXED_UNKNOWN_LAYOUT:
    return "An unknown sample layout was specified for a channel.";
  MIXED_INPUT_TAKEN:
    return "A segment's input is connected twice.";
  MIXED_INPUT_MISSING:
    return "A segment's input is not connected.";
  MIXED_GRAPH_CYCLE:
    return "A cycle was detected in the segment graph.";
  MIXED_MIXING_FAILED:
    return "An error occurred during the mixing of a segment.";
  default:
    return "Unknown error code.";
  }
}
