#include "internal.h"

struct plugin_entry{
  char *file;
  void *handle;
};

struct plugin_vector{
  struct plugin_entry **entries;
  size_t count;
  size_t size;
};

struct segment_entry{
  char *name;
  size_t argc;
  struct mixed_segment_field_info *args;
  mixed_make_segment_function function;
};

struct segment_vector{
  struct segment_entry **entries;
  size_t count;
  size_t size;
};

struct plugin_vector plugins = {0};
struct segment_vector segments = {0};

MIXED_EXPORT int mixed_load_plugin(char *file){
  struct plugin_entry *entry = 0;
  void *handle = 0;

  entry = calloc(1, sizeof(struct plugin_entry));
  if(!entry){
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

  entry->file = strdup(file);
  entry->handle = handle;
  if(!vector_add(entry, (struct vector *)&plugins)){
    goto cleanup;
  }
  
  return 1;

 cleanup:
  if(entry){
    if(entry->file)
      free(entry->file);
    free(entry);
  }
  close_library(handle);
  return 0;
}

MIXED_EXPORT int mixed_close_plugin(char *file){
  for(size_t i=0; i<plugins.count; ++i){
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
      free(entry->file);
      free(entry);
      return vector_remove_pos(i, (struct vector *)&plugins);
    }
  }
  return 0;
}

MIXED_EXPORT int mixed_register_segment(char *name, size_t argc, struct mixed_segment_field_info *args, mixed_make_segment_function function){
  struct segment_entry *entry = 0;

  entry = calloc(1, sizeof(struct segment_entry));
  if(!entry){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  entry->args = calloc(argc, sizeof(struct mixed_segment_field_info));
  if(!entry->args){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  for(size_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = segments.entries[i];
    if(strcmp(entry->name, name) == 0){
      mixed_err(MIXED_DUPLICATE_SEGMENT);
      goto cleanup;
    }
  }

  entry->name = strdup(name);
  entry->argc = argc;
  memcpy(entry->args, args, argc*sizeof(struct mixed_segment_field_info));
  entry->function = function;
  return vector_add(entry, (struct vector *)&plugins);
  
 cleanup:
  if(entry){
    if(entry->name)
      free(entry->name);
    if(entry->args)
      free(entry->args);
    free(entry);
  }
  return 0;
}

MIXED_EXPORT int mixed_deregister_segment(char *name){
  for(size_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = segments.entries[i];
    if(strcmp(entry->name, name) == 0){
      free(entry->name);
      free(entry->args);
      free(entry);
      return vector_remove_pos(i, (struct vector *)&segments);
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}

MIXED_EXPORT int mixed_list_segments(size_t *count, char **names){
  *count = segments.count;
  if(names){
    for(size_t i=0; i<segments.count; ++i){
      names[i] = segments.entries[i]->name;
    }
  }
  return 1;
}

MIXED_EXPORT int mixed_make_segment_info(char *name, size_t *argc, const struct mixed_segment_field_info **args){
  for(size_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = segments.entries[i];
    if(strcmp(entry->name, name) == 0){
      *argc = entry->argc;
      *args = entry->args;
      return 1;
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}

MIXED_EXPORT int mixed_make_segment(char *name, void *args, struct mixed_segment *segment){
  for(size_t i=0; i<segments.count; ++i){
    struct segment_entry *entry = segments.entries[i];
    if(strcmp(entry->name, name) == 0){
      return entry->function(args, segment);
    }
  }
  mixed_err(MIXED_BAD_SEGMENT);
  return 0;
}
