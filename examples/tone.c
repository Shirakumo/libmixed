#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 100;
  WINDOW *window = 0;
  struct mixed_segment chain = {0}, generator = {0}, upmix = {0};
  struct mixed_segment fade = {0};
  struct mixed_buffer buffer = {0};
  struct out *out = 0;

  enum mixed_generator_type wave_type = MIXED_SINE;
  float frequency = 440;

  if(argc < 3){
    fprintf(stderr, "Usage: ./test_tone wave-type frequency \n");
    return 0;
  }

  if(0 == strcmp("sine", argv[1])) wave_type = MIXED_SINE;
  else if(0 == strcmp("square", argv[1])) wave_type = MIXED_SQUARE;
  else if(0 == strcmp("triangle", argv[1])) wave_type = MIXED_TRIANGLE;
  else if(0 == strcmp("sawtooth", argv[1])) wave_type = MIXED_SAWTOOTH;
  else{
    fprintf(stderr, "Invalid wave type. Must be one of sine, square, triangle, sawtooth.\n");
    goto cleanup;
  }

  char *end;
  frequency = strtod(argv[2], &end);
  if(*end != '\0'){
    fprintf(stderr, "Cannot use '%s' as a frequency.\n", argv[2]);
    goto cleanup;
  }
  
  signal(SIGINT, interrupt_handler);

  if(!load_out_segment(samples, &out)){
    goto cleanup;
  }

  if(!mixed_make_buffer(samples, &buffer)){
    goto cleanup;
  }

  if(!mixed_make_segment_generator(wave_type, frequency, internal_samplerate, &generator)
     || !mixed_make_segment_channel_convert(1, 2, internal_samplerate, &upmix)
     || !mixed_make_segment_fade(0.0, 0.8, 5.0, MIXED_CUBIC_IN_OUT, internal_samplerate, &fade)
     || !mixed_make_segment_chain(&chain)){
    fprintf(stderr, "Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &generator)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &fade)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &buffer, &fade)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &buffer, &upmix)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &out->left, &upmix)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &out->right, &upmix)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_chain_add(&generator, &chain) ||
     !mixed_chain_add(&fade, &chain) ||
     !mixed_chain_add(&upmix, &chain) ||
     !mixed_chain_add(&out->segment, &chain)){
    fprintf(stderr, "Failed to assemble chain: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  // Start up ncurses
  window = load_curses();
  mvprintw(0, 0, "<←/→>: Change frequency <SPC>: Cycle wave-type");

  mixed_segment_start(&chain);
  size_t played;
  do{
    mixed_segment_mix(&chain);
    if(mixed_error() != MIXED_NO_ERROR){
      fprintf(stderr, "Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }

    void *buffer;
    uint32_t bytes = UINT32_MAX;
    mixed_pack_request_read(&buffer, &bytes, &out->pack);
    played = out123_play(out->handle, buffer, bytes);
    mixed_pack_finish_read(played, &out->pack);
    
    // IO
    int c = getch();
    switch(c){
    case 'q': interrupted = 1; break;
    case KEY_LEFT: frequency -= 10; break;
    case KEY_RIGHT: frequency += 10; break;
    case ' ': switch(wave_type){
      case MIXED_SINE: wave_type = MIXED_SQUARE; break;
      case MIXED_SQUARE: wave_type = MIXED_TRIANGLE; break;
      case MIXED_TRIANGLE: wave_type = MIXED_SAWTOOTH; break;
      case MIXED_SAWTOOTH: wave_type = MIXED_SINE; break;
      }break;
    }
    mixed_segment_set(MIXED_GENERATOR_FREQUENCY, &frequency, &generator);
    mixed_segment_set(MIXED_GENERATOR_TYPE, &wave_type, &generator);

    mvprintw(1, 0, "Processed: %4i Played: %4i Frequency: %f Type: %i", bytes, played, frequency, wave_type);
    refresh();
  }while(!interrupted);
  
  mixed_segment_end(&chain);

  exit = 0;

 cleanup:
  mixed_free_segment(&chain);
  mixed_free_segment(&generator);
  mixed_free_segment(&fade);
  free_out(out);
  free_curses(window);
  
  return exit;
}
