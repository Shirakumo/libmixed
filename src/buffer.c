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
  buffer->size = 0;
  mixed_buffer_clear(buffer);
}

MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer){
  buffer->r1_start = 0;
  buffer->r1_size = 0;
  buffer->r2_start = 0;
  buffer->r2_size = 0;
  buffer->reserved_start = 0;
  buffer->reserved_size = 0;
  return 1;
}

static inline size_t free_for_r2(struct mixed_buffer *buffer){
  return buffer->r1_start - buffer->r2_start - buffer->r2_size;
}

static inline size_t free_after_r1(struct mixed_buffer *buffer){
  return buffer->size - buffer->r1_start - buffer->r2_size;
}

MIXED_EXPORT int mixed_buffer_request_write(struct mixed_buffer *buffer, float **area, size_t *size){
  if(buffer->r2_size){
    size_t free = free_for_r2(buffer);
    if(*size < free) free = *size;
    if(free == 0) return 0;
    *size = free;
    buffer->reserved_size = free;
    buffer->reserved_start = buffer->r2_start + buffer->r2_size;
    *area = buffer->data + buffer->reserved_start;
  }else{
    size_t free = free_after_r1(buffer);
    if(buffer->r1_start <= free){
      if(free == 0) return 0;
      if(*size < free) free = *size;
      *size = free;
      buffer->reserved_size = free;
      buffer->reserved_start = buffer->r1_start + buffer->r1_size;
      *area = buffer->data + buffer->reserved_start;
    } else {
      if(buffer->r1_start == 0) return 0;
      if(buffer->r1_start < *size) *size = buffer->r1_start;
      buffer->reserved_size = *size;
      buffer->reserved_start = 0;
      *area = buffer->data;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_write(struct mixed_buffer *buffer, size_t size){
  if(buffer->reserved_size < size) return 0;
  if(size == 0){
    buffer->reserved_size = 0;
    buffer->reserved_start = 0;
  }else if(buffer->r1_size == 0 && buffer->r2_size == 0){
    buffer->r1_start = buffer->reserved_start;
    buffer->r1_size = size;
  }else if(buffer->reserved_start == buffer->r1_start + buffer->r1_size){
    buffer->r1_size += size;
  }else{
    buffer->r2_size += size;
  }
  buffer->reserved_start = 0;
  buffer->reserved_size = 0;
  return 1;
}

MIXED_EXPORT int mixed_buffer_request_read(struct mixed_buffer *buffer, float **area, size_t *size){
  if(buffer->r1_size == 0) return 0;
  *size = buffer->r1_size;
  *area = buffer->data + buffer->r1_start;
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_read(struct mixed_buffer *buffer, size_t size){
  if(buffer->r1_size < size) return 0;
  if(buffer->r1_size == size){
    buffer->r1_start = buffer->r2_start;
    buffer->r1_size = buffer->r2_size;
    buffer->r2_start = 0;
    buffer->r2_size = 0;
  }else{
    buffer->r1_size -= size;
    buffer->r1_start += size;
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  // FIXME: fixup
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
  // FIXME: fixup
  void *new = realloc(buffer->data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->data = new;
  buffer->size = size;
  return 1;
}
