#include "internal.h"

void free_vector(struct vector *vector){
  if(vector->data)
    free(vector->data);
  vector->data = 0;
}

int vector_add(void *element, struct vector *vector){
  // Not yet initialised
  if(!vector->data){
    if(vector->size == 0) vector->size = BASE_VECTOR_SIZE;
    vector->data = calloc(vector->size, sizeof(void *));
    vector->count = 0;
  }
  // Too small
  if(vector->count == vector->size){
    vector->data = crealloc(vector->data, vector->size, vector->size*2, sizeof(void *));
    vector->size *= 2;
  }
  // Check completeness
  if(!vector->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    vector->count = 0;
    vector->size = 0;
    return 0;
  }
  // All good
  vector->data[vector->count] = element;
  ++vector->count;
  return 1;
}

int vector_remove_pos(size_t i, struct vector *vector){
  if(!vector->data){
    mixed_err(MIXED_NOT_INITIALIZED);
    return 0;
  }
  // Shift down
  for(size_t j=i+1; j<vector->count; ++j){
    vector->data[j-1] = vector->data[j];
  }
  --vector->count;
  vector->data[vector->count] = 0;
  // We have sufficiently deallocated. Shrink.
  if(vector->count < vector->size/4 && BASE_VECTOR_SIZE < vector->size){
    vector->data = crealloc(vector->data, vector->size, vector->size/2,
                            sizeof(struct mixed_buffer *));
    vector->size /= 2;
    if(!vector->data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      vector->count = 0;
      vector->size = 0;
      return 0;
    }
  }
  return 1;
}

int vector_remove_item(void *element, struct vector *vector){
  if(!vector->data){
    mixed_err(MIXED_NOT_INITIALIZED);
    return 0;
  }
  for(size_t i=0; i<vector->count; ++i){
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
    vector->data = crealloc(vector->data, vector->size, BASE_VECTOR_SIZE,
                            sizeof(struct mixed_buffer *));
    vector->size = BASE_VECTOR_SIZE;
    if(!vector->data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      vector->size = 0;
      return 0;
    }
  }
  return 1;
}
