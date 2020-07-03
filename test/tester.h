#include<mixed.h>

struct test{
  char *name;
  int exit;
  char reason[1024];
  int (*fun)();
};

int register_test(char *name, int (*fun)());
struct test *find_test(int id);

#define define_test(TITLE, ...)                                         \
  int __testfun_ ## TITLE();                                            \
  static int __testid_ ## TITLE = register_test(# TITLE, __testfun_ ## TITLE); \
  int __testfun_ ## TITLE(){                                            \
    int __testid = __test_ ## TITLE;                                    \
    __VA_ARGS__                                                         \
    return 1;                                                           \
  };

#define fail(EXIT, ...) {                               \
  struct test *__test = find_test(__testid);            \
  snprintf(__test->reason, 1024, __VA_ARGS__);          \
  __test->exit = EXIT;                                  \
  return 0;                                             \
  };
