#include "internal.h"
#include "bip.h"

MIXED_EXPORT int mixed_make_buffer(uint32_t size, struct mixed_buffer *buffer){
  mixed_err(MIXED_NO_ERROR);
  if(buffer->_data && !buffer->is_virtual){
    mixed_err(MIXED_BUFFER_ALLOCATED);
    return 0;
  }
  buffer->_data = mixed_calloc(size, sizeof(float));
  if(!buffer->_data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->is_virtual = 0;
  buffer->size = size;
  return 1;
}

MIXED_EXPORT void mixed_free_buffer(struct mixed_buffer *buffer){
  if(buffer->_data && !buffer->is_virtual)
    mixed_free(buffer->_data);
  buffer->_data = 0;
  buffer->size = 0;
  buffer->is_virtual = 0;
  mixed_buffer_clear(buffer);
}

MIXED_EXPORT int mixed_buffer_clear(struct mixed_buffer *buffer){
  buffer->read = 0;
  buffer->write = 0;
  buffer->reserved = 0;
  return 1;
}

MIXED_EXPORT int mixed_buffer_request_write(float **area, uint32_t *size, struct mixed_buffer *buffer){
  uint32_t off = 0;
  if(!bip_request_write(&off, size, (struct bip*)buffer)){
    *area = 0;
    return 0;
  }
  *area = buffer->_data+off;
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_write(uint32_t size, struct mixed_buffer *buffer){
  return bip_finish_write(size, (struct bip*)buffer);
}

MIXED_EXPORT int mixed_buffer_request_read(float **area, uint32_t *size, struct mixed_buffer *buffer){
  uint32_t off = 0;
  if(!bip_request_read(&off, size, (struct bip*)buffer)){
    *area = 0;
    return 0;
  }
  *area = buffer->_data+off;
  return 1;
}

MIXED_EXPORT int mixed_buffer_finish_read(uint32_t size, struct mixed_buffer *buffer){
  return bip_finish_read(size, (struct bip*)buffer);
}

MIXED_EXPORT uint32_t mixed_buffer_available_read(struct mixed_buffer *buffer){
  return bip_available_read((struct bip*)buffer);
}

MIXED_EXPORT uint32_t mixed_buffer_available_write(struct mixed_buffer *buffer){
  return bip_available_write((struct bip*)buffer);
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
  float *new = mixed_realloc(buffer->_data, size*sizeof(float));
  if(!new){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  buffer->_data = new;
  buffer->size = size;
  return 1;
}
