#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 1024;
  struct mixed_mixer mixer = {0};
  struct mixed_segment lmix_segment = {0};
  struct mixed_segment rmix_segment = {0};
  struct mixed_segment gen_segment = {0};
  struct mp3 *mp3s[argc-1];
  struct out *out = 0;

  signal(SIGINT, interrupt_handler);
  
  if(argc<2){
    printf("Usage: ./test_mix_mp3 mp3-file mp3-file* \n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    printf("Failed to init MPG123.\n");
    goto cleanup;
  }

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }
  
  if(!mixed_make_segment_mixer(0, &lmix_segment) ||
     !mixed_make_segment_mixer(0, &rmix_segment) ||
     !mixed_make_segment_general(1.0, 0.0, &gen_segment)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(// The mixer segment's 0th buffer is its output.
     !mixed_segment_set_out(MIXED_MONO, &out->left, &lmix_segment) ||
     !mixed_segment_set_out(MIXED_MONO, &out->right, &rmix_segment) ||
     // The general segment has two ins and two outs. But since it is
     // specified as being in-place, we can attach the same buffers.
     !mixed_segment_set_in(MIXED_LEFT, &out->left, &gen_segment) ||
     !mixed_segment_set_in(MIXED_RIGHT, &out->right, &gen_segment) ||
     !mixed_segment_set_out(MIXED_LEFT, &out->left, &gen_segment) ||
     !mixed_segment_set_out(MIXED_RIGHT, &out->right, &gen_segment)){
    printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Initialize MP3 sources
  for(size_t i=1; i<argc; ++i){
    struct mp3 *mp3 = 0;
    if(!load_mp3_segment(argv[i], samples, &mp3)){
      goto cleanup;
    }
    mp3s[i-1] = mp3;
    // Attach to combining segments
    if(!mixed_segment_set_in(i, &mp3->left, &lmix_segment) ||
       !mixed_segment_set_in(i, &mp3->right, &rmix_segment)){
      printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    // Register with mixer.
    if(!mixed_mixer_add(&mp3->segment, &mixer)){
      printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
  }

  // The order in which segments are added does matter, as the system
  // will not figure out on its own in which order the segments should
  // be called. We specify this here.
  if(!mixed_mixer_add(&lmix_segment, &mixer) ||
     !mixed_mixer_add(&rmix_segment, &mixer) ||
     !mixed_mixer_add(&gen_segment, &mixer) ||
     !mixed_mixer_add(&out->segment, &mixer)){
    printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Perform the mixing
  mixed_mixer_start(&mixer);

  size_t read = 0, read_total = 0, played = 0;
  uint8_t samplesize = mixed_samplesize(out->channel.encoding);
  do{
    read_total = 0;
    for(size_t i=0; i<argc-1; ++i){
      if(mpg123_read(mp3s[i]->handle, mp3s[i]->channel.data, mp3s[i]->channel.size, &read) != MPG123_OK){
        printf("Failure during MP3 decoding: %s\n", mpg123_strerror(mp3s[i]->handle));
        goto cleanup;
      }
      // Make sure we play until everything's done.
      if(read_total < read) read_total = read;
      // Make sure to zero out empty regions.
      for(size_t j=read; j<mp3s[i]->channel.size; ++j){
        ((uint8_t *)mp3s[i]->channel.data)[j] = 0;
      }
    }
    
    if(!mixed_mixer_mix(samples, &mixer)){
      printf("Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    played = out123_play(out->handle, out->channel.data, out->channel.size);
    if(played < samples*samplesize){
      printf("Warning: device not catching up with input (%i vs %i)\n", played, samples);
    }
  }while(read_total && !interrupted);

  mixed_mixer_end(&mixer);

  // Clean the shop
  exit = 0;
  
 cleanup:
  printf("\nCleaning up.\n");
  
  mixed_free_segment(&lmix_segment);
  mixed_free_segment(&rmix_segment);
  mixed_free_segment(&gen_segment);
  mixed_free_mixer(&mixer);

  for(size_t i=1; i<argc; ++i){
    if(mp3s[i]){
      free_mp3(mp3s[i-1]);
    }
  }

  free_out(out);
  
  mpg123_exit();
  return exit;
}
