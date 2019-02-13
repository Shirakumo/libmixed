#include "internal.h"

MIXED_EXPORT int mixed_make_buffer(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  buffer->data = calloc(size, sizeof(float));
  if(!buffer->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->size = size;
  return 1;
}

MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->data)
    free(buffer->data);
  buffer->data = 0;
}

MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer){
  if(buffer->data){
    memset(buffer->data, 0, sizeof(float)*buffer->size);
    return 1;
  }
  return 0;
}

MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  if(from != to){
    size_t size = (to->size<from->size)? from->size : to->size;
    memcpy(to->data, from->data, sizeof(float)*size);
    if(size < to->size){
      memset(to->data+size, 0, sizeof(float)*(to->size-size));
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_resize(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  void *new = realloc(buffer->data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->data = new;
  buffer->size = size;
  return 1;
}
