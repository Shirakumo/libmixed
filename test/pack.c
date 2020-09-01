#define __TEST_SUITE pack
#include <string.h>
#include <pthread.h>
#include "tester.h"

define_test(make, {
    struct mixed_pack pack = {0};
    pack.channels = 2;
    pack.encoding = MIXED_UINT24;
    pack.samplerate = 1;
    pass(mixed_make_pack(1024, &pack));
    is(pack.size, 1024*2*3);
    
  cleanup:
    mixed_free_pack(&pack);
  });

define_test(randomized, {
    struct mixed_pack pack = {0};
    pack.channels = 1;
    pack.encoding = MIXED_UINT8;
    pack.samplerate = 1;
    uint32_t size = 1024;
    uint32_t wptr = 0, rptr = 0;
    int32_t full = 0;
    
    pass(mixed_make_pack(size, &pack));

    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      void *area = 0;
      uint32_t avail = UINT32_MAX;
      if(rand() % 2 == 0){
        mixed_pack_request_write(&area, &avail, &pack);
        avail = rand() % (avail+1);
        pass(mixed_pack_finish_write(avail, &pack));
        wptr = (wptr + avail) % size;
        full += avail;
      }else{
        mixed_pack_request_read(&area, &avail, &pack);
        avail = rand() % (avail+1);
        pass(mixed_pack_finish_read(avail, &pack));
        rptr = (rptr + avail) % size;
        full -= avail;
      }
      avail = UINT32_MAX;
      if(size < full) fail("Over/Underflow");
      if(rptr < wptr){
        mixed_pack_request_read(&area, &avail, &pack);
        is(avail, wptr-rptr);
        is(mixed_pack_available_read(&pack), wptr-rptr);
        is(mixed_pack_available_write(&pack), size-wptr);
      }else if(wptr < rptr){
        mixed_pack_request_read(&area, &avail, &pack);
        is(avail, size-rptr);
        is(mixed_pack_available_read(&pack), size-rptr);
        is(mixed_pack_available_write(&pack), rptr-wptr);
      }else if(full == 0){
        mixed_pack_request_read(&area, &avail, &pack);
        is(avail, 0);
        is(mixed_pack_available_read(&pack), 0);
        is(mixed_pack_available_write(&pack), size-wptr);
      }else{
        mixed_pack_request_read(&area, &avail, &pack);
        is(avail, size-rptr);
        is(mixed_pack_available_write(&pack), 0);
        is(mixed_pack_available_read(&pack), size-rptr);
      }
    }

  cleanup:
    mixed_free_pack(&pack);
  });
