#include <string.h>
#include <stdlib.h>
#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 1024;
  size_t samplerate = 44100;
  struct mixed_mixer mixer = {0};
  struct mixed_segment generator = {0};
  struct mixed_segment fade = {0};
  struct out *out = 0;

  enum mixed_generator_type wave_type = MIXED_SINE;
  size_t frequency = 440;

  if(argc < 3){
    printf("Usage: ./test_tone wave-type frequency \n");
    return 0;
  }

  if(0 == strcmp("sine", argv[1])) wave_type = MIXED_SINE;
  else if(0 == strcmp("square", argv[1])) wave_type = MIXED_SQUARE;
  else if(0 == strcmp("triangle", argv[1])) wave_type = MIXED_TRIANGLE;
  else if(0 == strcmp("sawtooth", argv[1])) wave_type = MIXED_SAWTOOTH;
  else{
    printf("Invalid wave type. Must be one of sine, square, triangle, sawtooth.\n");
    goto cleanup;
  }
  
  frequency = strtol(argv[2], 0, 10);
  if(frequency <= 0){
    printf("Invalid frequency. Must be an integer above 0.\n");
    goto cleanup;
  }
  
  signal(SIGINT, interrupt_handler);

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }

  if(!mixed_make_segment_generator(wave_type, frequency, samplerate, &generator) ||
     !mixed_make_segment_fade(0.0, 1.0, 5.0, MIXED_CUBIC_IN_OUT, samplerate, &fade)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->left, &generator) ||
     !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &out->left, &fade) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->left, &fade)){
    printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_mixer_add(&generator, &mixer) ||
     !mixed_mixer_add(&fade, &mixer) ||
     !mixed_mixer_add(&out->segment, &mixer)){
    printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  mixed_mixer_start(&mixer);

  size_t played;
  uint8_t samplesize = mixed_samplesize(out->channel.encoding);
  do{
    if(!mixed_mixer_mix(samples, &mixer)){
      printf("Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    played = out123_play(out->handle, out->channel.data, out->channel.size);
    if(played < samples*samplesize){
      printf("Warning: device not catching up with input (%i vs %i)\n", played, samples);
    }
  }while(!interrupted);
  
  mixed_mixer_end(&mixer);

  exit = 0;

 cleanup:
  
  mixed_free_segment(&generator);
  mixed_free_segment(&fade);
  free_out(out);
  
  return exit;
}
