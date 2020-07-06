#include <string.h>
#include "tester.h"
#define __TEST_SUITE buffer

define_test(make, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    mixed_free_buffer(&buffer);
    
  cleanup:;
  });

define_test(write_allocation, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area=0, *area2=0, *area3=0;
    size_t size = 512;
    is(mixed_buffer_available_write(&buffer), 1024);
    is(mixed_buffer_available_read(&buffer), 0);
    // Write half
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 512);
    isnt_p(area, 0);
    pass(mixed_buffer_finish_write(size, &buffer));
    is(mixed_buffer_available_write(&buffer), 512);
    is(mixed_buffer_available_read(&buffer), 512);
    // Write half
    pass(mixed_buffer_request_write(&area2, &size, &buffer));
    is(size, 512);
    isnt_p(area, area2);
    pass(mixed_buffer_finish_write(size, &buffer));
    is(mixed_buffer_available_write(&buffer), 0);
    is(mixed_buffer_available_read(&buffer), 1024);
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

define_test(transfer, {
    struct mixed_buffer a={0}, b={0};
    float *area;
    size_t size = 1024;
    float mem[size];
    // Allocate
    pass(mixed_make_buffer(size, &a));
    pass(mixed_make_buffer(size, &b));
    for(int i=0; i<1024; i++)
      mem[i] = rand();
    // Write to a
    pass(mixed_buffer_request_write(&area, &size, &a));
    memcpy(mem, area, sizeof(float)*size);
    pass(mixed_buffer_finish_write(size, &a));
    // Transfer
    pass(mixed_buffer_transfer(&a, &b));
    // Check contents of b
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);
    // Ensure a is empty
    fail(mixed_buffer_request_read(&area, &size, &a));

  cleanup:
    mixed_free_buffer(&a);
    mixed_free_buffer(&b);
  });

define_test(copy, {
    struct mixed_buffer a={0}, b={0};
    float *area;
    size_t size = 1024;
    float mem[size];
    // Allocate
    pass(mixed_make_buffer(size, &a));
    pass(mixed_make_buffer(size, &b));
    for(int i=0; i<1024; i++)
      mem[i] = rand();
    // Write to a
    pass(mixed_buffer_request_write(&area, &size, &a));
    memcpy(mem, area, sizeof(float)*size);
    pass(mixed_buffer_finish_write(size, &a));
    // Transfer
    pass(mixed_buffer_copy(&a, &b));
    // Check contents of b
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);
    // Ensure a is full
    pass(mixed_buffer_request_read(&area, &size, &a));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);
    
  cleanup:
    mixed_free_buffer(&a);
    mixed_free_buffer(&b);
  });

define_test(resize, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *w_area=0, *r_area=0;
    size_t w_size=512, r_size=0;
    // Write to buffer a bit
    pass(mixed_buffer_request_write(&w_area, &w_size, &buffer));
    pass(mixed_buffer_finish_write(256, &buffer));
    // Start reading back
    pass(mixed_buffer_request_read(&r_area, &r_size, &buffer));
    // Write some more
    pass(mixed_buffer_request_write(&w_area, &w_size, &buffer));
    // Resize
    pass(mixed_buffer_resize(2048, &buffer));
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

#undef __TEST_SUITE
