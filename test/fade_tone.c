#include "common.h"

int main(){
  int exit = 1;
  size_t samples = 2048;
  size_t samplerate = 44100;
  struct mixed_mixer mixer = {0};
  struct mixed_segment generator = {0};
  struct mixed_segment fade = {0};
  struct out *out;
  
  signal(SIGINT, interrupt_handler);

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }

  if(!mixed_make_segment_generator(MIXED_SINE, 440, samplerate, &generator) ||
     !mixed_make_segment_fade(0.0, 1.0, 5.0, MIXED_CUBIC_IN_OUT, samplerate, &fade)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_out(MIXED_MONO, &out->left, &generator) /* ||
     !mixed_segment_set_in(MIXED_MONO, &out->left, &fade) ||
     !mixed_segment_set_out(MIXED_MONO, &out->left, &fade)*/){
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
  do{
    if(!mixed_mixer_mix(samples, &mixer)){
      printf("Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    played = out123_play(out->handle, out->channel.data, out->channel.size);
    if(played < samples){
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
