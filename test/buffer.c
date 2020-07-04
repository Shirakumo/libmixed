#include "tester.h"
#define __TEST_SUITE buffer

define_test(make, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(write_allocation, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area=0, *area2=0, *area3=0;
    size_t size = 512;
    // Write half
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 512);
    isnt_p(area, 0);
    pass(mixed_buffer_finish_write(size, &buffer));
    // Write half
    pass(mixed_buffer_request_write(&area2, &size, &buffer));
    is(size, 512);
    isnt_p(area, area2);
    pass(mixed_buffer_finish_write(size, &buffer));
    // Buffer now full
    fail(mixed_buffer_request_write(&area3, &size, &buffer));
    is(size, 0);
    is_p(area3, 0);
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(full_read_write, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area=0, *area2=0;
    size_t size=1024, size2=1024;
    // Request full write
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 1024);
    // Read before commit
    fail(mixed_buffer_request_read(&area2, &size2, &buffer));
    is(size2, 0);
    is_p(area2, 0);
    // Read after commit
    pass(mixed_buffer_finish_write(size, &buffer));
    size2 = 1024;
    pass(mixed_buffer_request_read(&area2, &size2, &buffer));
    isnt_p(area2, 0);
    // Reading twice is fine
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, size2);
    is_p(area, area2);
    // Finishing twice is not
    pass(mixed_buffer_finish_read(size, &buffer));
    fail(mixed_buffer_finish_read(size2, &buffer));
    // Buffer now empty
    fail(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, 0);
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(partial_read_write, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *w_area=0, *r_area=0;
    size_t w_size=512, r_size=256;
    // Write to buffer a bit
    pass(mixed_buffer_request_write(&w_area, &w_size, &buffer));
    is(w_size, 512);
    pass(mixed_buffer_finish_write(256, &buffer));
    // Start reading back
    pass(mixed_buffer_request_read(&r_area, &r_size, &buffer));
    is(r_size, 256);
    // Write some more
    pass(mixed_buffer_request_write(&w_area, &w_size, &buffer));
    is(w_size, 512);
    isnt_p(r_area, w_area);
    // Try to read the newly written region
    pass(mixed_buffer_finish_read(r_size, &buffer));
    fail(mixed_buffer_request_read(&r_area, &r_size, &buffer));
    pass(mixed_buffer_finish_write(w_size, &buffer));
    pass(mixed_buffer_request_read(&r_area, &r_size, &buffer));
    pass(mixed_buffer_finish_read(r_size, &buffer));
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

#undef __TEST_SUITE
