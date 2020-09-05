#include<stdio.h>
#include"../src/mixed.h"
#define RANDOMIZED_REPEAT 10000000

struct test{
  char *suite;
  char *name;
  char *file;
  int line;
  int form;
  char reason[1024];
  int (*fun)();
};

#ifndef __TEST_SUITE
#error "__TEST_SUITE is not defined."
#endif

void maybe_invoke_debugger();
int register_test(char *suite, char *name, int (*fun)());
struct test *find_test(int id);

#define __TEST_FUN2(SUITE, TEST) __testfun_ ## SUITE ## TEST
#define __TEST_FUN1(SUITE, TEST) __TEST_FUN2(SUITE, TEST)
#define __TEST_FUN(TEST) __TEST_FUN1(__TEST_SUITE, TEST)
#define __TEST_ID2(SUITE, TEST) __testid_ ## SUITE ## TEST
#define __TEST_ID1(SUITE, TEST) __TEST_ID2(SUITE, TEST)
#define __TEST_ID(TEST) __TEST_ID1(__TEST_SUITE, TEST)
#define __TEST_INIT2(SUITE, TEST) __testinit_ ## SUITE ## TEST
#define __TEST_INIT1(SUITE, TEST) __TEST_INIT2(SUITE, TEST)
#define __TEST_INIT(TEST) __TEST_INIT1(__TEST_SUITE, TEST)
#define __NAME1(VAR) # VAR
#define __NAME(VAR) __NAME1(VAR)

#define define_test(TITLE, ...)                                         \
  int __TEST_FUN(TITLE)();                                              \
  static int __TEST_ID(TITLE);                                          \
  int __TEST_FUN(TITLE)(){                                              \
    int __testid = __TEST_ID(TITLE);                                    \
    int __testresult = 1;                                               \
    int __formid = 0;                                                   \
    __VA_ARGS__                                                         \
      return __testresult;                                              \
  }                                                                     \
  __attribute__((constructor)) static void __TEST_INIT(TITLE)(){        \
    __TEST_ID(TITLE) = register_test(__NAME(__TEST_SUITE), # TITLE, __TEST_FUN(TITLE)); \
  }

#define fail_test(...){                                 \
    maybe_invoke_debugger();                            \
    struct test *__test = find_test(__testid);          \
    snprintf(__test->reason, 1024, __VA_ARGS__);        \
    __test->file = __FILE__;                            \
    __test->line = __LINE__;                            \
    __test->form = __formid;                            \
    __testresult = 0;                                   \
    goto cleanup;                                       \
  }

#define pass(FORM)                                                      \
  if(!FORM){                                                            \
    int __error = mixed_error();                                        \
    fail_test("%s", mixed_error_string(__error));                       \
  }else ++__formid;

#define fail(FORM)                                                      \
  if(FORM){                                                             \
    fail_test("Expected form to fail, but it succeeded.");              \
  }else ++__formid;

#define is(A, B){                                                       \
    ssize_t __a = (ssize_t)(A);                                         \
    ssize_t __b = (ssize_t)(B);                                         \
    if(__a != __b)                                                      \
      fail_test("Value was %li but should have been %li", __a, __b)     \
      else ++__formid;                                                  \
  }

#define is_a(A, B, D){                                                  \
    ssize_t __a = (ssize_t)(A);                                         \
    ssize_t __b = (ssize_t)(B);                                         \
    if(D < labs(__a - __b))                                              \
      fail_test("Value was %li but should have been %li", __a, __b)     \
      else ++__formid;                                                  \
  }

#define isnt(A, B){                                                     \
    ssize_t __a = (ssize_t)(A);                                         \
    ssize_t __b = (ssize_t)(B);                                         \
    if(__a == __b)                                                      \
      fail_test("Value was %li but should not have been.", __a)         \
    else ++__formid;                                                    \
  }

#define is_ui(A, B){                                                    \
    size_t __a = (size_t)(A);                                           \
    size_t __b = (size_t)(B);                                           \
    if(__a != __b)                                                      \
      fail_test("Value was %lu but should have been %lu", __a, __b)     \
    else ++__formid;                                                    \
  }

#define is_f(A, B){                                                     \
    float __a = (float)(A);                                             \
    float __b = (float)(B);                                             \
    if(__a != __b)                                                      \
      fail_test("Value was %f but should have been %f", __a, __b)       \
    else ++__formid;                                                    \
  }

#define is_p(A, B){                                                     \
    void *__a = (void *)(A);                                            \
    void *__b = (void *)(B);                                            \
    if(__a != __b)                                                      \
      fail_test("Value was %p but should have been %p", __a, __b)       \
    else ++__formid;                                                    \
  }

#define isnt_p(A, B){                                                   \
    void *__a = (void *)(A);                                            \
    void *__b = (void *)(B);                                            \
    if(__a == __b)                                                      \
      fail_test("Value was %p but should not have been", __a)           \
    else ++__formid;                                                    \
  }
