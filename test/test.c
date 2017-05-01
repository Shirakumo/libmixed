#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <mpg123.h>
#include <out123.h>
#include "mixed.h"

char volatile interrupted = 0;

void interrupt_handler(int shim){
  interrupted = 1;
}

int fmt123_to_mixed(int fmt){
  switch(fmt){
  case MPG123_ENC_SIGNED_8: return MIXED_INT8;
  case MPG123_ENC_UNSIGNED_8: return MIXED_UINT8;
  case MPG123_ENC_SIGNED_16: return MIXED_INT16;
  case MPG123_ENC_UNSIGNED_16: return MIXED_UINT16;
  case MPG123_ENC_SIGNED_24: return MIXED_INT24;
  case MPG123_ENC_UNSIGNED_24: return MIXED_UINT24;
  case MPG123_ENC_SIGNED_32: return MIXED_INT32;
  case MPG123_ENC_UNSIGNED_32: return MIXED_UINT32;
  case MPG123_ENC_FLOAT_32: return MIXED_FLOAT;
  case MPG123_ENC_FLOAT_64: return MIXED_DOUBLE;
  default: return -1;
  }
}

struct mp3{
  mpg123_handle *handle;
  struct mixed_channel channel;
  struct mixed_segment segment;
  struct mixed_buffer left;
  struct mixed_buffer right;
};

void free_mp3(struct mp3 *mp3){
  if(!mp3) return;
  if(mp3->handle){
    mpg123_close(mp3->handle);
    mpg123_delete(mp3->handle);
  }
  if(mp3->channel.data){
    free(mp3->channel.data);
  }
  mixed_free_segment(&mp3->segment);
  mixed_free_buffer(&mp3->left);
  mixed_free_buffer(&mp3->right);
  free(mp3);
}

int load_mp3_segment(char *file, size_t samples, struct mp3 **_mp3){
  long mp3_samplerate = 0;
  int mp3_channels = 0;
  int mp3_encoding = 0;
  char *mp3_encname = 0;
  uint8_t mp3_samplesize = 0;
  struct mp3 *mp3 = calloc(1, sizeof(struct mp3));

  if(!mp3){
    printf("Failed to allocate mixer data.\n");
    goto cleanup;
  }

  mp3->handle = mpg123_new(NULL, 0);
  if(mp3->handle == 0){
    printf("Failed to create MPG123 handle.\n");
    goto cleanup;
  }

  if(mpg123_open(mp3->handle, file) != MPG123_OK){
    printf("Failed to open %s: %s\n", file, mpg123_strerror(mp3->handle));
    goto cleanup;
  }
  
  if(mpg123_getformat(mp3->handle, &mp3_samplerate, &mp3_channels, &mp3_encoding) != MPG123_OK){
    printf("Failed to get format properties of %s: %s\n", file, mpg123_strerror(mp3->handle));
    goto cleanup;
  }

  // test app limitation for now
  if(mp3_channels != 2){
    printf("File %s has %i channels instead of 2. I can't deal with this.\n", file, mp3_channels);
    goto cleanup;
  }
  // libmixed limitation for now
  if(mp3_samplerate != 44100){
    printf("File %s has a sample rate of %i Hz instead of 44100. I can't deal with this.\n", file, mp3_samplerate);
    goto cleanup;
  }
  
  mp3_encname = (char *)out123_enc_longname(mp3_encoding);
  printf("MP3: %i channels @ %li Hz, %s\n", mp3_channels, mp3_samplerate, mp3_encname);

  mp3->channel.encoding = fmt123_to_mixed(mp3_encoding);
  mp3->channel.channels = mp3_channels;
  mp3->channel.layout = MIXED_ALTERNATING;
  mp3->channel.samplerate = mp3_samplerate;
  mp3_samplesize = mixed_samplesize(mp3->channel.encoding);
  mp3->channel.data = calloc(samples*mp3_samplesize, sizeof(uint8_t));
  mp3->channel.size = samples*mp3_samplesize;

  if(!mixed_make_segment_source(&mp3->channel, &mp3->segment)){
    printf("Failed to create segment for %s: %s\n", file, mixed_error_string(-1));
    goto cleanup;
  }

  mp3->left.size = samples;
  mp3->right.size = samples;
  if(   !mixed_make_buffer(&mp3->left)
     || !mixed_make_buffer(&mp3->right)){
    printf("Failed to allocate mixer buffers: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(   !mixed_segment_set_buffer(0, &mp3->left,  &mp3->segment)
     || !mixed_segment_set_buffer(1, &mp3->right, &mp3->segment)){
    printf("Failed to set buffers for %s: %s\n", file, mixed_error_string(-1));
    goto cleanup;
  }

  *_mp3 = mp3;
  return 1;

 cleanup:
  free_mp3(mp3);
  return 0;
}

int main(int argc, char **argv){
  int exit = 1;
  size_t samples = 1024;
  out123_handle *oh = 0;
  long out_samplerate = 44100;
  int out_channels = 2;
  int out_encoding = MPG123_ENC_SIGNED_16;
  int out_framesize = 0;
  uint8_t out_samplesize = 0;
  char *out_encname = "signed 16 bit";
  struct mixed_channel out_channel = {0};
  struct mixed_segment out_segment = {0};
  struct mixed_buffer left = {0};
  struct mixed_buffer right = {0};
  struct mixed_mixer mixer = {0};
  struct mixed_segment lmix_segment = {0};
  struct mixed_segment rmix_segment = {0};
  struct mixed_segment gen_segment = {0};
  struct mp3 *mp3s[argc-1];

  signal(SIGINT, interrupt_handler);
  
  if(argc<2){
    printf("Please pass at least one audio file to play.\n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    printf("Failed to init MPG123.\n");
    goto cleanup;
  }

  // Connect to the audio system
  oh = out123_new();
  if(oh == 0){
    printf("Failed to create OUT123 handle.\n");
    goto cleanup;
  }

  if(out123_open(oh, 0, 0) != OUT123_OK){
    printf("Failed to open sound device: %s\n", out123_strerror(oh));
    goto cleanup;
  }

  if(out123_start(oh, out_samplerate, out_channels, MPG123_ENC_FLOAT_32)){
    printf("Failed to start playback on device: %s\n", out123_strerror(oh));
    goto cleanup;
  }
  
  if(out123_getformat(oh, &out_samplerate, &out_channels, &out_encoding, &out_framesize) != OUT123_OK){
    printf("Failed to get format properties of your device: %s\n", out123_strerror(oh));
    goto cleanup;
  }

  out_encname = (char *)out123_enc_longname(out_encoding);
  printf("OUT: %i channels @ %li Hz, %s\n", out_channels, out_samplerate, out_encname);
  
  // Prepare pipeline segments
  out_channel.encoding = fmt123_to_mixed(out_encoding);
  out_channel.channels = out_channels;
  out_channel.layout = MIXED_ALTERNATING;
  out_channel.samplerate = out_samplerate;
  out_samplesize = mixed_samplesize(out_channel.encoding);
  out_channel.data = calloc(samples*out_samplesize, sizeof(uint8_t));
  out_channel.size = samples*out_samplesize;

  if(!out_channel.data){
    printf("Couldn't allocate output buffer.\n");
    goto cleanup;
  }
  
  if(   !mixed_make_segment_mixer(0, &lmix_segment)
     || !mixed_make_segment_mixer(0, &rmix_segment)
     || !mixed_make_segment_general(1.0, 0.0, &gen_segment)
     || !mixed_make_segment_drain(&out_channel, &out_segment)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  left.size = samples;
  right.size = samples;
  if(   !mixed_make_buffer(&left)
     || !mixed_make_buffer(&right)){
    printf("Failed to allocate mixer buffers: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  if(   // The mixer segment's 0th buffer is its output.
        !mixed_segment_set_buffer(0, &left, &lmix_segment)
     || !mixed_segment_set_buffer(0, &right, &rmix_segment)
        // The general segment has two ins and two outs. But since it is
        // specified as being in-place, we can attach the same buffers.
     || !mixed_segment_set_buffer(0, &left, &gen_segment)
     || !mixed_segment_set_buffer(1, &right, &gen_segment)
     || !mixed_segment_set_buffer(2, &left, &gen_segment)
     || !mixed_segment_set_buffer(3, &right, &gen_segment)
        // The drain needs two input channels to play back from.
     || !mixed_segment_set_buffer(0, &left, &out_segment)
     || !mixed_segment_set_buffer(1, &right, &out_segment)){
    printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Initialize MP3 sources
  for(size_t i=1; i<argc; ++i){
    struct mp3 *mp3 = 0;
    if(!load_mp3_segment(argv[i], samples, &mp3)){
      goto cleanup;
    }
    mp3s[i-1] = mp3;
    // Attach to combining segments
    if(   !mixed_segment_set_buffer(i, &mp3->left, &lmix_segment)
       || !mixed_segment_set_buffer(i, &mp3->right, &rmix_segment)){
      printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    // Register with mixer.
    if(!mixed_mixer_add(&mp3->segment, &mixer)){
      printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
  }

  // The order in which segments are added does matter, as the system
  // will not figure out on its own in which order the segments should
  // be called. We specify this here.
  if(   !mixed_mixer_add(&lmix_segment, &mixer)
     || !mixed_mixer_add(&rmix_segment, &mixer)
     || !mixed_mixer_add(&gen_segment, &mixer)
     || !mixed_mixer_add(&out_segment, &mixer)){
    printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Perform the mixing
  mixer.samplerate = out_samplerate;
  mixed_mixer_start(&mixer);

  size_t read = 0, read_total = 0, played = 0;
  do{
    read_total = 0;
    for(size_t i=0; i<argc-1; ++i){
      if(mpg123_read(mp3s[i]->handle, mp3s[i]->channel.data, mp3s[i]->channel.size, &read) != MPG123_OK){
        printf("Failure during MP3 decoding: %s\n", mpg123_strerror(mp3s[i]->handle));
        goto cleanup;
      }
      // Make sure we play until everything's done.
      if(read_total < read) read_total = read;
      // Make sure to zero out empty regions.
      for(size_t j=read; j<mp3s[i]->channel.size; ++j){
        ((uint8_t *)mp3s[i]->channel.data)[j] = 0;
      }
    }
    
    if(!mixed_mixer_mix(samples, &mixer)){
      printf("Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    
    played = out123_play(oh, out_channel.data, out_channel.size);
    if(played < samples){
      printf("Warning: device not catching up with input (%i vs %i)\n", played, samples);
    }
  }while(read_total && !interrupted);

  mixed_mixer_end(&mixer);

  // Clean the shop
  exit = 0;
  
 cleanup:
  printf("\nCleaning up.\n");
  
  if(oh){
    out123_close(oh);
    out123_del(oh);
  }

  if(out_channel.data){
    free(out_channel.data);
  }
  
  mixed_free_segment(&lmix_segment);
  mixed_free_segment(&rmix_segment);
  mixed_free_segment(&gen_segment);
  mixed_free_segment(&out_segment);
  mixed_free_buffer(&left);
  mixed_free_buffer(&right);
  mixed_free_mixer(&mixer);

  for(size_t i=1; i<argc; ++i){
    if(mp3s[i]){
      free_mp3(mp3s[i-1]);
    }
  }
  
  mpg123_exit();
  return exit;
}
