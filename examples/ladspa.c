#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 4096;
  uint32_t samplerate = 44100;
  struct mixed_segment chain = {0}, ladspa = {0};
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

  if(!mixed_make_segment_ladspa(argv[2], 0, samplerate, &ladspa)
     || !mixed_make_segment_chain(&chain)){
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

  if(!mixed_chain_add(&mp3->segment, &chain) ||
     !mixed_chain_add(&ladspa, &chain) ||
     !mixed_chain_add(&out->segment, &chain)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  mixed_segment_start(&chain);

  size_t read, played;
  do{
    void *buffer;
    uint32_t read2 = UINT32_MAX;
    mixed_pack_request_write(&buffer, &read2, &mp3->pack);
    if(mpg123_read(mp3->handle, buffer, read2, &read) != MPG123_OK){
      fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
      goto cleanup;
    }
    mixed_pack_finish_write(read, &mp3->pack);
    
    mixed_segment_mix(&chain);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }

    uint32_t bytes = UINT32_MAX;
    mixed_pack_request_read(&buffer, &bytes, &out->pack);
    played = out123_play(out->handle, buffer, bytes);
    if(played < out->pack.size){
      fprintf(stderr, "Warning: device not catching up with input (%lu vs %u)\n", played, bytes);
    }
    mixed_pack_finish_read(played, &out->pack);
  }while(played && !interrupted);
  
  mixed_segment_end(&chain);

  exit = 0;

 cleanup:
  mixed_free_segment(&chain);
  mixed_free_segment(&ladspa);
  
  free_mp3(mp3);
  free_out(out);
  
  return exit;
}
