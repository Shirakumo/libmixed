#define __TEST_SUITE distribute
#include "tester.h"

define_test(distribute, {
    struct mixed_buffer buffer = {0}, out1 = {0}, out2 = {0};
    struct mixed_segment distribute = {0};
    // Allocate stuff
    pass(mixed_make_buffer(100, &buffer));
    pass(mixed_make_segment_distribute(&distribute));
    // Connect the buffers
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &distribute));
    pass(mixed_segment_set_out(MIXED_BUFFER, 0, &out1, &distribute));
    pass(mixed_segment_set_out(MIXED_BUFFER, 1, &out2, &distribute));
    // Fill
    float *data, *data1, *data2;
    uint32_t samples = UINT32_MAX;
    pass(mixed_buffer_request_write(&data, &samples, &buffer));
    for(uint32_t i=0; i<samples; ++i)
      data[i] = (float)i;
    pass(mixed_buffer_finish_write(samples, &buffer));
    // Run
    pass(mixed_segment_start(&distribute));
    pass(mixed_segment_mix(&distribute));
    // Check
    is(mixed_buffer_available_read(&out1), mixed_buffer_available_read(&buffer));
    is(mixed_buffer_available_read(&out2), mixed_buffer_available_read(&buffer));
    pass(mixed_buffer_request_read(&data, &samples, &buffer));
    pass(mixed_buffer_request_read(&data1, &samples, &out1));
    pass(mixed_buffer_request_read(&data2, &samples, &out2));
    for(size_t i=0; i<samples; ++i){
      is_f(data1[i], data[i]);
      is_f(data2[i], data[i]);
    }
    pass(mixed_buffer_finish_read(samples/2, &out1));
    pass(mixed_buffer_finish_read(samples/2, &out2));
    // Run again
    pass(mixed_segment_mix(&distribute));
    // Check we actually consumed
    is(mixed_buffer_available_read(&buffer), samples/2);
    is(mixed_buffer_available_read(&out1), mixed_buffer_available_read(&buffer));
    is(mixed_buffer_available_read(&out2), mixed_buffer_available_read(&buffer));

  cleanup:
    mixed_free_segment(&distribute);
    mixed_free_buffer(&buffer);
  })

define_test(distribute_varying, {
    struct mixed_buffer buffer = {0}, out1 = {0}, out2 = {0};
    struct mixed_segment distribute = {0};
    // Allocate stuff
    pass(mixed_make_buffer(100, &buffer));
    pass(mixed_make_segment_distribute(&distribute));
    // Connect the buffers
    pass(mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &distribute));
    pass(mixed_segment_set_out(MIXED_BUFFER, 0, &out1, &distribute));
    pass(mixed_segment_set_out(MIXED_BUFFER, 1, &out2, &distribute));
    // Fill
    float *data;
    uint32_t samples = UINT32_MAX;
    pass(mixed_buffer_request_write(&data, &samples, &buffer));
    for(uint32_t i=0; i<samples; ++i)
      data[i] = (float)i;
    pass(mixed_buffer_finish_write(samples, &buffer));
    // Run
    pass(mixed_segment_start(&distribute));
    pass(mixed_segment_mix(&distribute));
    // Check
    pass(mixed_buffer_finish_read(samples/2, &out1));
    pass(mixed_buffer_finish_read(samples/4, &out2));
    // Run again
    pass(mixed_segment_mix(&distribute));
    // Check we actually consumed
    is(mixed_buffer_available_read(&buffer), samples*3/4);
    is(mixed_buffer_available_read(&out1), mixed_buffer_available_read(&buffer));
    is(mixed_buffer_available_read(&out2), mixed_buffer_available_read(&buffer));

  cleanup:
    mixed_free_segment(&distribute);
    mixed_free_buffer(&buffer);
  })
#undef __TEST_SUITE
