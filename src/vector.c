#include "mixed.h"

int mixed_vector_make(struct mixed_vector *vector){
  size_t size = (vector->size!=0)? vector->size : 16;
  vector->data = calloc(size, sizeof(void *));
  if(!vector->data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  vector->size = size;
  vector->count = 0;
  return 1;
}

void mixed_vector_free(struct mixed_vector *vector){
  if(vector->data)
    free(vector->data);
  vector->data = 0;
  vector->size = 0;
  vector->count = 0;
}

int mixed_vector_push(void *element, struct mixed_vector *vector){
  if(vector->count < vector->size){
    vector->data[count] = element;
    ++vector->count;
  }else{
    vector->data = realloc(vector->data, vector->size*2);
    if(!vector->data){
      mixed_err(MIXED_OUT_OF_MEMORY);
      return 0;
    }
    vector->size *= 2;
    mixed_vector_push(element, vector);
  }
  return 1;
}

int mixed_vector_pushnew(void *element, struct mixed_vector *vector){
  for(size_t i=0; i<vector->count; ++i){
    if(vector->data[i] == element){
      return 1;
    }
  }
  return mixed_vector_push_extend(element, vector);
}

void *mixed_vector_pop(struct mixed_vector *vector){
  if(vector->count > 0){
    void *element = vector->data[vector->count];
    vector->data[vector->count] = 0;
    --vector->count;
    return element;
  }
  return 0;
}

int mixed_vector_remove(void *element, struct mixed_vector *vector){
  size_t newcount = 0;
  for(size_t i=0; i<vector->count; ++i){
    if(vector->data[i] != element){
      vector->data[newcount] = vector->data[i];
      ++newcount;
    }
  }
  for(size_t i=newcount; i<vector->count; ++i){
    vector->data[i] = 0;
  }
  vector->count = newcount;
  return 1;
}
