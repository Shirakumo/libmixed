#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 500;
  WINDOW *window = 0;
  struct mixed_segment chain = {0}, sfx_l = {0}, sfx_r = {0};
  struct mp3 *mp3 = 0;
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

  if(!load_mp3_segment(argv[1], samples, &mp3)){
    goto cleanup;
  }

  double speed = 1.0;
  if(!mixed_make_segment_speed_change(speed, &sfx_l)
     || !mixed_make_segment_speed_change(speed, &sfx_r)
     || !mixed_make_segment_chain(&chain)){
    fprintf(stderr, "Failed to create segment: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &mp3->left, &sfx_l)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->left, &sfx_l)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &mp3->right, &sfx_r)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->right, &sfx_r)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(!mixed_chain_add(&mp3->segment, &chain) ||
     !mixed_chain_add(&sfx_l, &chain) ||
     !mixed_chain_add(&sfx_r, &chain) ||
     !mixed_chain_add(&out->segment, &chain)){
    fprintf(stderr, "Failed to assemble chain: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Start up ncurses
  //window = load_curses();
  if(window) mvprintw(0, 0, "<←/→>: Change speed <SPC>: Reset");

  // Perform the mixing
  if(!mixed_segment_start(&chain)){
    fprintf(stderr, "Failure starting the segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
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
    case KEY_LEFT: speed /= 1.0625; break;
    case KEY_RIGHT: speed *= 1.0625; break;
    case ' ': speed = 1.0; break;
    }
    mixed_segment_set(MIXED_SPEED_FACTOR, &speed, &sfx_l);
    mixed_segment_set(MIXED_SPEED_FACTOR, &speed, &sfx_r);

    mvprintw(1, 0, "Read: %4i Processed: %4i Played: %4i Speed: %f", read, bytes, played, speed);
    refresh();
  }while(played && !interrupted);
  mixed_segment_end(&chain);

  exit = 0;
  
 cleanup:
  mixed_free_segment(&sfx_l);
  mixed_free_segment(&sfx_r);
  mixed_free_segment(&chain);

  free_mp3(mp3);
  free_out(out);
  free_curses(window);
  
  mpg123_exit();
  return exit;
}
