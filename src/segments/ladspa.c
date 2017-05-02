#include <dlfcn.h>
#include "ladspa.h"
#include "internal.h"

struct ladspa_segment_data{
  LADSPA_Descriptor *descriptor;
  LADSPA_Handle *handle;
  char active;
  size_t samplerate;
};

int ladspa_segment_free(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  if(data){
    if(data->active && data->descriptor->deactivate)
      data->descriptor->deactivate(data->handle);
    if(data->descriptor->cleanup)
      data->descriptor->cleanup(data->handle);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int ladspa_segment_set_in(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  // FIXME
}

int ladspa_segment_set_out(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  // FIXME
}

int ladspa_segment_mix(size_t samples, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  data->descriptor->run(data->handle, samples);
  return 1;
}

int ladspa_segment_start(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  if(data->active){
    mixed_err(MIXED_SEGMENT_ALREADY_STARTED);
    return 0;
  }
  
  if(data->descriptor->activate){
    data->descriptor->activate(data->handle);
  }
  data->active = 1;
  return 1;
}

int ladspa_segment_end(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  if(!data->active){
    mixed_err(MIXED_SEGMENT_ALREADY_ENDED);
    return 0;
  }
  
  if(data->descriptor->deactivate){
    data->descriptor->deactivate(data->handle);
  }else{
    mixed_err(MIXED_NOT_IMPLEMENTED);
  }
  data->active = 0;
  return 1;
}

struct mixed_segment_info ladspa_segment_info(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  struct mixed_segment_info info = {0};
  info.name = "ladspa";
  info.description = data->descriptor->Name;
  info.min_inputs = 0;
  info.max_inputs = 0;
  info.outputs = 0;

  if(!LADSPA_IS_INPLACE_BROKEN(data->descriptor->Properties)){
    info.flags = MIXED_INPLACE;
  }
  
  for(size_t i=0; i<data->descriptor->PortCount; ++i){
    const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
    if(LADSPA_IS_PORT_AUDIO(port)){
      if(LADSPA_IS_PORT_INPUT(port)){
        ++info.max_inputs;
        ++info.min_inputs;
      }else{
        ++info.outputs;
      }
    }  
  }
  return info;
}

int ladspa_segment_get(size_t field, void *value, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  // FIXME
  return 0;
}

int ladspa_segment_set(size_t field, void *value, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  // FIXME
  return 0;
}

int ladspa_load_descriptor(char *file, size_t index, LADSPA_Descriptor **_descriptor){
  int exit = 0;
  // FIXME: Check LADSPA_PATH
  
  // Nodelete so that we can close the handle after this.
  void *lib = dlopen(file, RTLD_NOW | RTLD_NODELETE);
  if(!lib){
    fprintf(stderr, "MIXED: DYLD error: %s\n", dlerror());
    mixed_err(MIXED_LADSPA_OPEN_FAILED);
    goto cleanup;
  }

  dlerror();
  LADSPA_Descriptor_Function ladspa_descriptor = dlsym(lib, "ladspa_descriptor");
  char *error = dlerror();
  if(error != 0){
    fprintf(stderr, "MIXED: DYLD error: %s\n", error);
    mixed_err(MIXED_LADSPA_BAD_LIBRARY);
    goto cleanup;
  }
  
  const LADSPA_Descriptor *descriptor = ladspa_descriptor(index);
  if(!descriptor){
    mixed_err(MIXED_LADSPA_NO_PLUGIN_AT_INDEX);
    goto cleanup;
  }

  *_descriptor = (LADSPA_Descriptor *)descriptor;

 cleanup:
  if(lib)
    dlclose(lib);
  return exit;
}

int mixed_make_segment_ladspa(char *file, size_t index, size_t samplerate, struct mixed_segment *segment){
  LADSPA_Descriptor *descriptor;
  if(!ladspa_load_descriptor(file, index, &descriptor)){
    return 0;
  }

  struct ladspa_segment_data *data = calloc(1, sizeof(struct ladspa_segment_data));
  if(!data){
    return 0;
  }

  data->handle = data->descriptor->instantiate(data->descriptor, samplerate);  
  if(!data->handle){
    mixed_err(MIXED_LADSPA_INSTANTIATION_FAILED);
    free(data);
    return 0;
  }

  segment->free = ladspa_segment_free;
  segment->mix = ladspa_segment_mix;
  segment->start = ladspa_segment_start;
  segment->end = ladspa_segment_end;
  segment->set_in = ladspa_segment_set_in;
  segment->set_out = ladspa_segment_set_out;
  segment->info = ladspa_segment_info;
  segment->get = ladspa_segment_get;
  segment->set = ladspa_segment_set;
  segment->data = data;
  return 1;
}
