#include <string.h>
#include "tester.h"
#define __TEST_SUITE pack

int make_pack(enum mixed_encoding encoding, int channels, struct mixed_packed_audio *pack){
  pack->data = calloc(16, mixed_samplesize(encoding));
  if(pack->data == 0) return 0;
  pack->size = 16;
  pack->encoding = encoding;
  pack->channels = channels;
  pack->layout = MIXED_ALTERNATING;
  pack->samplerate = 1;
  return 1;
}

define_test(single_channel, {
    struct mixed_packed_audio pack = {0};
    struct mixed_buffer buffer = {0};
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 1, &pack));
    pass(mixed_make_buffer(16, &buffer));
    int8_t *data = (int8_t *)pack.data;
    for(int i=0; i<pack.size; ++i)
      data[i] = rand()%128;
    // Convert to buffers
    pass(mixed_buffer_from_packed_audio(&pack, &buffer, &pack.size, 1.0));
    is(mixed_buffer_available_read(&buffer), pack.size);
    for(int i=0; i<pack.size; ++i)
      is(buffer._data[i], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_packed_audio(&buffer, &pack, &pack.size, 1.0));
    for(int i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffer._data[i]));
    is(mixed_buffer_available_read(&buffer), 0);

  cleanup:
    mixed_free_buffer(&buffer);
    free(data);
  })

define_test(dual_channel, {
    struct mixed_packed_audio pack = {0};
    struct mixed_buffer buffers[2] = {0};
    struct mixed_buffer *barray[2];
    barray[0] = &buffers[0];
    barray[1] = &buffers[1];
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 2, &pack));
    pass(mixed_make_buffer(8, &buffers[0]));
    pass(mixed_make_buffer(8, &buffers[1]));
    int8_t *data = (int8_t *)pack.data;
    for(int i=0; i<pack.size; ++i)
      data[i] = rand()%128;
    // Convert to buffers
    pass(mixed_buffer_from_packed_audio(&pack, barray, &pack.size, 1.0));
    is(mixed_buffer_available_read(&buffers[0]), buffers[0].size);
    is(mixed_buffer_available_read(&buffers[1]), buffers[1].size);
    for(int i=0; i<pack.size; ++i)
      is(buffers[i%2]._data[i/2], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_packed_audio(barray, &pack, &pack.size, 1.0));
    for(int i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffers[i%2]._data[i/2]));
    is(mixed_buffer_available_read(&buffers[0]), 0);
    is(mixed_buffer_available_read(&buffers[1]), 0);

  cleanup:
    mixed_free_buffer(&buffers[0]);
    mixed_free_buffer(&buffers[1]);
    free(data);
  })

define_test(dual_channel_uneven, {
    struct mixed_packed_audio pack = {0};
    struct mixed_buffer buffers[2] = {0};
    struct mixed_buffer *barray[2];
    barray[0] = &buffers[0];
    barray[1] = &buffers[1];
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 2, &pack));
    pass(mixed_make_buffer(4, &buffers[0]));
    pass(mixed_make_buffer(8, &buffers[1]));
    int8_t *data = (int8_t *)pack.data;
    for(int i=0; i<pack.size; ++i)
      data[i] = rand()%128;
    // Convert to buffers
    pass(mixed_buffer_from_packed_audio(&pack, barray, &pack.size, 1.0));
    is(mixed_buffer_available_read(&buffers[0]), buffers[0].size);
    is(mixed_buffer_available_read(&buffers[1]), buffers[0].size);
    for(int i=0; i<pack.size; ++i)
      is(buffers[i%2]._data[i/2], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_packed_audio(barray, &pack, &pack.size, 1.0));
    for(int i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffers[i%2]._data[i/2]));
    is(mixed_buffer_available_read(&buffers[0]), 0);
    is(mixed_buffer_available_read(&buffers[1]), 0);

  cleanup:
    mixed_free_buffer(&buffers[0]);
    mixed_free_buffer(&buffers[1]);
    free(data);
  })

#undef __TEST_SUITE
