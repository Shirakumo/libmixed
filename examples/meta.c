#include "common.h"

#define DECODE_INT(type){                       \
    type *p = calloc(1, sizeof(type));          \
    if(!p) return 0;                            \
    *p = strtoll(in, 0, 10);                    \
    *target = p;                                \
  }

int parse_arg(int type, void **target, const char *in){
  switch(type){
  case MIXED_INT8:
    DECODE_INT(int8_t) break;
  case MIXED_UINT8:
    DECODE_INT(uint8_t) break;
  case MIXED_INT16:
    DECODE_INT(int16_t) break;
  case MIXED_UINT16:
    DECODE_INT(uint16_t) break;
  case MIXED_INT24:
    DECODE_INT(int24_t) break;
  case MIXED_UINT24:
    DECODE_INT(uint24_t) break;
  case MIXED_INT32:
    DECODE_INT(int32_t) break;
  case MIXED_UINT32:
    DECODE_INT(uint32_t) break;
  case MIXED_SIZE_T:
    DECODE_INT(size_t) break;
  case MIXED_BIQUAD_FILTER_ENUM:
  case MIXED_REPEAT_MODE_ENUM:
  case MIXED_NOISE_TYPE_ENUM:
  case MIXED_GENERATOR_TYPE_ENUM:
  case MIXED_FADE_TYPE_ENUM:
  case MIXED_ATTENUATION_ENUM:
  case MIXED_ENCODING_ENUM:
  case MIXED_ERROR_ENUM:
  case MIXED_RESAMPLE_TYPE_ENUM:
  case MIXED_LOCATION_ENUM: 
    DECODE_INT(int) break;
  default:
    fprintf(stderr, "Don't know how to decode type %s\n", mixed_type_string(type));
    return 0;
  }
  return 1;
}

int main(int argc, char **argv){
  struct mixed_segment segment = {0};
  uint32_t segments = 0;
  char **names = 0;
  uint32_t sargc = 0;
  void **sargv = 0;
  int result = 1;

  if(argc < 2){
    if(!mixed_list_segments(&segments, 0)){
      fprintf(stderr, "Failed to list segments:\n%s\n", mixed_error_string(-1));
      goto cleanup;
    }

    names = (char **)calloc(segments, sizeof(char *));
    if(!names){
      fprintf(stderr, "Failed to list segments:\n%s\n", mixed_error_string(-1));
      goto cleanup;
    }

    if(!mixed_list_segments(&segments, names)){
      fprintf(stderr, "Failed to list segments:\n%s\n", mixed_error_string(-1));
      goto cleanup;
    }

    printf("Known segments:\n");
    for(size_t i=0; i<segments; ++i){
      printf("- %s\n", names[i]);
    }
  }else{
    char *name = argv[1];
    const struct mixed_segment_field_info *arginfo;
    if(!mixed_make_segment_info(name, &sargc, &arginfo)){
      fprintf(stderr, "Failed to retrieve segment info:\n%s\n", mixed_error_string(-1));
    }
    if(argc-2 < sargc){
      printf("%s requires the following construction arguments:\n", name);
      for(size_t i=0; i<sargc; ++i){
        printf("- %s (%s)\n", arginfo[i].description, mixed_type_string(arginfo[i].type));
      }
    }else{
      sargv = calloc(sargc, sizeof(void *));
      for(uint32_t i=0; i<sargc; ++i){
        if(!parse_arg(arginfo[i].type, &sargv[i], argv[i])){
          fprintf(stderr, "Failed to parse argument %u\n", i);
          goto cleanup;
        }
      }
      if(!mixed_make_segment(name, sargv, &segment)){
        fprintf(stderr, "Failed to create segment:\n%s\n", mixed_error_string(-1));
        goto cleanup;
      }
      struct mixed_segment_info info = {0};
      if(!mixed_segment_info(&info, &segment)){
        fprintf(stderr, "Failed to get segment info:\n%s\n", mixed_error_string(-1));
        goto cleanup;
      }
      printf("Name:        %s\n", info.name);
      printf("Description: %s\n", info.description);
      printf("Flags:       %i\n", info.flags);
      printf("Min Inputs:  %u\n", info.min_inputs);
      printf("Max Inputs:  %u\n", info.max_inputs);
      printf("Outputs:     %u\n", info.outputs);
      printf("Fields:\n");
      for(size_t i=0; i<32; ++i){
        struct mixed_segment_field_info field = info.fields[i];
        if(!field.type_count) break;
        printf("- %-16s\t%s[%i]\t%2i %s\n", mixed_segment_field_string(field.field), mixed_type_string(field.type), field.type_count, field.flags, field.description);
      }
    }
  }

  result = 0;
 cleanup:
  mixed_free_segment(&segment);
  if(names) free(names);
  if(sargv){
    for(size_t i=0; i<sargc; ++i){
      if(sargv[i])free(sargv[i]);
    }
    free(sargv);
  }
  return result;
}
