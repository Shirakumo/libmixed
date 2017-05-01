#include <stdio.h>
#include "mixed.h"
#define BASE_VECTOR_SIZE 32

void mixed_err(int errorcode);

void *crealloc(void *ptr, size_t oldcount, size_t newcount, size_t size);
