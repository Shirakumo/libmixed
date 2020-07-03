#include<mixed.h>

struct test{
  char *name;
  int exit;
  char reason[1024];
  int (*fun)();
};

int register_test(char *name, int (*fun)());
struct test *find_test(int id);

#define __TEST_SUITE mixed

#define __TEST_FUN2(SUITE, TEST) __testfun_ ## SUITE ## TEST
#define __TEST_FUN1(SUITE, TEST) __TEST_FUN2(SUITE, TEST)
#define __TEST_FUN(TEST) __TEST_FUN1(__TEST_SUITE, TEST)
#define __TEST_ID2(SUITE, TEST) __testid_ ## SUITE ## TEST
#define __TEST_ID1(SUITE, TEST) __TEST_ID2(SUITE, TEST)
#define __TEST_ID(TEST) __TEST_ID1(__TEST_SUITE, TEST)
#define __TEST_INIT2(SUITE, TEST) __testinit_ ## SUITE ## TEST
#define __TEST_INIT1(SUITE, TEST) __TEST_INIT2(SUITE, TEST)
#define __TEST_INIT(TEST) __TEST_INIT1(__TEST_SUITE, TEST)

#define define_test(TITLE, ...)                                         \
  int __TEST_FUN(TITLE)();                                              \
  static int __TEST_ID(TITLE);                                          \
  int __TEST_FUN(TITLE)(){                                              \
    int __testid = __TEST_ID(TITLE);                                    \
    __VA_ARGS__                                                         \
      return 1;                                                         \
  }                                                                     \
  __attribute__((constructor)) static void __TEST_INIT(TITLE)(){        \
    __TEST_ID(TITLE) = register_test(# TITLE, __TEST_FUN(TITLE));       \
  }

#define fail(EXIT, ...) {                               \
  struct test *__test = find_test(__testid);            \
  snprintf(__test->reason, 1024, __VA_ARGS__);          \
  __test->exit = EXIT;                                  \
  return 0;                                             \
  };
