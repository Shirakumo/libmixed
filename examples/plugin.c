#include "../src/mixed.h"

// This is what we'll keep in the segment's opaque data field.
// For this plugin we just need to track input and output
// buffers, that's all.
struct data{
  struct mixed_buffer **in;
  struct mixed_buffer **out;
  uint32_t count;
};

int start(struct mixed_segment *segment){
  struct data *data = (struct data *)segment->data;
  // Ensure that the segment has all inputs and outputs set, so that
  // mixing won't encounter an error.
  for(uint32_t i=0; i<data->count; ++i){
    if(data->in[i] == 0 || data->out[i] == 0){
      mixed_set_error(MIXED_NOT_INITIALIZED);
      return 0;
    }
  }
  return 1;
}

int mix(struct mixed_segment *segment){
  struct data *data = (struct data *)segment->data;
  for(uint32_t i=0; i<data->count; ++i){
    mixed_buffer_transfer(data->in[i], data->out[i]);
  }
  return 1;
}

// The set_out, set_in, set, and get functions are very frequently
// pretty boilerplate-y like these here.
int set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct data *data = (struct data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    if(data->count <= location){
      mixed_set_error(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->out[location] = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_set_error(MIXED_INVALID_FIELD);
    return 0;
  }
}

int set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct data *data = (struct data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    if(data->count <= location){
      mixed_set_error(MIXED_INVALID_LOCATION);
      return 0;
    }
    data->in[location] = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_set_error(MIXED_INVALID_FIELD);
    return 0;
  }
}

int info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct data *data = (struct data *)segment->data;
  info->name = "example_plugin";
  info->description = "Transfers one or more buffers.";
  info->min_inputs = data->count;
  info->max_inputs = data->count;
  info->outputs = data->count;

  struct mixed_segment_field_info *field = info->fields;
  field->field = MIXED_BUFFER;
  field->type = MIXED_BUFFER_POINTER;
  field->type_count = 1;
  field->flags = MIXED_IN | MIXED_OUT | MIXED_SET;
  field->description = "The buffer for audio data attached to the location.";
  // Clear out the last field so that it marks the end of the list.
  field++;
  field->field = 0;
  return 1;
}

int make_example(void *args, struct mixed_segment *segment){
  // First decode the arguments
  uint32_t count = *(((uint32_t**)args)[0]);

  // Then allocate the data we need for our segment.
  // We should always use mixed_calloc for allocations, so
  // that if the user overrides the allocator, plugins will
  // still use that allocator, rather than the system's.
  struct data *data = mixed_calloc(1, sizeof(struct data));
  if(!data) goto cleanup;

  data->count = count;
  data->in = mixed_calloc(count, sizeof(struct mixed_buffer *));
  data->out = mixed_calloc(count, sizeof(struct mixed_buffer *));
  if(!data->in) goto cleanup;
  if(!data->out) goto cleanup;

  // We only need to set the segment fields that we have
  // implemented. The rest can be left at 0.
  segment->start = start;
  segment->mix = mix;
  segment->set_in = set_in;
  segment->set_out = set_out;
  segment->info = info;
  segment->data = data;
  return 1;

  // Like this the error handling and memory cleanup is a lot
  // simpler than if we did it at every allocation site.
 cleanup:
  mixed_set_error(MIXED_OUT_OF_MEMORY);
  if(data){
    if(data->in) free(data->in);
    if(data->out) free(data->out);
    free(data);
  }
  return 0;
}

// These two functions present the plugin interface itself.
// We could register multiple segments in one plugin, too.
int mixed_make_plugin(mixed_register_segment_function registrar){
  struct mixed_segment_field_info args[1] = {{
    .description = "The number of buffers",
    .type = MIXED_INT32
  }};
  registrar("example_plugin", 1, args, make_example);
}

int mixed_free_plugin(mixed_deregister_segment_function registrar){
  registrar("example_plugin");
}
