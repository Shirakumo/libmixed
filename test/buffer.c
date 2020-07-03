#include "tester.h"
#define __TEST_SUITE buffer

define_test(allocate, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
  cleanup:;
  });

define_test(write, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area;
    size_t size = 512;
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 512);
    pass(mixed_buffer_finish_write(size, &buffer));
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(over_request, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area;
    size_t size = 2048;
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 1024);
    pass(mixed_buffer_finish_write(size, &buffer));
  cleanup:
    mixed_free_buffer(&buffer);
  });

#undef __TEST_SUITE
