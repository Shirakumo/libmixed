#include <string.h>
#include "tester.h"
#define __TEST_SUITE pack

int make_pack(enum mixed_encoding encoding, struct mixed_packed_audio *pack){
  pack->data = calloc(16, mixed_samplesize(encoding));
  if(pack->data == 0) return 0;
  pack->size = 16;
  pack->encoding = encoding;
  pack->channels = 1;
  pack->layout = MIXED_ALTERNATING;
  pack->samplerate = 1;
  return 1;
}

define_test(int8, {
    struct mixed_packed_audio pack = {0};
    struct mixed_buffer buffer = {0};
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, &pack));
    pass(mixed_make_buffer(16, &buffer));
    int8_t *data = (int8_t *)pack.data;
    for(int i=0; i<pack.size; ++i)
      data[i] = rand()%128;
    // Convert to buffers
    pass(mixed_buffer_from_packed_audio(&pack, &buffer, &pack.size, 1.0));
    for(int i=0; i<pack.size; ++i)
      is(buffer._data[i], mixed_from_int8(data[i]));
    is(mixed_buffer_available_read(&buffer), pack.size);
    // Convert from buffers
    pass(mixed_buffer_to_packed_audio(&buffer, &pack, &pack.size, 1.0));
    for(int i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffer._data[i]));
    is(mixed_buffer_available_read(&buffer), 0);

  cleanup:
    mixed_free_buffer(&buffer);
    free(data);
  })

#undef __TEST_SUITE
