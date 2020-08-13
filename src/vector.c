#include "internal.h"

void free_vector(struct vector *vector){
  if(vector->data)
    free(vector->data);
  vector->data = 0;
}

int vector_add(void *element, struct vector *vector){
  void **data = vector->data;
  // Not yet initialised
  if(!data){
    if(vector->size == 0) vector->size = BASE_VECTOR_SIZE;
    data = calloc(vector->size, sizeof(void *));
    vector->count = 0;
  }
  // Too small
  if(vector->count == vector->size){
    data = crealloc(vector->data, vector->size, vector->size*2, sizeof(void *));
    if(data) vector->size *= 2;
  }
  // Check completeness
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  // All good
  data[vector->count] = element;
  vector->data = data;
  ++vector->count;
  return 1;
}

int vector_remove_pos(uint32_t i, struct vector *vector){
  if(!vector->data){
    mixed_err(MIXED_NOT_INITIALIZED);
    return 0;
  }
  // Shift down
  for(uint32_t j=i+1; j<vector->count; ++j){
    vector->data[j-1] = vector->data[j];
  }
  --vector->count;
  vector->data[vector->count] = 0;
  // We have sufficiently deallocated. Shrink.
  if(vector->count < vector->size/4 && BASE_VECTOR_SIZE < vector->size){
    void **data = crealloc(vector->data, vector->size, vector->size/2, sizeof(void *));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    vector->size /= 2;
    vector->data = data;
  }
  return 1;
}

int vector_remove_item(void *element, struct vector *vector){
  if(!vector->data){
    mixed_err(MIXED_NOT_INITIALIZED);
    return 0;
  }
  for(uint32_t i=0; i<vector->count; ++i){
    if(vector->data[i] == element){
      vector_remove_pos(i, vector);
      break;
    }
  }
  return 1;
}

int vector_clear(struct vector *vector){
  vector->count = 0;
  // We have sufficiently deallocated. Shrink.
  if(BASE_VECTOR_SIZE < vector->size){
    void **data = crealloc(vector->data, vector->size, BASE_VECTOR_SIZE, sizeof(void *));
    if(!data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    vector->size = BASE_VECTOR_SIZE;
    vector->data = data;
  }
  return 1;
}
