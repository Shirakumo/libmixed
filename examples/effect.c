#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 100;
  struct mixed_segment_sequence sequence = {0};
  struct mixed_segment sfx_segment = {0};
  struct mp3 *mp3;
  struct out *out = 0;

  signal(SIGINT, interrupt_handler);
  
  if(argc<2){
    fprintf(stderr, "Usage: ./test_effect mp3-file \n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    fprintf(stderr, "Failed to init MPG123.\n");
    goto cleanup;
  }

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }
  
  //if(!mixed_make_segment_frequency_pass(MIXED_PASS_HIGH, 440, 44100, &sfx_segment)){
  if(!mixed_make_segment_speed_change(2.0, &sfx_segment)){
    fprintf(stderr, "Failed to create segment: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Initialize MP3 source
  if(!load_mp3_segment(argv[1], samples, &mp3)){
    goto cleanup;
  }
  
  // Attach to combining segments
  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &sfx_segment)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &sfx_segment)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  // Register with sequence.
  if(!mixed_segment_sequence_add(&mp3->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // The order in which segments are added does matter, as the system
  // will not figure out on its own in which order the segments should
  // be called. We specify this here.
  if(!mixed_segment_sequence_add(&mp3->segment, &sequence) ||
     !mixed_segment_sequence_add(&sfx_segment, &sequence) ||
     !mixed_segment_sequence_add(&out->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Perform the mixing
  mixed_segment_sequence_start(&sequence);

  size_t read = 0, read_total = 0, played = 0;
  do{
    read_total = 0;
    if(mpg123_read(mp3->handle, mp3->pack.data, mp3->pack.size, &read) != MPG123_OK){
      fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
      goto cleanup;
    }
    mp3->pack.frames = read / (mp3->pack.channels*mixed_samplesize(mp3->pack.encoding));
    // Make sure we play until everything's done.
    if(read_total < read) read_total = read;
    
    mixed_segment_sequence_mix(&sequence);
    mixed_buffer_clear(&mp3->right);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    size_t bytes = out->pack.frames * out->pack.channels * mixed_samplesize(out->pack.encoding);
    played = out123_play(out->handle, out->pack.data, bytes);
    if(played < bytes){
      fprintf(stderr, "Warning: device not catching up with input (%i vs %i)\n", played, bytes);
    }
  }while(read_total && !interrupted);

  mixed_segment_sequence_end(&sequence);

  // Clean the shop
  exit = 0;
  
 cleanup:
  fprintf(stderr, "\nCleaning up.\n");
  
  mixed_free_segment(&sfx_segment);
  mixed_free_segment_sequence(&sequence);
  free_mp3(mp3);
  free_out(out);
  
  mpg123_exit();
  return exit;
}
