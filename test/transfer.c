#define __TEST_SUITE transfer
#include "tester.h"

static int make_pack(enum mixed_encoding encoding, int channels, struct mixed_pack *pack){
  pack->encoding = encoding;
  pack->channels = channels;
  pack->samplerate = 1;
  if(!mixed_make_pack(16, pack))
    return 0;
  char *buffer;
  uint32_t size = UINT32_MAX;
  mixed_pack_request_write((void**)&buffer, &size, pack);
  for(uint32_t i=0; i<size; ++i)
    buffer[i] = rand()%128;
  mixed_pack_finish_write(size, pack);
  return 1;
}

define_test(single_channel, {
    struct mixed_pack pack = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_buffer *barray[1];
    float volume = 1.0;
    barray[0] = &buffer;
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 1, &pack));
    pass(mixed_make_buffer(16, &buffer));
    unsigned char *data = pack._data;
    // Convert from pack
    pass(mixed_buffer_from_pack(&pack, barray, &volume, 1.0));
    is(mixed_buffer_available_read(&buffer), pack.size);
    is(mixed_pack_available_read(&pack), 0);
    for(uint32_t i=0; i<pack.size; ++i)
      is(buffer._data[i], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_pack(barray, &pack, &volume, 1.0));
    is(mixed_buffer_available_read(&buffer), 0);
    is(mixed_pack_available_read(&pack), pack.size);
    for(uint32_t i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffer._data[i]));

  cleanup:
    mixed_free_buffer(&buffer);
    mixed_free_pack(&pack);
  })

define_test(dual_channel, {
    struct mixed_pack pack = {0};
    struct mixed_buffer buffers[2] = {0};
    struct mixed_buffer *barray[2];
    float volume = 1.0;
    barray[0] = &buffers[0];
    barray[1] = &buffers[1];
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 2, &pack));
    pass(mixed_make_buffer(pack.size/2, &buffers[0]));
    pass(mixed_make_buffer(pack.size/2, &buffers[1]));
    unsigned char *data = pack._data;
    // Convert from pack
    pass(mixed_buffer_from_pack(&pack, barray, &volume, 1.0));
    is(mixed_buffer_available_read(&buffers[0]), buffers[0].size);
    is(mixed_buffer_available_read(&buffers[1]), buffers[1].size);
    is(mixed_pack_available_read(&pack), 0);
    for(uint32_t i=0; i<pack.size; ++i)
      is_f(buffers[i%2]._data[i/2], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_pack(barray, &pack, &volume, 1.0));
    for(uint32_t i=0; i<pack.size; ++i)
      is(data[i], mixed_to_int8(buffers[i%2]._data[i/2]));
    is(mixed_buffer_available_read(&buffers[0]), 0);
    is(mixed_buffer_available_read(&buffers[1]), 0);
    is(mixed_pack_available_read(&pack), pack.size);

  cleanup:
    mixed_free_buffer(&buffers[0]);
    mixed_free_buffer(&buffers[1]);
    mixed_free_pack(&pack);
  })

define_test(dual_channel_uneven, {
    struct mixed_pack pack = {0};
    struct mixed_buffer buffers[2] = {0};
    struct mixed_buffer *barray[2];
    float volume = 1.0;
    barray[0] = &buffers[0];
    barray[1] = &buffers[1];
    // Create pack with random samples
    pass(make_pack(MIXED_INT8, 2, &pack));
    pass(mixed_make_buffer(pack.size/4, &buffers[0]));
    pass(mixed_make_buffer(pack.size/2, &buffers[1]));
    unsigned char *data = pack._data;
    // Convert to buffers
    pass(mixed_buffer_from_pack(&pack, barray, &volume, 1.0));
    is(mixed_buffer_available_read(&buffers[0]), buffers[0].size);
    is(mixed_buffer_available_read(&buffers[1]), buffers[0].size);
    is(mixed_pack_available_read(&pack), pack.size-2*buffers[0].size);
    for(uint32_t i=0; i<8; ++i)
      is_f(buffers[i%2]._data[i/2], mixed_from_int8(data[i]));
    // Convert from buffers
    pass(mixed_buffer_to_pack(barray, &pack, &volume, 1.0));
    for(uint32_t i=0; i<8; ++i)
      is(data[i], mixed_to_int8(buffers[i%2]._data[i/2]));
    is(mixed_buffer_available_read(&buffers[0]), 0);
    is(mixed_buffer_available_read(&buffers[1]), 0);
    is(mixed_pack_available_read(&pack), pack.size/2);

  cleanup:
    mixed_free_buffer(&buffers[0]);
    mixed_free_buffer(&buffers[1]);
    mixed_free_pack(&pack);
  })

define_test(bounds_check, {
    mixed_transfer_function_from decoder = mixed_translator_from(MIXED_INT16);
    mixed_transfer_function_to encoder = mixed_translator_to(MIXED_INT16);
    is_f(decoder(0, 0, 1, 0, 1.0, 1.0), 1.0);
    is_f(encoder(0, 0, 1, 0, 1.0, 1.0), 1.0);
  cleanup: {}
  })

#undef __TEST_SUITE
