#include "../ladspa.h"
#include "../internal.h"

struct ladspa_port{
  struct mixed_buffer *buffer;
  char direction;
  float control;
};

struct ladspa_segment_data{
  LADSPA_Descriptor *descriptor;
  LADSPA_Handle *handle;
  char active;
  uint32_t samplerate;
  struct ladspa_port *ports;
};

int ladspa_segment_free(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  if(data){
    if(data->active && data->descriptor->deactivate)
      data->descriptor->deactivate(data->handle);
    if(data->descriptor->cleanup)
      data->descriptor->cleanup(data->handle);
    if(data->ports)
      mixed_free(data->ports);
    mixed_free(segment->data);
  }
  segment->data = 0;
  return 1;
}

// FIXME: add start method that checks for buffer completeness.

int ladspa_segment_set_in(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  uint32_t index = 0;

  switch(field){
  case MIXED_BUFFER:
    for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
      const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
      if(LADSPA_IS_PORT_AUDIO(port) && LADSPA_IS_PORT_INPUT(port)){
        if(index == location){
          data->ports[i].buffer = ((struct mixed_buffer *)buffer);
          data->ports[i].direction = MIXED_IN;
          return 1;
        }
        ++index;
      }
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int ladspa_segment_set_out(uint32_t field, uint32_t location, void *buffer, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  uint32_t index = 0;

  switch(field){
  case MIXED_BUFFER:
    for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
      const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
      if(LADSPA_IS_PORT_AUDIO(port) && LADSPA_IS_PORT_OUTPUT(port)){
        if(index == location){
          data->ports[i].buffer = ((struct mixed_buffer *)buffer);
          data->ports[i].direction = MIXED_OUT;
          return 1;
        }
        ++index;
      }
    }
    mixed_err(MIXED_INVALID_LOCATION);
    return 0;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int ladspa_segment_mix(struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  uint32_t samples = UINT32_MAX;
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    struct ladspa_port *port = &data->ports[i];
    if(port->buffer){
      float *buffer;
      if(port->direction == MIXED_IN)
        mixed_buffer_request_read(&buffer, &samples, port->buffer);
      else
        mixed_buffer_request_write(&buffer, &samples, port->buffer);
      data->descriptor->connect_port(data->handle, i, buffer);
    }
  }
  data->descriptor->run(data->handle, samples);
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    struct ladspa_port *port = &data->ports[i];
    if(port->buffer){
      if(port->direction == MIXED_IN)
        mixed_buffer_finish_read(samples, port->buffer);
      else
        mixed_buffer_finish_write(samples, port->buffer);
    }
  }
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

int ladspa_segment_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  
  info->name = "ladspa";
  info->description = data->descriptor->Name;
  info->min_inputs = 0;
  info->max_inputs = 0;
  info->outputs = 0;

  if(!LADSPA_IS_INPLACE_BROKEN(data->descriptor->Properties)){
    info->flags = MIXED_INPLACE;
  }
  
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
    if(LADSPA_IS_PORT_AUDIO(port)){
      if(LADSPA_IS_PORT_INPUT(port)){
        ++info->max_inputs;
        ++info->min_inputs;
      }else{
        ++info->outputs;
      }
    }

    // FIXME: Fill fields
    struct mixed_segment_field_info *field = info->fields;
    clear_info_field(field++);
  }
  return 1;
}

int ladspa_segment_get(uint32_t field, void *value, struct mixed_segment *segment){
  // I don't think the LADSPA interface is compatible with how we want
  // to use it. If I read it correctly, it says that the output controls
  // are only updated while the plugin is actually run, which is not
  // what we want here in most cases.
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  uint32_t index = 0;
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
    if(LADSPA_IS_PORT_CONTROL(port) && LADSPA_IS_PORT_OUTPUT(port)){
      if(index == field){
        *((float *)value) = data->ports[i].control;
        return 1;
      }
      ++index;
    }
  }
  mixed_err(MIXED_INVALID_FIELD);
  return 0;
}

int ladspa_segment_set(uint32_t field, void *value, struct mixed_segment *segment){
  struct ladspa_segment_data *data = (struct ladspa_segment_data *)segment->data;
  uint32_t index = 0;
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    const LADSPA_PortDescriptor port = data->descriptor->PortDescriptors[i];
    if(LADSPA_IS_PORT_CONTROL(port) && LADSPA_IS_PORT_INPUT(port)){
      if(index == field){
        data->ports[i].control = *((float *)value);
        return 1;
      }
      ++index;
    }
  }
  mixed_err(MIXED_INVALID_FIELD);
  return 0;
}

int ladspa_load_descriptor(const char *file, uint32_t index, LADSPA_Descriptor **_descriptor){
  LADSPA_Descriptor_Function descriptor_function;
  const LADSPA_Descriptor *descriptor;

  void *handle = open_library(file);
  if(!handle) goto cleanup;

  *(void **)(&descriptor_function) = load_symbol(handle, "descriptor_function");
  if(!descriptor_function) goto cleanup;
  
  descriptor = descriptor_function(index);
  if(!descriptor){
    mixed_err(MIXED_LADSPA_NO_PLUGIN_AT_INDEX);
    goto cleanup;
  }

  *_descriptor = (LADSPA_Descriptor *)descriptor;
  return 1;

 cleanup:
  close_library(handle);
  return 0;
}

MIXED_EXPORT int mixed_make_segment_ladspa(const char *file, uint32_t index, uint32_t samplerate, struct mixed_segment *segment){
  struct ladspa_segment_data *data = 0;

  data = mixed_calloc(1, sizeof(struct ladspa_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  if(!ladspa_load_descriptor(file, index, &data->descriptor)){
    goto cleanup;
  }

  data->ports = mixed_calloc(data->descriptor->PortCount, sizeof(struct ladspa_port));
  if(!data->ports){
    mixed_err(MIXED_OUT_OF_MEMORY);
    goto cleanup;
  }

  data->handle = data->descriptor->instantiate(data->descriptor, samplerate);  
  if(!data->handle){
    mixed_err(MIXED_LADSPA_INSTANTIATION_FAILED);
    goto cleanup;
  }

  // Connect all ports to shims
  for(uint32_t i=0; i<data->descriptor->PortCount; ++i){
    data->descriptor->connect_port(data->handle, i, &data->ports[i].control);
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

 cleanup:
  if(data){
    if(data->ports)
      mixed_free(data->ports);
    mixed_free(data);
  }
  
  return 0;
}

int __make_ladspa(void *args, struct mixed_segment *segment){
  return mixed_make_segment_ladspa(ARG(const char *, 0), ARG(uint32_t, 1), ARG(uint32_t, 2), segment);
}

REGISTER_SEGMENT(ladspa, __make_ladspa, 3, {
    {.description = "file", .type = MIXED_STRING},
    {.description = "index", .type = MIXED_UINT32},
    {.description = "samplerate", .type = MIXED_UINT32}})
