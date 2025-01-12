#include "internal.h"
#include "bip.h"

MIXED_EXPORT int mixed_make_pack(uint32_t frames, struct mixed_pack *pack){
  mixed_err(MIXED_NO_ERROR);
  pack->_data = mixed_calloc(frames*pack->channels, mixed_samplesize(pack->encoding));
  if(!pack->_data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  pack->size = frames*pack->channels*mixed_samplesize(pack->encoding);
  return 1;
}

MIXED_EXPORT void mixed_free_pack(struct mixed_pack *pack){
  if(pack->_data)
    mixed_free(pack->_data);
  pack->_data = 0;
  pack->size = 0;
  mixed_pack_clear(pack);
}

MIXED_EXPORT int mixed_pack_clear(struct mixed_pack *pack){
  bip_discard((struct bip*)pack);
  return 1;
}

MIXED_EXPORT int mixed_pack_request_write(void **area, uint32_t *size, struct mixed_pack *pack){
  uint32_t off = 0;
  if(!bip_request_write(&off, size, (struct bip*)pack))
     return 0;
  *area = pack->_data+off;
  return 1;
}

MIXED_EXPORT int mixed_pack_finish_write(uint32_t size, struct mixed_pack *pack){
  return bip_finish_write(size, (struct bip*)pack);
}

MIXED_EXPORT int mixed_pack_request_read(void **area, uint32_t *size, struct mixed_pack *pack){
  uint32_t off = 0;
  if(!bip_request_read(&off, size, (struct bip*)pack))
    return 0;
  *area = pack->_data+off;
  return 1;
}

MIXED_EXPORT int mixed_pack_finish_read(uint32_t size, struct mixed_pack *pack){
  return bip_finish_read(size, (struct bip*)pack);
}

MIXED_EXPORT uint32_t mixed_pack_available_read(struct mixed_pack *pack){
  return bip_available_read((struct bip*)pack);
}

MIXED_EXPORT uint32_t mixed_pack_available_write(struct mixed_pack *pack){
  return bip_available_write((struct bip*)pack);
}
