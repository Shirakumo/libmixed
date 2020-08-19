#include "internal.h"

MIXED_EXPORT int mixed_make_pack(uint32_t frames, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  pack->_data = calloc(frames*pack->channels, mixed_samplesize(pack->encoding));
  if(!pack->_data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  pack->size = frames*pack->channels*mixed_samplesize(pack->encoding);
  return 1;
}

MIXED_EXPORT void mixed_free_pack(struct mixed_pack *pack){
  if(pack->_data)
    free(pack->_data);
  pack->_data = 0;
  pack->size = 0;
  mixed_pack_clear(pack);
}

MIXED_EXPORT int mixed_pack_clear(struct mixed_pack *pack){
  pack->read = 0;
  pack->write = 0;
  pack->full_r2 = 0;
  pack->reserved = 0;
  return 1;
}

MIXED_EXPORT int mixed_pack_request_write(void **area, uint32_t *size, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  uint32_t to_write = *size;
  uint32_t read = atomic_read(pack->read);
  uint32_t write = atomic_read(pack->write);
  if(!atomic_read(pack->full_r2)){
    uint32_t available = pack->size - write;
    if(0 < available){
      to_write = MIN(to_write, available);
      *size = to_write;
      *area = pack->_data + write;
      pack->reserved = to_write;
    }else if(0 < read){
      to_write = MIN(to_write, read);
      *size = to_write;
      *area = pack->_data;
      pack->reserved = to_write;
      atomic_write(pack->full_r2, 1);
      atomic_write(pack->write, 0);
    }else{
      *size = 0;
      *area = 0;
    }
  }else if(write < read){
    // We're behind read, but still have some space.
    to_write = MIN(to_write, read-write);
    *size = to_write;
    *area = pack->_data+write;
    pack->reserved = to_write;
  }else{
    *size = 0;
    *area = 0;
    return 0;
  }
  return 1;
}

MIXED_EXPORT int mixed_pack_finish_write(uint32_t size, struct mixed_pack *pack){
  if(pack->reserved < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  uint32_t write = atomic_read(pack->write);
  atomic_write(pack->write, write+size);
  pack->reserved = 0;
  return 1;
}

MIXED_EXPORT int mixed_pack_request_read(void **area, uint32_t *size, struct mixed_pack *pack){
  uint32_t read = atomic_read(pack->read);
  uint32_t available = atomic_read(pack->full_r2)
    ?(pack->size - read)
    :(atomic_read(pack->write) - read);
  if(0 < available){
    *size = MIN(*size, available);
    *area = pack->_data + read;
    return 1;
  }else if(atomic_read(pack->full_r2)){
    atomic_write(pack->full_r2, 0);
    atomic_write(pack->read, 0);
    *size = MIN(*size, atomic_read(pack->write));
    *area = pack->_data;
    return 1;
  }else{
    *size = 0;
    *area = 0;
    return 0;
  }
}

MIXED_EXPORT int mixed_pack_finish_read(uint32_t size, struct mixed_pack *pack){
  uint32_t read = atomic_read(pack->read);
  uint32_t available = atomic_read(pack->full_r2)
    ?(pack->size - read)
    :(atomic_read(pack->write) - read);
  if(available < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  atomic_write(pack->read, read+size);
  return 1;
}

MIXED_EXPORT uint32_t mixed_pack_available_read(struct mixed_pack *pack){
  uint32_t read = atomic_read(pack->read);
  if(atomic_read(pack->full_r2)){
    if(read < pack->size)
      return pack->size - read;
    else
      return atomic_read(pack->write);
  }else
    return (atomic_read(pack->write) - read);
}

MIXED_EXPORT uint32_t mixed_pack_available_write(struct mixed_pack *pack){
  uint32_t read = atomic_read(pack->read);
  uint32_t write = atomic_read(pack->write);
  if(atomic_read(pack->full_r2))
    return read - write;
  else if(write == pack->size)
    return read;
  else
    return pack->size - write;
}
