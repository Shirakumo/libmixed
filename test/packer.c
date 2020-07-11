#define __TEST_SUITE packer
#include "tester.h"

static int make_pack(enum mixed_encoding encoding, int channels, struct mixed_packed_audio *pack){
  int samples = channels*100;
  pack->data = calloc(samples, mixed_samplesize(encoding));
  if(pack->data == 0) return 0;
  pack->size = samples*mixed_samplesize(encoding);
  pack->frames = samples / channels;
  pack->encoding = encoding;
  pack->channels = channels;
  pack->samplerate = 100;
  return 1;
}

define_test(single_channel_no_resample, {
    struct mixed_packed_audio pack_i = {0};
    struct mixed_packed_audio pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment packer = {0};
    struct mixed_segment unpacker = {0};
    // Allocate stuff
    pass(make_pack(MIXED_INT8, 1, &pack_i));
    pass(make_pack(MIXED_INT8, 1, &pack_o));
    pass(mixed_make_buffer(pack_i.frames, &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, pack_i.samplerate, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, pack_o.samplerate, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Fill data
    int8_t *data_i = (int8_t *)pack_i.data;
    int8_t *data_o = (int8_t *)pack_o.data;
    for(int i=0; i<pack_i.size; ++i)
      data_i[i] = i % 128;
    // Run
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check
    is(pack_o.frames, pack_i.frames);
    for(int i=0; i<pack_i.size; ++i)
      is(data_o[i], data_i[i]);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    if(pack_i.data) free(pack_i.data);
    if(pack_o.data) free(pack_o.data);
  })

define_test(single_channel_resample_out, {
    struct mixed_packed_audio pack_i = {0};
    struct mixed_packed_audio pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment unpacker = {0};
    struct mixed_segment packer = {0};
    // Allocate stuff
    pass(make_pack(MIXED_INT8, 1, &pack_i));
    pass(make_pack(MIXED_INT8, 1, &pack_o));
    pass(mixed_make_buffer(pack_i.frames, &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, 100, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, 80, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Fill data
    int8_t *data_i = (int8_t *)pack_i.data;
    int8_t *data_o = (int8_t *)pack_o.data;
    for(int i=0; i<pack_i.size; ++i)
      data_i[i] = rand()%128;
    // Run
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check for expected
    is(pack_o.frames, 60);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    if(pack_i.data) free(pack_i.data);
    if(pack_o.data) free(pack_o.data);
  })

define_test(single_channel_resample_in_out, {
    struct mixed_packed_audio pack_i = {0};
    struct mixed_packed_audio pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment unpacker = {0};
    struct mixed_segment packer = {0};
    // Allocate stuff
    pass(make_pack(MIXED_INT8, 1, &pack_i));
    pass(make_pack(MIXED_INT8, 1, &pack_o));
    pass(mixed_make_buffer(pack_i.frames, &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, 80, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, 80, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Fill data
    int8_t *data_i = (int8_t *)pack_i.data;
    int8_t *data_o = (int8_t *)pack_o.data;
    for(int i=0; i<pack_i.size; ++i)
      data_i[i] = rand()%128;
    // Run
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check for expected
    is(pack_o.frames, 60);
    for(int i=0; i<pack_o.size; ++i)
      is(data_o[i], data_i[i]);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    if(pack_i.data) free(pack_i.data);
    if(pack_o.data) free(pack_o.data);
  })
  
#undef __TEST_SUITE
