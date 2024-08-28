#define __TEST_SUITE buffer
#include <string.h>
#include <pthread.h>
#include "tester.h"

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
    uint32_t size = 512;
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
    mixed_buffer_request_write(&area3, &size, &buffer);
    is(size, 0);
    is_p(area3, 0);
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(full_read_write, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area=0, *area2=0;
    uint32_t size=1024, size2=1024;
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
    uint32_t w_size=512, r_size=256;
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

define_test(read_bounds, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(1024, &buffer));
    float *area;
    uint32_t size=1024;
    // Fill buffer
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 1024);
    pass(mixed_buffer_finish_write(size, &buffer));
    // Read back
    size = 1000;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, 1000);
    pass(mixed_buffer_finish_read(size, &buffer));
    size = 24;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, 24);
    pass(mixed_buffer_finish_read(size, &buffer));
    // Write more
    size = 1024;
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, 1024);
    pass(mixed_buffer_finish_write(size, &buffer));
    // Read more, wrapping around.
    size = 200;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, 200);
    pass(mixed_buffer_finish_read(size, &buffer));
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(bip_read_write, {
    struct mixed_buffer buffer = {0};
    pass(mixed_make_buffer(100, &buffer));
    float *area=0;
    uint32_t size=UINT32_MAX;
    // Write full
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, buffer.size);
    pass(mixed_buffer_finish_write(size, &buffer));
    // Read half
    size = UINT32_MAX;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, buffer.size);
    pass(mixed_buffer_finish_read(size/2, &buffer));
    is(mixed_buffer_available_read(&buffer), buffer.size/2);
    is(mixed_buffer_available_write(&buffer), buffer.size/2);
    // Write a quarter
    size = UINT32_MAX;
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, buffer.size/2);
    pass(mixed_buffer_finish_write(size/2, &buffer));
    is(mixed_buffer_available_read(&buffer), buffer.size/2);
    is(mixed_buffer_available_write(&buffer), buffer.size/4);
    // Read a quarter
    size = UINT32_MAX;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, buffer.size/2);
    pass(mixed_buffer_finish_read(size/2, &buffer));
    is(mixed_buffer_available_read(&buffer), buffer.size/4);
    is(mixed_buffer_available_write(&buffer), buffer.size/2);
    // Write up to read
    size = UINT32_MAX;
    pass(mixed_buffer_request_write(&area, &size, &buffer));
    is(size, buffer.size/2);
    pass(mixed_buffer_finish_write(size, &buffer));
    is(mixed_buffer_available_read(&buffer), buffer.size/4);
    is(mixed_buffer_available_write(&buffer), 0);
    // Read the rest of the second part
    size = UINT32_MAX;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, buffer.size/4);
    pass(mixed_buffer_finish_read(size, &buffer));
    is(mixed_buffer_available_read(&buffer), buffer.size*3/4);
    is(mixed_buffer_available_write(&buffer), buffer.size/4);
    // Read up to write
    size = UINT32_MAX;
    pass(mixed_buffer_request_read(&area, &size, &buffer));
    is(size, buffer.size*3/4);
    pass(mixed_buffer_finish_read(size, &buffer));
    is(mixed_buffer_available_read(&buffer), 0);
    is(mixed_buffer_available_write(&buffer), buffer.size/4);
    
  cleanup:
    mixed_free_buffer(&buffer);
  });

define_test(transfer, {
    struct mixed_buffer a={0}, b={0};
    float *area;
    uint32_t size = 1024;
    float mem[size];
    // Allocate
    pass(mixed_make_buffer(size, &a));
    pass(mixed_make_buffer(size, &b));
    for(int i=0; i<1024; i++)
      mem[i] = rand();
    // Write to a
    pass(mixed_buffer_request_write(&area, &size, &a));
    memcpy(area, mem, sizeof(float)*size);
    pass(mixed_buffer_finish_write(size, &a));
    // Transfer
    pass(mixed_buffer_transfer(&a, &b));
    // Check contents of b
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);
    // Ensure a is empty
    fail(mixed_buffer_request_read(&area, &size, &a));
    // Transfer from b to b
    pass(mixed_buffer_transfer(&b, &b));
    // Check b has stuff to read from
    size = UINT32_MAX;
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);

  cleanup:
    mixed_free_buffer(&a);
    mixed_free_buffer(&b);
  });

define_test(copy, {
    struct mixed_buffer a={0}, b={0};
    float *area;
    uint32_t size = 1024;
    float mem[size];
    // Allocate
    pass(mixed_make_buffer(size, &a));
    pass(mixed_make_buffer(size, &b));
    for(int i=0; i<1024; i++)
      mem[i] = rand();
    // Write to a
    pass(mixed_buffer_request_write(&area, &size, &a));
    memcpy(area, mem, sizeof(float)*size);
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
    uint32_t w_size=512, r_size=0;
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

define_test(with_transfer, {
    float *area;
    uint32_t size = 1024;
    float mem[size];
    struct mixed_buffer a = {0}, b = {0};
    pass(mixed_make_buffer(size, &a));
    pass(mixed_make_buffer(size, &b));
    for(uint32_t i=0; i<size; i++)
      mem[i] = rand();
    // Write to a
    pass(mixed_buffer_request_write(&area, &size, &a));
    memcpy(area, mem, sizeof(float)*size);
    pass(mixed_buffer_finish_write(size, &a));
    // Transfer
    with_mixed_buffer_transfer(i, samples, af, &a, bf, &b, {
        is(samples, 1024);
        is_f(af[i], mem[i]);
        bf[i] = af[i];
      });
    // Check
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    is(memcmp(mem, area, sizeof(float)*size), 0);
    // Write b->b
    with_mixed_buffer_transfer(i, samples, af, &b, bf, &b, {
        is(samples, 1024);
        is_f(af[i], mem[i]);
        bf[i] = af[i]+1;
      });
    // Check
    pass(mixed_buffer_request_read(&area, &size, &b));
    is(size, 1024);
    for(uint32_t i=0; i<size; ++i)
      is_f(area[i], mem[i]+1);
    
  cleanup:
    mixed_free_buffer(&a);
    mixed_free_buffer(&b);
  });

define_test(randomized, {
    struct mixed_buffer buffer = {0};
    uint32_t size = 1024;
    uint32_t wptr = 0, rptr = 0, full = 0;
    
    pass(mixed_make_buffer(size, &buffer));

    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      float *area = 0;
      uint32_t avail = rand() % size;
      uint32_t prev_avail = avail;
      if(rand() % 2 == 0){
        mixed_buffer_request_write(&area, &avail, &buffer);
        if(prev_avail < avail) fail("Write more than requested.");
        if(avail) isnt_p(area, 0);
        avail = rand() % (avail+1);
        pass(mixed_buffer_finish_write(avail, &buffer));
        wptr = (wptr + avail) % size;
        full += avail;
      }else{
        mixed_buffer_request_read(&area, &avail, &buffer);
        if(prev_avail < avail) fail("Read more than requested.");
        if(avail) isnt_p(area, 0);
        avail = rand() % (avail+1);
        pass(mixed_buffer_finish_read(avail, &buffer));
        rptr = (rptr + avail) % size;
        full -= avail;
      }
      avail = UINT32_MAX;
      if(size < full) fail("Over/Underflow");
      if(rptr < wptr){
        mixed_buffer_request_read(&area, &avail, &buffer);
        is(avail, wptr-rptr);
        is(mixed_buffer_available_read(&buffer), wptr-rptr);
        is(mixed_buffer_available_write(&buffer), size-wptr);
      }else if(wptr < rptr){
        mixed_buffer_request_read(&area, &avail, &buffer);
        is(avail, size-rptr);
        is(mixed_buffer_available_read(&buffer), size-rptr);
        is(mixed_buffer_available_write(&buffer), rptr-wptr);
      }else if(full == 0){
        mixed_buffer_request_read(&area, &avail, &buffer);
        is(avail, 0);
        is(mixed_buffer_available_read(&buffer), 0);
        is(mixed_buffer_available_write(&buffer), size-wptr);
      }else{
        mixed_buffer_request_read(&area, &avail, &buffer);
        is(avail, size-rptr);
        is(mixed_buffer_available_write(&buffer), 0);
        is(mixed_buffer_available_read(&buffer), size-rptr);
      }
    }

  cleanup:
    mixed_free_buffer(&buffer);
  });

void *async_reader(struct mixed_buffer *buffer){
  uint32_t *status = calloc(sizeof(uint32_t), 1);
  uint32_t size = buffer->size;
  *status = 0;
  for(int i=0; i<RANDOMIZED_REPEAT; ++i){
    float *area = 0;
    uint32_t read = rand() % size;
    mixed_buffer_request_read(&area, &read, buffer);
    if(buffer->_data+size < area+read)
      *status = 1;
    if(!mixed_buffer_finish_read(read, buffer))
      *status = 2;
    if(*status != 0)
      break;
  }
  return status;
}

define_test(async_read_write, {
    struct mixed_buffer buffer = {0};
    uint32_t size = 1024;
    pthread_t reader = 0;
    uint32_t *status = 0;
    pass(mixed_make_buffer(size, &buffer));

    if(pthread_create(&reader, 0, (void *(*)(void *))async_reader, &buffer) != 0){
      fail_test("Failed to spawn thread.");
    }

    for(int i=0; i<RANDOMIZED_REPEAT; ++i){
      float *area = 0;
      uint32_t write = rand() % size;
      mixed_buffer_request_write(&area, &write, &buffer);
      if(buffer._data+size < area+write){
        fail_test("Writer thread failed with bad read size: [%p, %i] vs [%p, %i] (+%i)",
                  buffer._data, size, area, write, (area+write)-(buffer._data+size));
      }
      pass(mixed_buffer_finish_write(write, &buffer));
    }

    pthread_join(reader, (void **)&status);
    reader = 0;
    if(*status != 0){
      fail_test("Reader thread failed with exit code %i", *status);
    }
    
  cleanup:
    if(reader) pthread_cancel(reader);
    if(status) free(status);
    mixed_free_buffer(&buffer);
  })

#undef __TEST_SUITE
