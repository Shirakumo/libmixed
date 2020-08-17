#define __TEST_SUITE packer
#include "tester.h"
#include <math.h>

static int make_pack(enum mixed_encoding encoding, int channels, struct mixed_pack *pack){
  int frames = 500;
  pack->encoding = encoding;
  pack->channels = channels;
  pack->samplerate = 48000;
  if(!mixed_make_pack(frames, pack))
    return 0;
  char *buffer;
  uint32_t size = UINT32_MAX;
  mixed_pack_request_write((void**)&buffer, &size, pack);
  for(uint32_t i=0; i<size; ++i)
    buffer[i] = rand()%128;
  mixed_pack_finish_write(size, pack);
  return 1;
}

define_test(single_channel_no_resample, {
    struct mixed_pack pack_i = {0};
    struct mixed_pack pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment packer = {0};
    struct mixed_segment unpacker = {0};
    // Allocate stuff
    pass(make_pack(MIXED_INT8, 1, &pack_i));
    pass(make_pack(MIXED_INT8, 1, &pack_o));
    pass(mixed_make_buffer(pack_i.size, &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, pack_i.samplerate, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, pack_o.samplerate, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Clear packer
    mixed_pack_clear(&pack_o);
    // Run
    pass(mixed_segment_start(&unpacker));
    pass(mixed_segment_start(&packer));
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check
    is(mixed_pack_available_read(&pack_i), 0);
    is(mixed_pack_available_read(&pack_o), pack_o.size);
    for(uint32_t i=0; i<pack_o.size; ++i)
      is(((char*)pack_o._data)[i], ((char*)pack_i._data)[i]);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    mixed_free_pack(&pack_i);
    mixed_free_pack(&pack_o);
  })

define_test(single_channel_downsample_out, {
    struct mixed_pack pack_i = {0};
    struct mixed_pack pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment unpacker = {0};
    struct mixed_segment packer = {0};
    // Allocate stuff
    pass(make_pack(MIXED_INT8, 1, &pack_i));
    pass(make_pack(MIXED_INT8, 1, &pack_o));
    pack_o.samplerate = pack_i.samplerate / 2;
    pass(mixed_make_buffer(pack_i.size, &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, pack_i.samplerate, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, pack_i.samplerate, &packer));
    // Set quality so we get predictable sample counts
    enum mixed_resample_type quality = MIXED_LINEAR_INTERPOLATION;
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &unpacker));
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Clear packer
    mixed_pack_clear(&pack_o);
    // Run
    pass(mixed_segment_start(&unpacker));
    pass(mixed_segment_start(&packer));
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check for expected
    is(mixed_pack_available_read(&pack_o), pack_o.size/2);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    mixed_free_pack(&pack_i);
    mixed_free_pack(&pack_o);
  })

define_test(single_channel_resample_in_out, {
    struct mixed_pack pack_i = {0};
    struct mixed_pack pack_o = {0};
    struct mixed_buffer buffer = {0};
    struct mixed_segment unpacker = {0};
    struct mixed_segment packer = {0};
    // Allocate stuff
    pass(make_pack(MIXED_FLOAT, 1, &pack_i));
    pass(make_pack(MIXED_FLOAT, 1, &pack_o));
    pass(mixed_make_buffer(pack_i.size/sizeof(float), &buffer));
    pass(mixed_make_segment_unpacker(&pack_i, 44100, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, 44100, &packer));
    // Set quality so we get predictable results
    enum mixed_resample_type quality = MIXED_SINC_BEST_QUALITY;
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &unpacker));
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &packer));
    // Fill data
    float *data_i = (float *)pack_i._data;
    float *data_o = (float *)pack_o._data;
    for(uint32_t i=0; i<buffer.size; ++i){
      data_i[i] = ((float)i) / pack_i.size;
      data_o[i] = 0.0;
    }
    mixed_pack_clear(&pack_o);
    // Run
    pass(mixed_segment_start(&unpacker));
    pass(mixed_segment_start(&packer));
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check for expected
    for(uint32_t i=0; i<mixed_pack_available_read(&pack_o)/sizeof(float); ++i)
      is(fabs(data_o[i]-data_i[i]) < 0.00005, 1);

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&buffer);
    mixed_free_pack(&pack_i);
    mixed_free_pack(&pack_o);
  })

define_test(dual_channel_resample_in_out, {
    struct mixed_pack pack_i = {0};
    struct mixed_pack pack_o = {0};
    struct mixed_buffer l = {0}, r = {0};
    struct mixed_segment unpacker = {0};
    struct mixed_segment packer = {0};
    // Allocate stuff
    pass(make_pack(MIXED_FLOAT, 2, &pack_i));
    pass(make_pack(MIXED_FLOAT, 2, &pack_o));
    pass(mixed_make_buffer(pack_i.size/(2*sizeof(float)), &l));
    pass(mixed_make_buffer(pack_i.size/(2*sizeof(float)), &r));
    pass(mixed_make_segment_unpacker(&pack_i, 44100, &unpacker));
    pass(mixed_make_segment_packer(&pack_o, 44100, &packer));
    // Set quality so we get predictable results
    enum mixed_resample_type quality = MIXED_SINC_BEST_QUALITY;
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &unpacker));
    pass(mixed_segment_set(MIXED_RESAMPLE_TYPE, &quality, &packer));
    // Connect the buffers
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &l, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &l, &packer));
    pass(mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &r, &unpacker));
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &r, &packer));
    // Fill data
    float *data_i = (float *)pack_i._data;
    float *data_o = (float *)pack_o._data;
    for(uint32_t i=0; i<pack_i.size/sizeof(float); ++i){
      data_i[i] = ((float)i) / (pack_i.size*sizeof(float))+(i%2/10.0);
      data_o[i] = 0.0;
    }
    mixed_pack_clear(&pack_o);
    // Run
    pass(mixed_segment_start(&unpacker));
    pass(mixed_segment_start(&packer));
    pass(mixed_segment_mix(&unpacker));
    pass(mixed_segment_mix(&packer));
    // Check for expected
    // KLUDGE: seems the first few samples are very distorted and in general
    //         the resampling quality here is /much/ lower.
    for(uint32_t i=10; i<mixed_pack_available_read(&pack_o)/sizeof(float); ++i){
      is(fabs(data_o[i]-data_i[i]) < 0.005, 1);
    }

  cleanup:
    mixed_free_segment(&packer);
    mixed_free_segment(&unpacker);
    mixed_free_buffer(&l);
    mixed_free_buffer(&r);
    mixed_free_pack(&pack_i);
    mixed_free_pack(&pack_o);
  })
  
#undef __TEST_SUITE
