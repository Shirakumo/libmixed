#include <stdio.h>
#include <string.h>
#include "tester.h"

#define MAX_TESTS 1024
struct test tests[MAX_TESTS] = {0};
int test_count = 0;

int register_test(char *suite, char *name, int (*fun)()){
  tests[test_count].suite = suite;
  tests[test_count].name = name;
  tests[test_count].form = -1;
  tests[test_count].reason[0] = 0;
  tests[test_count].fun = fun;
  return test_count++;
}

struct test *find_test(int id){
  if(id < 0 || MAX_TESTS <= id || tests[id].name[0] == 0)
    return 0;
  return &tests[id];
}

struct test *search_test(const char *name){
  for(int i=0; i<test_count; ++i){
    if(strncmp(name, tests[i].suite, strlen(tests[i].suite)) == 0
       || strncmp(name, tests[i].name, strlen(tests[i].name)) == 0)
      return &tests[i];
  }
  return 0;
}

int run_test(struct test *test){
  printf("Running %-10s %-36s \033[0;90m...\033[0;0m ", test->suite, test->name);
  int result = test->fun();
  printf((result==0)?"\033[1;31m[FAIL]":"\033[0;32m[OK  ]");
  printf("\033[0;0m\n");
  return result;
}

int run_tests(struct test *tests[], int test_count){
  printf("This is libmixed %s\n", mixed_version());
  printf("\033[1;33m --> \033[0;0mWill run %i tests.\n", test_count);
  struct test *failed[test_count];
  int failures = 0;
  int passes = 0;

  for(int i=0; i<test_count; ++i){
    if(run_test(tests[i])){
      ++passes;
    }else{
      failed[failures++] = tests[i];
    }
  }
  
  fprintf(stderr, "\nPassed: %3i", passes);
  fprintf(stderr, "\nFailed: %3i\n", failures);
  if(failures){
    fprintf(stderr, "\033[1;33m --> \033[0;0mThe following tests failed:\n");
    for(int i=0; i<failures; ++i){
      struct test *test = failed[i];
      fprintf(stderr, "%s %s (%i):\n  %s\n", test->suite, test->name, test->form, test->reason);
    }
  }
  return (failures == 0);
}

int main(int argc, char *argv[]){
  struct test *tests_to_run[MAX_TESTS] = {0};
  int tests_to_runc = 0;
  if(1 < argc){
    for(int i=1; i<argc; ++i){
      struct test *test = search_test(argv[i]);
      if(!test) fprintf(stderr, "WARN: No test matching \"%s\" found.\n", argv[i]);
      else tests_to_run[tests_to_runc++] = test;
    }
  }else{
    for(int i=0; i<test_count; ++i){
      tests_to_run[tests_to_runc++] = &tests[i];
    }
  }

  return !run_tests(tests_to_run, tests_to_runc);
}
