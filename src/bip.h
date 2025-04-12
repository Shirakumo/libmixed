#include "internal.h"

#define read_buffer_state(READ, WRITE, FULL_R2, BUFFER) \
  uint32_t READ = atomic_read(BUFFER->read);            \
  uint32_t WRITE ## _ = atomic_read(BUFFER->write);     \
  char FULL_R2 = WRITE ## _ >> 31;                      \
  uint32_t WRITE = WRITE ## _ & 0x7FFFFFFF;

static inline int bip_request_write(uint32_t *off, uint32_t *size, struct bip *buffer){
  mixed_err(MIXED_NO_ERROR);
  uint32_t to_write = *size;
  read_buffer_state(read, write, full_r2, buffer);
  // Check if we're waiting for read to catch up with a full second region
  if(!full_r2){
    uint32_t available = buffer->size - write;
    if(0 < available){ // No, we still have space left.
      to_write = MIN(to_write, available);
      *size = to_write;
      *off = write;
      buffer->reserved = to_write;
    }else if(0 < read){ // We are at the end and need to wrap now.
      to_write = MIN(to_write, read);
      *size = to_write;
      *off = 0;
      buffer->reserved = to_write;
      atomic_write(buffer->write, 0x80000000);
    }else{ // Read has not done anything yet, no space!
      *size = 0;
      *off = 0;
      debug_log("%p Overrun", (void*)buffer);
      return 0;
    }
  }else if(write < read){
    // We're behind read, but still have some space.
    to_write = MIN(to_write, read-write);
    *size = to_write;
    *off = write;
    buffer->reserved = to_write;
  }else{
    *size = 0;
    *off = 0;
    debug_log("%p Overrun", (void*)buffer);
    return 0;
  }
  return 1;
}

static inline int bip_finish_write(uint32_t size, struct bip *buffer){
  if(buffer->reserved < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
 retry: {
    uint32_t write = atomic_read(buffer->write);
    if(!atomic_cas(buffer->write, write, write+size))
      goto retry;
  }
  buffer->reserved = 0;
  return 1;
}

static inline int bip_request_read(uint32_t *off, uint32_t *size, struct bip *buffer){
  read_buffer_state(read, write, full_r2, buffer);
  if(full_r2){
    uint32_t available = buffer->size - read;
    if(0 < available){
      *size = MIN(*size, available);
      *off = read;
    }else if(0 < write){ // We are at the end and need to wrap now.
    retry:
      if(!atomic_cas(buffer->write, write_, write)){
        write_ = atomic_read(buffer->write);
        write = write_ & 0x7FFFFFFF;
        goto retry;
      }
      *size = MIN(*size, write);
      *off = 0;
      atomic_write(buffer->read, 0);
    }else{ // Write has not done anything yet, no space!
      *size = 0;
      *off = 0;
      debug_log("%p Underrun", (void*)buffer);
      return 0;
    }
  }else if(read < write){
    *size = MIN(*size, write-read);
    *off = read;
  }else{
    *size = 0;
    *off = 0;
    debug_log("%p Underrun", (void*)buffer);
    return 0;
  }
  return 1;
}

static inline int bip_finish_read(uint32_t size, struct bip *buffer){
  read_buffer_state(read, write, full_r2, buffer);
  if(full_r2){
    if(buffer->size-read < size){
      mixed_err(MIXED_BUFFER_OVERCOMMIT);
      return 0;
    }else{
      atomic_write(buffer->read, read+size);
    }
  }else if(read<write){
    if(write-read < size){
      mixed_err(MIXED_BUFFER_OVERCOMMIT);
      return 0;
    }
    atomic_write(buffer->read, read+size);
  }else if(0 < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  return 1;
}

static inline void bip_discard(struct bip *buffer){
  atomic_write(buffer->read, 0);
  atomic_write(buffer->write, 0);
  atomic_write(buffer->reserved, 0);
}

static inline uint32_t bip_available_read(struct bip *buffer){
  read_buffer_state(read, write, full_r2, buffer);
  if(full_r2){
    if(read < buffer->size)
      return buffer->size - read;
    else
      return write;
  }else
    return write - read;
}

static inline uint32_t bip_available_write(struct bip *buffer){
  read_buffer_state(read, write, full_r2, buffer);
  if(full_r2)
    return read - write;
  else if(write == buffer->size)
    return read;
  else
    return buffer->size - write;
}
