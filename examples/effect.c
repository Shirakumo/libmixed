#include <curses.h>
#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 500;
  WINDOW *window = 0;
  struct mixed_segment_sequence sequence = {0};
  struct mixed_segment sfx_l = {0}, sfx_r = {0};
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

  double speed = 1.0;
  if(!mixed_make_segment_speed_change(speed, &sfx_l)
     || !mixed_make_segment_speed_change(speed, &sfx_r)){
    fprintf(stderr, "Failed to create segment: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Initialize MP3 source
  if(!load_mp3_segment(argv[1], samples, &mp3)){
    goto cleanup;
  }
  
  // Attach to combining segments
  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &sfx_l)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &sfx_l)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->right, &sfx_r)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->right, &sfx_r)){
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
     !mixed_segment_sequence_add(&sfx_l, &sequence) ||
     !mixed_segment_sequence_add(&sfx_r, &sequence) ||
     !mixed_segment_sequence_add(&out->segment, &sequence)){
    fprintf(stderr, "Failed to assemble sequence: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Start up ncurses
  window = load_curses();
  mvprintw(0, 0, "<←/→>: Change speed");

  // Perform the mixing
  mixed_segment_sequence_start(&sequence);
  size_t read = 0, played = 0;
  do{
    void *buffer;
    read = SIZE_MAX;
    mixed_pack_request_write(&buffer, &read, &mp3->pack);
    if(mpg123_read(mp3->handle, buffer, read, &read) != MPG123_OK){
      fprintf(stderr, "Failure during MP3 decoding: %s\n", mpg123_strerror(mp3->handle));
      goto cleanup;
    }
    mixed_pack_finish_write(read, &mp3->pack);
    
    mixed_segment_sequence_mix(&sequence);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    size_t bytes = SIZE_MAX;
    mixed_pack_request_read(&buffer, &bytes, &out->pack);
    played = out123_play(out->handle, buffer, bytes);
    mixed_pack_finish_read(played, &out->pack);

    // IO
    int c = getch();
    switch(c){
    case 'q': interrupted = 1; break;
    case KEY_LEFT: speed *= 0.9; break;
    case KEY_RIGHT: speed *= 1.1; break;
    }
    mixed_segment_set(MIXED_SPEED_FACTOR, &speed, &sfx_l);
    mixed_segment_set(MIXED_SPEED_FACTOR, &speed, &sfx_r);

    mvprintw(1, 0, "Read: %4i Processed: %4i Played: %4i Speed: %f", read, bytes, played, speed);
    refresh();
  }while(played && !interrupted);
  mixed_segment_sequence_end(&sequence);

  exit = 0;
  
 cleanup:
  mixed_free_segment(&sfx_l);
  mixed_free_segment(&sfx_r);
  mixed_free_segment_sequence(&sequence);

  free_mp3(mp3);
  free_out(out);
  free_curses(window);
  
  mpg123_exit();
  return exit;
}
