#include "common.h"

double mtime(){
  struct timespec spec;
  if(clock_gettime(CLOCK_MONOTONIC, &spec) == 0){
    return spec.tv_sec + ((double)spec.tv_nsec) / ((double)1E9);
  }
  return 0.0;
}

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 100;
  size_t samplerate = 44100;
  struct mixed_segment_sequence sequence = {0};
  struct mixed_segment space = {0};
  struct out *out = 0;
  struct mp3 *mp3 = 0;
  float dphi = 1.0; // Measured deg/s
  float r = 100.0;  // Measured in cm

  signal(SIGINT, interrupt_handler);

  if(argc < 2 || 4 < argc){
    fprintf(stderr, "Usage: ./test_space mp3-file [speed] [radius]\n");
    return 0;
  }

  if(3 <= argc){
    char *end;
    dphi = strtod(argv[2], &end);
    if(*end != '\0'){
      fprintf(stderr, "Cannot use '%s' as an angle speed.\n", argv[2]);
      goto cleanup;
    }
  }

  if(4 <= argc){
    char *end;
    r = strtod(argv[3], &end);
    if(*end != '\0'){
      fprintf(stderr, "Cannot use '%s' as a radius.\n", argv[3]);
      goto cleanup;
    }
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

  if(!mixed_make_segment_space_mixer(samplerate, &space)){
    fprintf(stderr, "Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_in(MIXED_BUFFER, 0, &mp3->left, &space) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &space) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &out->right, &space)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_sequence_add(&mp3->segment, &sequence) ||
     !mixed_segment_sequence_add(&space, &sequence) ||
     !mixed_segment_sequence_add(&out->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  mixed_segment_sequence_start(&sequence);

  double dt = ((double)samples) / ((double)samplerate);
  float phi = 0.0;
  float vel[3] = {0.0, 0.0, 0.0};
  float pos[3] = {r, 0.0, 0.0};
  float rad;
  
  size_t read, played;
  do{
    if(mpg123_read(mp3->handle, mp3->pack.data, mp3->pack.size, &read) != MPG123_OK){
      fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
      goto cleanup;
    }
    
    for(size_t j=read; j<mp3->pack.size; ++j){
      ((uint8_t *)mp3->pack.data)[j] = 0;
    }
    
    // Calculate new position
    phi += dphi * dt;
    rad = phi * M_PI / 180.0;
    vel[0] = r*2.0*cos(rad) - pos[0];
    vel[2] = r*sin(rad) - pos[2];
    pos[0] = pos[0] + vel[0];
    pos[2] = pos[2] + vel[2];

    mixed_segment_set_in(MIXED_SPACE_LOCATION, 0, pos, &space);
    mixed_segment_set_in(MIXED_SPACE_VELOCITY, 0, vel, &space);
    
    mixed_segment_sequence_mix(&sequence);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    size_t bytes = out->pack.frames * out->pack.channels * mixed_samplesize(out->pack.encoding);
    played = out123_play(out->handle, out->pack.data, bytes);
    if(played < bytes){
      fprintf(stderr, "Warning: device not catching up with input (%i vs %i)\n", played, bytes);
    }
  }while(read && !interrupted);
  
  mixed_segment_sequence_end(&sequence);

  exit = 0;

 cleanup:
  mixed_free_segment_sequence(&sequence);
  mixed_free_segment(&space);
  
  free_out(out);
  free_mp3(mp3);
  
  return exit;
}
