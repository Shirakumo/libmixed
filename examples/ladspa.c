#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 4096;
  size_t samplerate = 44100;
  struct mixed_segment_sequence sequence = {0};
  struct mixed_segment ladspa = {0};
  struct mp3 *mp3 = 0;
  struct out *out = 0;

  signal(SIGINT, interrupt_handler);

  if(argc < 3){
    fprintf(stderr, "Usage: ./test_ladspa mp3-file ladspa-file ladspa-param* \n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    fprintf(stderr, "Failed to init MPG123.\n");
    goto cleanup;
  }

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }

  if(!load_mp3_segment(argv[1], samples, &mp3)){
    goto cleanup;
  }

  if(!mixed_make_segment_ladspa(argv[2], 0, samplerate, &ladspa)){
    fprintf(stderr, "Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  float value;
  for(int i=3; i<argc; ++i){
    char *end;
    float value = strtod(argv[i], &end);
    if(*end != '\0'){
      fprintf(stderr, "Cannot use '%s' as a float parameter to the LADSPA plugin.\n", argv[i]);
      goto cleanup;
    }
    mixed_segment_set(i-3, &value, &ladspa);
  }

  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &ladspa) ||
     !mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &mp3->right, &ladspa) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &ladspa) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &out->right, &ladspa)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_sequence_add(&mp3->segment, &sequence) ||
     !mixed_segment_sequence_add(&ladspa, &sequence) ||
     !mixed_segment_sequence_add(&out->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  mixed_segment_sequence_start(&sequence);

  size_t read, played;
  do{
    if(mpg123_read(mp3->handle, mp3->pack.data, mp3->pack.size, &read) != MPG123_OK){
      fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
      goto cleanup;
    }
    
    for(size_t j=read; j<mp3->pack.size; ++j){
      ((uint8_t *)mp3->pack.data)[j] = 0;
    }
    
    mixed_segment_sequence_mix(samples, &sequence);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    played = out123_play(out->handle, out->pack.data, out->pack.size);
    if(played < out->pack.size){
      fprintf(stderr, "Warning: device not catching up with input (%i vs %i)\n", played, samples);
    }
  }while(read && !interrupted);
  
  mixed_segment_sequence_end(&sequence);

  exit = 0;

 cleanup:
  mixed_free_segment_sequence(&sequence);
  mixed_free_segment(&ladspa);
  
  free_mp3(mp3);
  free_out(out);
  
  return exit;
}
