#include "internal.h"

struct plugin_entry{
  char *file;
  void *handle;
};

struct plugin_vector{
  struct plugin_entry **entries;
  uint32_t count;
  uint32_t size;
};

struct segment_entry{
  char name[MIXED_MAX_SEGMENT_NAME_LENGTH];
  uint32_t argc;
  mixed_make_segment_function function;
  struct mixed_segment_field_info args[MIXED_MAX_MAKE_ARG_COUNT];
};

struct segment_vector{
  uint32_t count;
  struct segment_entry entries[MIXED_MAX_SEGMENT_COUNT];
};

struct plugin_vector plugins = {0};
struct segment_vector segments = {0};

MIXED_EXPORT int mixed_load_plugin(char *file){
  struct plugin_entry *entry = 0;
  void *handle = 0;
  uint32_t length = strlen(file);

  entry = mixed_calloc(1, sizeof(struct plugin_entry));
  if(!entry){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }
  
  entry->file = mixed_calloc(length+1, 1);
  if(!entry->file){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  handle = open_library(file);
  if(!handle) goto cleanup;
  
  mixed_make_plugin_function function;
  *(void **)(&function) = load_symbol(handle, "mixed_make_plugin");
  if(!function) goto cleanup;

  if(!function(mixed_register_segment)){
    goto cleanup;
  }

  memcpy(entry->file, file, length);
  entry->handle = handle;
  if(!vector_add(entry, (struct vector *)&plugins)){
    goto cleanup;
  }
  
  return 1;

 cleanup:
  if(entry){
    if(entry->file)
      mixed_free(entry->file);
    mixed_free(entry);
  }
  close_library(handle);
  return 0;
}

MIXED_EXPORT int mixed_close_plugin(char *file){
  for(uint32_t i=0; i<plugins.count; ++i){
    struct plugin_entry *entry = plugins.entries[i];
    if(strcmp(entry->file, file) == 0){
      mixed_free_plugin_function function;
      *(void **)(&function) = load_symbol(entry->handle, "mixed_free_plugin");
      if(!function){
        mixed_err(MIXED_BAD_DYNAMIC_LIBRARY);
        return 0;
      }
      if(!function(mixed_deregister_segment)){
        return 0;
      }
      close_library(entry->handle);
      mixed_free(entry->file);
      mixed_free(entry);
      return vector_remove_pos(i, (struct vector *)&plugins);
    }
  }
  return 0;
}

MIXED_EXPORT int mixed_register_segment(char *name, uint32_t argc, struct mixed_segment_field_info *args, mixed_make_segment_function function){
  struct segment_entry *entry = 0;

  if(MIXED_MAX_MAKE_ARG_COUNT < argc){
    mixed_err(MIXED_BAD_ARGUMENT_COUNT);
    return 0;
  }

  uint32_t name_length = strlen(name);
  if(MIXED_MAX_SEGMENT_NAME_LENGTH < name_length){
    mixed_err(MIXED_BAD_NAME);
    return 0;
  }

  if(MIXED_MAX_SEGMENT_COUNT <= segments.count){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  for(uint32_t i=0; i<segments.count; ++i){
    struct segment_entry *cur_entry = &segments.entries[i];
    if(!entry && !cur_entry->function){
      entry = cur_entry;
    }else if(strcmp(cur_entry->name, name) == 0){
      mixed_err(MIXED_DUPLICATE_SEGMENT);
      return 0;
    }
  }

  if(!entry){
    entry = &segments.entries[segments.count];
  }

  memcpy(entry->name, name, name_length);
  entry->argc = argc;
  memcpy(entry->args, args, argc*sizeof(struct mixed_segment_field_info));
  entry->function = function;
  segments.count++;

  return 1;
}

MIXED_EXPORT int mixed_deregister_segment(char *name){
  for(uint32_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = &segments.entries[i];
    if(entry->function && strcmp(entry->name, name) == 0){
      entry->function = 0;
      segments.count--;
      return 1;
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}

MIXED_EXPORT int mixed_list_segments(uint32_t *count, char **names){
  *count = segments.count;
  if(names){
    for(uint32_t i=0,j=0; j<segments.count; ++i){
      struct segment_entry *entry = &segments.entries[i];
      if(!entry->function) continue;
      names[j] = entry->name;
      ++j;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_info(char *name, uint32_t *argc, const struct mixed_segment_field_info **args){
  for(uint32_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = &segments.entries[i];
    if(entry->function && strcmp(entry->name, name) == 0){
      *argc = entry->argc;
      *args = entry->args;
      return 1;
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}

MIXED_EXPORT int mixed_make_segment(char *name, void *args, struct mixed_segment *segment){
  for(uint32_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = &segments.entries[i];
    if(entry->function && strcmp(entry->name, name) == 0){
      return entry->function(args, segment);
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}
