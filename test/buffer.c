#include "tester.h"
#define __TEST_SUITE buffer

define_test(allocate, {
    struct mixed_buffer buffer = {0};
    check(mixed_make_buffer(1024, &buffer));
  })

#undef __TEST_SUITE
