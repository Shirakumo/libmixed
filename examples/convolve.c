#include "common.h"

int main(int argc, char **argv){
  int exit = 1;
  uint32_t samples = 100;
  struct mixed_segment chain = {0}, conv_l = {0}, conv_r = {0};
  struct mixed_pack fir = {0};
  struct mixed_buffer firs[2] = {0};
  struct out *out = 0;
  struct mp3 *mp3 = 0;

  signal(SIGINT, interrupt_handler);

  if(argc != 3){
    fprintf(stderr, "Usage: ./test_space mp3-file wav-file\n");
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

  if(!load_wav_pack(argv[2], &fir)){
    goto cleanup;
  }
  uint32_t frames = mixed_pack_available_read(&fir) / mixed_byte_stride(fir.channels, fir.encoding);

  switch(fir.channels){
  case 1:
  case 2: {
    if(!mixed_make_buffer(frames, &firs[0])
       || !mixed_make_buffer(frames, &firs[1])){
      fprintf(stderr, "Failed to create buffers: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    float volume = 1.0;
    struct mixed_buffer *_firs[2] = {&firs[0], &firs[1]};
    if(!mixed_buffer_from_pack(&fir, _firs, &volume, 1.0)){
      fprintf(stderr, "Failed to decode WAV file: %s\n", mixed_error_string(-1));
      goto cleanup;
    }}
    break;
  default:
    fprintf(stderr, "WAV file contains more than two channels.\n");
  }

  if(!mixed_make_segment_convolution(2048, firs[0]._data, firs[0].size, internal_samplerate, &conv_l)
     || !mixed_make_segment_convolution(2048, firs[1]._data, firs[0].size, internal_samplerate, &conv_r)
     || !mixed_make_segment_chain(&chain)){
    fprintf(stderr, "Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &mp3->left, &conv_l)
     || !mixed_segment_set_in(MIXED_BUFFER, MIXED_MONO, &mp3->right, &conv_r)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->left, &conv_l)
     || !mixed_segment_set_out(MIXED_BUFFER, MIXED_MONO, &out->right, &conv_r)){
    fprintf(stderr, "Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_chain_add(&mp3->segment, &chain)
     || !mixed_chain_add(&conv_l, &chain)
     || !mixed_chain_add(&conv_r, &chain)
     || !mixed_chain_add(&out->segment, &chain)){
    fprintf(stderr, "Failed to assemble chain: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

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
  }while(!interrupted);
  mixed_segment_end(&chain);

  exit = 0;

 cleanup:
  mixed_free_segment(&chain);
  mixed_free_segment(&conv_l);
  mixed_free_segment(&conv_r);
  mixed_free_buffer(&firs[0]);
  mixed_free_buffer(&firs[1]);
  mixed_free_pack(&fir);
  
  free_out(out);
  free_mp3(mp3);

  mpg123_exit();
  return exit;
}
