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
  uint32_t samples = 100;
  WINDOW *window = 0;
  struct mixed_segment chain = {0}, space = {0}, downmix = {0};
  struct mixed_buffer mono = {0};
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

  if(!mixed_make_buffer(samples, &mono)){
    goto cleanup;
  }

  if(!mixed_make_segment_space_mixer(internal_samplerate, &space)
     || !mixed_make_segment_channel_convert(2, 1, internal_samplerate, &downmix)
     || !mixed_make_segment_chain(&chain)){
    fprintf(stderr, "Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &mono, &downmix)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &downmix)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &mp3->right, &downmix)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &mono, &space)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &space)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &out->right, &space)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_chain_add(&mp3->segment, &chain)
     || !mixed_chain_add(&downmix, &chain)
     || !mixed_chain_add(&space, &chain)
     || !mixed_chain_add(&out->segment, &chain)){
    fprintf(stderr, "Failed to assemble chain: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Start up ncurses
  window = load_curses();
  if(window)mvprintw(0, 0, "<←/→>: Change speed <↑/↓>: Change radius <SPC>: Toggle doppler effect");

  double dt = ((double)samples) / ((double)internal_samplerate);
  float phi = 0.0;
  float vel[3] = {0.0, 0.0, 0.0};
  float pos[3] = {r, 0.0, 0.0};
  float rad;
  float doppler = 0.0;
  
  // Perform the mixing
  mixed_segment_start(&chain);
  size_t read = 0, played = 0;
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
    mixed_pack_finish_read(played, &out->pack);
    
    // IO
    if(!window) continue;
    int c = getch();
    switch(c){
    case 'q': interrupted = 1; break;
    case KEY_LEFT: dphi /= 1.0625; break;
    case KEY_RIGHT: dphi *= 1.0625; break;
    case KEY_UP: r += 1; break;
    case KEY_DOWN: r -= 1; break;
    case ' ': doppler = (doppler==1.0)?0.0:1.0; break;
    }
    
    phi += dphi * dt;
    rad = phi * M_PI / 180.0;
    vel[0] = r*2.0*cos(rad) - pos[0];
    vel[2] = r*sin(rad) - pos[2];
    pos[0] = pos[0] + vel[0];
    pos[2] = pos[2] + vel[2];
    mixed_segment_set_in(MIXED_SPACE_LOCATION, 0, pos, &space);
    mixed_segment_set_in(MIXED_SPACE_VELOCITY, 0, vel, &space);
    mixed_segment_set(MIXED_SPACE_DOPPLER_FACTOR, &doppler, &space);

    mvprintw(1, 0, "Read: %4u Processed: %4u Played: %4u Speed: %f Radius: %f\n", read, bytes, played, dphi, r);
    refresh();
  }while(!interrupted);
  mixed_segment_end(&chain);

  exit = 0;

 cleanup:
  mixed_free_segment(&chain);
  mixed_free_segment(&downmix);
  mixed_free_segment(&space);
  mixed_free_buffer(&mono);
  
  free_out(out);
  free_mp3(mp3);
  free_curses(window);

  mpg123_exit();
  return exit;
}
