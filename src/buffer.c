#include "internal.h"

MIXED_EXPORT int mixed_make_buffer(uint32_t size, struct mixed_buffer *buffer){
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
  buffer->read = 0;
  buffer->write = 0;
  buffer->full_r2 = 0;
  buffer->reserved = 0;
  return 1;
}

MIXED_EXPORT int mixed_buffer_request_write(float **area, uint32_t *size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  uint32_t to_write = *size;
  char full_r2 = atomic_read(buffer->full_r2);
  uint32_t read = atomic_read(buffer->read);
  uint32_t write = atomic_read(buffer->write);
  // Check if we're waiting for read to catch up with a full second region
  if(!full_r2){
    uint32_t available = buffer->size - write;
    if(0 < available){ // No, we still have space left.
      to_write = MIN(to_write, available);
      *size = to_write;
      *area = buffer->_data + write;
      buffer->reserved = to_write;
    }else if(0 < read){ // We are at the end and need to wrap now.
      to_write = MIN(to_write, read);
      *size = to_write;
      *area = buffer->_data;
      buffer->reserved = to_write;
      atomic_write(buffer->full_r2, 1);
      atomic_write(buffer->write, 0);
    }else{ // Read has not done anything yet, no space!
      *size = 0;
      *area = 0;
      return 0;
    }
  }else if(write < read){
    // We're behind read, but still have some space.
    to_write = MIN(to_write, read-write);
    *size = to_write;
    *area = buffer->_data+write;
    buffer->reserved = to_write;
  }else{
    *size = 0;
    *area = 0;
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_write(uint32_t size, struct mixed_buffer *buffer){
  if(buffer->reserved < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  uint32_t write = atomic_read(buffer->write);
  atomic_write(buffer->write, write+size);
  buffer->reserved = 0;
  return 1;
}

MIXED_EXPORT int mixed_buffer_request_read(float **area, uint32_t *size, struct mixed_buffer *buffer){
  char full_r2 = atomic_read(buffer->full_r2);
  uint32_t write = atomic_read(buffer->write);
  uint32_t read = atomic_read(buffer->read);
  if(full_r2){
    //printf("[A %i %i %i]", full_r2, write, read);
    *size = MIN(*size, buffer->size-read);
    *area = buffer->_data+read;
  }else if(read<write){
    //printf("[B %i %i %i]", full_r2, write, read);
    *size = MIN(*size, write-read);
    *area = buffer->_data+read;
  }else{
    //printf("[C %i %i %i]", full_r2, write, read);
    *size = 0;
    *area = 0;
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_read(uint32_t size, struct mixed_buffer *buffer){
  char full_r2 = atomic_read(buffer->full_r2);
  uint32_t write = atomic_read(buffer->write);
  uint32_t read = atomic_read(buffer->read);
  if(full_r2){
    if(buffer->size-read < size){
      mixed_err(MIXED_BUFFER_OVERCOMMIT);
      //printf("{A %i %i %i}", full_r2, write, read);
      return 0;
    }else if(buffer->size-read == size){
      //printf("{CR2}");
      atomic_write(buffer->read, 0);
      atomic_write(buffer->full_r2, 0);
    }else{
      atomic_write(buffer->read, read+size);
    }
  }else if(read<write){
    if(write-read < size){
      mixed_err(MIXED_BUFFER_OVERCOMMIT);
      //printf("{B %i %i %i}", full_r2, write, read);
      return 0;
    }
    atomic_write(buffer->read, read+size);
  }else if(0 < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    //printf("{C %i %i %i %i}", full_r2, write, read, size);
    return 0;
  }
  return 1;
}

MIXED_EXPORT uint32_t mixed_buffer_available_read(struct mixed_buffer *buffer){
  uint32_t read = atomic_read(buffer->read);
  if(atomic_read(buffer->full_r2)){
    if(read < buffer->size)
      return buffer->size - read;
    else
      return atomic_read(buffer->write);
  }else
    return (atomic_read(buffer->write) - read);
}

MIXED_EXPORT uint32_t mixed_buffer_available_write(struct mixed_buffer *buffer){
  uint32_t read = atomic_read(buffer->read);
  uint32_t write = atomic_read(buffer->write);
  if(atomic_read(buffer->full_r2))
    return read - write;
  else if(write == buffer->size)
    return read;
  else
    return buffer->size - write;
}

MIXED_EXPORT int mixed_buffer_transfer(struct mixed_buffer *from, struct mixed_buffer *to){
  mixed_err(MIXED_NO_ERROR);
  if(from != to){
    float *read, *write;
    uint32_t samples = UINT32_MAX;
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
    uint32_t samples = UINT32_MAX;
    mixed_buffer_request_read(&read, &samples, from);
    mixed_buffer_request_write(&write, &samples, to);
    memcpy(write, read, sizeof(float)*samples);
    mixed_buffer_finish_write(samples, to);
  }
  return 1;
}

MIXED_EXPORT int mixed_buffer_resize(uint32_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  float *new = realloc(buffer->_data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->_data = new;
  buffer->size = size;
  return 1;
}
