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
  pack->r1_start = 0;
  pack->r1_size = 0;
  pack->r2_start = 0;
  pack->r2_size = 0;
  pack->reserved_start = 0;
  pack->reserved_size = 0;
  return 1;
}

static inline uint32_t free_for_r2(struct mixed_pack *pack){
  return pack->r1_start - pack->r2_start - pack->r2_size;
}

static inline uint32_t free_after_r1(struct mixed_pack *pack){
  return pack->size - pack->r1_start - pack->r1_size;
}

MIXED_EXPORT int mixed_pack_request_write(void **area, uint32_t *size, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  if(pack->r2_size){
    uint32_t free = free_for_r2(pack);
    if(*size < free) free = *size;
    if(free == 0){
      *size = 0;
      mixed_err(MIXED_BUFFER_FULL);
      return 0;
    }
    *size = free;
    pack->reserved_size = free;
    pack->reserved_start = pack->r2_start + pack->r2_size;
    *area = pack->_data + pack->reserved_start;
  }else{
    uint32_t free = free_after_r1(pack);
    if(pack->r1_start <= free){
      if(free == 0){
        *size = 0;
        mixed_err(MIXED_BUFFER_FULL);
        return 0;
      }
      if(*size < free) free = *size;
      *size = free;
      pack->reserved_size = free;
      pack->reserved_start = pack->r1_start + pack->r1_size;
      *area = pack->_data + pack->reserved_start;
    } else {
      if(pack->r1_start == 0) {
        *size = 0;
        mixed_err(MIXED_BUFFER_FULL);
        return 0;
      }
      if(pack->r1_start < *size) *size = pack->r1_start;
      pack->reserved_size = *size;
      pack->reserved_start = 0;
      *area = pack->_data;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_pack_finish_write(uint32_t size, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  if(pack->reserved_size < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  if(size == 0){
    pack->reserved_size = 0;
    pack->reserved_start = 0;
  }else if(pack->r1_size == 0 && pack->r2_size == 0){
    pack->r1_start = pack->reserved_start;
    pack->r1_size = size;
  }else if(pack->reserved_start == pack->r1_start + pack->r1_size){
    pack->r1_size += size;
  }else{
    pack->r2_size += size;
  }
  pack->reserved_start = 0;
  pack->reserved_size = 0;
  return 1;
}

MIXED_EXPORT int mixed_pack_request_read(void **area, uint32_t *size, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  if(pack->r1_size == 0){
    *size = 0;
    mixed_err(MIXED_BUFFER_EMPTY);
    return 0;
  }
  *size = MIN(*size, pack->r1_size);
  *area = pack->_data + pack->r1_start;
  return 1;
}

MIXED_EXPORT uint32_t mixed_pack_available_read(struct mixed_pack *pack){
  return pack->r1_size;
}

MIXED_EXPORT uint32_t mixed_pack_available_write(struct mixed_pack *pack){
  return (pack->r2_size)? free_for_r2(pack) : free_after_r1(pack);
}

MIXED_EXPORT int mixed_pack_finish_read(uint32_t size, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  if(pack->r1_size < size){
    mixed_err(MIXED_BUFFER_OVERCOMMIT);
    return 0;
  }
  if(pack->r1_size == size){
    pack->r1_start = pack->r2_start;
    pack->r1_size = pack->r2_size;
    pack->r2_start = 0;
    pack->r2_size = 0;
  }else{
    pack->r1_size -= size;
    pack->r1_start += size;
  }
  return 1;
}
