#include "internal.h"

MIXED_EXPORT int mixed_make_buffer(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->_data && !buffer->virtual){
    mixed_err(MIXED_BUFFER_ALLOCATED);
    return 0;
  }
  buffer->_data = calloc(size, sizeof(float));
  if(!buffer->_data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->virtual = 0;
  buffer->size = size;
  return 1;
}

MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->_data && !buffer->virtual)
    free(buffer->_data);
  buffer->_data = 0;
  buffer->size = 0;
  buffer->virtual = 0;
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
  return buffer->size - buffer->r1_start - buffer->r1_size;
}

MIXED_EXPORT int mixed_buffer_request_write(float **area, size_t *size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->r2_size){
    size_t free = free_for_r2(buffer);
    if(*size < free) free = *size;
    if(free == 0){
      *size = 0;
      mixed_err(MIXED_BUFFER_FULL);
      return 0;
    }
    *size = free;
    buffer->reserved_size = free;
    buffer->reserved_start = buffer->r2_start + buffer->r2_size;
    *area = buffer->_data + buffer->reserved_start;
  }else{
    size_t free = free_after_r1(buffer);
    if(buffer->r1_start <= free){
      if(free == 0){
        *size = 0;
        mixed_err(MIXED_BUFFER_FULL);
        return 0;
      }
      if(*size < free) free = *size;
      *size = free;
      buffer->reserved_size = free;
      buffer->reserved_start = buffer->r1_start + buffer->r1_size;
      *area = buffer->_data + buffer->reserved_start;
    } else {
      if(buffer->r1_start == 0) {
        *size = 0;
        mixed_err(MIXED_BUFFER_FULL);
        return 0;
      }
      if(buffer->r1_start < *size) *size = buffer->r1_start;
      buffer->reserved_size = *size;
      buffer->reserved_start = 0;
      *area = buffer->_data;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_write(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->reserved_size < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
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

MIXED_EXPORT int mixed_buffer_request_read(float **area, size_t *size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->r1_size == 0){
    *size = 0;
    mixed_err(MIXED_BUFFER_EMPTY);
    return 0;
  }
  *size = MIN(*size, buffer->r1_size);
  *area = buffer->_data + buffer->r1_start;
  return 1;
}

MIXED_EXPORT size_t mixed_buffer_available_read(struct mixed_buffer *buffer){
  return buffer->r1_size;
}

MIXED_EXPORT size_t mixed_buffer_available_write(struct mixed_buffer *buffer){
  return (buffer->r2_size)? free_for_r2(buffer) : free_after_r1(buffer);
}

MIXED_EXPORT int mixed_buffer_finish_read(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->r1_size < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
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

MIXED_EXPORT int mixed_buffer_transfer(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  if(from != to){
    float *read, *write;
    size_t samples = SIZE_MAX;
    mixed_buffer_request_read(&read, &samples, from);
    mixed_buffer_request_write(&write, &samples, to);
    memcpy(write, read, sizeof(float)*samples);
    mixed_buffer_finish_read(samples, from);
    mixed_buffer_finish_write(samples, to);
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_copy(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  if(from != to){
    float *read, *write;
    size_t samples = SIZE_MAX;
    mixed_buffer_request_read(&read, &samples, from);
    mixed_buffer_request_write(&write, &samples, to);
    memcpy(write, read, sizeof(float)*samples);
    mixed_buffer_finish_write(samples, to);
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_resize(size_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  // FIXME: fixup
  void *new = realloc(buffer->_data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->_data = new;
  buffer->size = size;
  return 1;
}
