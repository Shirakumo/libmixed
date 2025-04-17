#include <unistd.h>
#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 500;
  uint32_t samplerate = 44100;
  uint32_t framesize = 2048;
  WINDOW *window = 0;
  struct mixed_buffer o = {0}, fftbuf = {0};
  struct mixed_segment chain = {0}, fft = {0}, downmix = {0};
  struct mp3 *mp3 = 0;

  signal(SIGINT, interrupt_handler);
  
  if(argc<2){
    fprintf(stderr, "Usage: ./test_fft mp3-file \n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    fprintf(stderr, "Failed to init MPG123.\n");
    goto cleanup;
  }

  if(!load_mp3_segment(argv[1], samples, &mp3)){
    goto cleanup;
  }

  samplerate = mp3->pack.samplerate;

  if(!mixed_make_segment_fwd_fft(samplerate, &fft)
     || !mixed_make_segment_channel_convert(2, 1, samplerate, &downmix)
     || !mixed_make_segment_chain(&chain)
     || !mixed_make_buffer(samples, &o)
     || !mixed_make_buffer(framesize, &fftbuf)){
    fprintf(stderr, "Failed to create segment: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &downmix)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &mp3->right, &downmix)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &o, &downmix)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &o, &fft)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &fftbuf, &fft)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(!mixed_chain_add(&mp3->segment, &chain)
     || !mixed_chain_add(&downmix, &chain)
     || !mixed_chain_add(&fft, &chain)){
    fprintf(stderr, "Failed to assemble chain: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Start up ncurses
  window = load_curses();
  
  // Perform the mixing
  if(!mixed_segment_start(&chain)){
    fprintf(stderr, "Failure starting the segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  size_t read = 0;
  uint32_t fft_processed = 0, width, height;
  float bytes_to_usecs = 1.0e6 / (mixed_samplesize(mp3->pack.encoding) * mp3->pack.channels * mp3->pack.samplerate);
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

    // Consume FFT output
    float *freq;
    fft_processed = UINT32_MAX;
    mixed_buffer_request_read(&freq, &fft_processed, &fftbuf);
    mixed_buffer_finish_read(fft_processed, &fftbuf);

    getmaxyx(window, height, width);
    height = ((height<32)? height : 32)-1;
    uint32_t binsize = (framesize/2<width)? 1 : (framesize/2) / width;
    size_t i=1;
    for(uint32_t bin=0; i<fft_processed; ++bin){
      // Average the current bin.
      float sample = 0.0;
      for(int j=0; j<binsize; ++j){
        // The array contains [frequency, amplitude, ...]
        // We only care about the amplitude and skip the exact frequency.
        sample += freq[i];
        i+=2;
      }
      sample = (sample * height * 0.1) / binsize;
      // Draw the bar
      for(int row=0; row<height; ++row){
        const char *bars[] = {"█", "▇", "▆", "▅", "▄", "▃", "▂", "▁", " "};
        int bar = round((row-sample+1)*8);
        if(bar < 1) bar = 0;
        if(7 < bar) bar = 8;
        mvprintw(height-row-1, bin, "%s", bars[(char)bar]);
      }
    }
    
    mvprintw(height, 0, "Read: %4u FFT: %4u Binsize: %i", read, fft_processed, binsize);
    refresh();
    
    // Simulate playback
    usleep(read * bytes_to_usecs);
  }while(read && !interrupted);
  mixed_segment_end(&chain);

  exit = 0;
  
 cleanup:
  mixed_free_segment(&downmix);
  mixed_free_segment(&fft);
  mixed_free_segment(&chain);
  mixed_free_buffer(&o);

  free_mp3(mp3);
  free_curses(window);
  
  mpg123_exit();
  return exit;
}
