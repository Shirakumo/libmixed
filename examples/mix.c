#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 100;
  struct mixed_segment_sequence sequence = {0};
  struct mixed_segment mix_segment = {0};
  struct mp3 *mp3s[argc-1];
  struct out *out = 0;

  signal(SIGINT, interrupt_handler);
  
  if(argc<2){
    fprintf(stderr, "Usage: ./test_mix mp3-file mp3-file* \n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    fprintf(stderr, "Failed to init MPG123.\n");
    goto cleanup;
  }

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }
  
  if(!mixed_make_segment_basic_mixer(2, &mix_segment)){
    fprintf(stderr, "Failed to create segment: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(!mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &mix_segment) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &out->right, &mix_segment)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Initialize MP3 sources
  for(int i=0; i+1<argc; ++i){
    struct mp3 *mp3 = 0;
    if(!load_mp3_segment(argv[i+1], samples, &mp3)){
      goto cleanup;
    }
    mp3s[i] = mp3;
    // Attach to combining segments
    if(!mixed_segment_set_in(MIXED_BUFFER, i*2, &mp3->left, &mix_segment) ||
       !mixed_segment_set_in(MIXED_BUFFER, i*2+1, &mp3->right, &mix_segment)){
      fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    // Register with sequence.
    if(!mixed_segment_sequence_add(&mp3->segment, &sequence)){
      fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
  }

  // The order in which segments are added does matter, as the system
  // will not figure out on its own in which order the segments should
  // be called. We specify this here.
  if(!mixed_segment_sequence_add(&mix_segment, &sequence) ||
     !mixed_segment_sequence_add(&out->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Perform the mixing
  mixed_segment_sequence_start(&sequence);

  size_t read = 0, played = 0;
  do{
    void *buffer;
    for(int i=0; i<argc-1; ++i){
      struct mp3 *mp3 = mp3s[i];
      read = SIZE_MAX;
      mixed_pack_request_write(&buffer, &read, &mp3->pack);
      if(mpg123_read(mp3->handle, buffer, read, &read) != MPG123_OK){
        fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
        goto cleanup;
      }
      mixed_pack_finish_write(read, &mp3->pack);
    }

    mixed_segment_sequence_mix(&sequence);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    size_t bytes = SIZE_MAX;
    mixed_pack_request_read(&buffer, &bytes, &out->pack);
    played = out123_play(out->handle, buffer, bytes);
    if(played < bytes){
      fprintf(stderr, "Warning: device not catching up with input (%lu vs %lu)\n", played, bytes);
    }
    mixed_pack_finish_read(played, &out->pack);
    
    printf("\rRead: %4lu Processed: %4lu Played: %4lu", read, bytes, played);
    fflush(stdout);
  }while(played && !interrupted);

  mixed_segment_sequence_end(&sequence);

  // Clean the shop
  exit = 0;
  
 cleanup:
  mixed_free_segment(&mix_segment);
  mixed_free_segment_sequence(&sequence);

  for(int i=1; i<argc; ++i){
    if(mp3s[i]){
      free_mp3(mp3s[i-1]);
    }
  }

  free_out(out);
  
  mpg123_exit();
  return exit;
}
