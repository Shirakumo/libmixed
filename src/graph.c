#include "mixed.h"

int mixed_graph_make(struct mixed_graph *graph){
  return mixed_vector_make(graph->connections);
}

void mixed_graph_free(struct mixed_graph *graph){
  for(size_t i=0; i<graph->connections->count; ++i){
    free(graph->connections->data[i]);
  }
  
  mixed_vector_free(graph->connections);
}

int mixed_graph_connect(void *source, uint8_t out, void *target, uint8_t in, struct mixed_graph *graph){
  for(size_t i=0; i<graph->connections->count; ++i){
    struct mixed_connection *connection = graph->connections->data[i];
    if(connection->from == source
       && connection->from_output = out
       && connection->to = target
       && connection->to_input = in){
      return 1;
    }
  }
  
  struct mixed_connection *connection = calloc(1, sizeof(struct mixed_connection));
  if(!connection){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  connection->from = source;
  connection->from_output = out;
  connection->to = target;
  connection->to_input = in;
  if(!vector_push(connection, graph->connections)){
    free(connection);
    return 0;
  }
  return 1;
}

int mixed_graph_disconnect(void *source, uint8_t out, void *target, uint8_t in, struct mixed_graph *graph){
  for(size_t i=0; i<graph->connections->count; ++i){
    struct mixed_connection *connection = graph->connections->data[i];
    if(connection->from == source
       && connection->from_output == out
       && connection->to == target
       && connection->to_input == in){
      return vector_remove(connection, graph->connections);
    }
  }
  return 1;
}

int mixed_graph_remove(void *segment, struct mixed_graph *graph){
  for(size_t i=0; i<graph->connections->count; ++i){
    struct mixed_connection *connection = graph->connections->data[i];
    if(connection->from == segment
       || connection->to == segment){
      if(!vector_remove(connection, graph->connections)){
        return 0;
      }
    }
  }
  return 1;
}

int mixed_graph_elements(struct mixed_vector *elements, struct mixed_graph *graph){
  for(size_t i=0; i<graph->connections->count; ++i){
    struct mixed_connection *connection = graph->connections->data[i];
    if(!vector_pushnew(connection->from, elements)){
      return 0;
    }
    if(!vector_pushnew(connection->to, elements)){
      return 0;
    }
  }
  return 1;
}
