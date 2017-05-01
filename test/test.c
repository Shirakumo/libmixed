#include <stdio.h>
#include <stdint.h>
#include <mpg123.h>
#include <out123.h>
#include "mixed.h"

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

int main(int argc, char **argv){
  int exit = 1;
  mpg123_handle *mh = 0;
  long mp3_samplerate = 0;
  int mp3_channels = 0;
  int mp3_encoding = 0;
  char *mp3_encname = 0;
  struct mixed_channel mp3_channel = {0};
  struct mixed_segment mp3_segment = {0};
  out123_handle *oh = 0;
  long out_samplerate = 44100;
  int out_channels = 2;
  int out_encoding = MPG123_ENC_SIGNED_16;
  int out_framesize = 0;
  char *out_encname = "signed 16";
  struct mixed_channel out_channel = {0};
  struct mixed_segment out_segment = {0};
  struct mixed_buffer left = {0};
  struct mixed_buffer right = {0};
  struct mixed_mixer mixer = {0};
  struct mixed_segment gen_segment = {0};
  int8_t *buffer = 0;
  size_t buffersize = 0;
  
  if(argc<2){
    printf("Please pass an audio file to play.\n");
    return 0;
  }

  if(mpg123_init() != MPG123_OK){
    printf("Failed to init MPG123.\n");
    goto cleanup;
  }

  mh = mpg123_new(NULL, 0);
  if(mh == 0){
    printf("Failed to create MPG123 handle.\n");
    goto cleanup;
  }

  if(mpg123_open(mh, argv[1]) != MPG123_OK){
    printf("Failed to open your file: %s\n", mpg123_strerror(mh));
    goto cleanup;
  }
  if(mpg123_getformat(mh, &mp3_samplerate, &mp3_channels, &mp3_encoding) != MPG123_OK){
    printf("Failed to get format properties of your file: %s\n", mpg123_strerror(mh));
    goto cleanup;
  }
  
  mp3_encname = out123_enc_longname(mp3_encoding);
  printf("MP3: %i channels @ %li Hz, %s\n", mp3_channels, mp3_samplerate, mp3_encname);

  oh = out123_new();
  if(oh == 0){
    printf("Failed to create OUT123 handle.\n");
    goto cleanup;
  }

  if(out123_open(oh, 0, 0) != OUT123_OK){
    printf("Failed to open sound device: %s\n", out123_strerror(oh));
    goto cleanup;
  }

  if(out123_start(oh, mp3_samplerate, mp3_channels, MPG123_ENC_FLOAT_32)){
    printf("Failed to start playback on device: %s\n", out123_strerror(oh));
    goto cleanup;
  }
  
  if(out123_getformat(oh, &out_samplerate, &out_channels, &out_encoding, &out_framesize) != OUT123_OK){
    printf("Failed to get format properties of your device: %s\n", out123_strerror(oh));
    goto cleanup;
  }

  out_encname = out123_enc_longname(out_encoding);
  printf("OUT: %i channels @ %li Hz, %s\n", out_channels, out_samplerate, out_encname);

  buffersize = 4096;
  buffer = calloc(buffersize, sizeof(uint8_t));
  if(!buffer){
    printf("Couldn't allocate temporary buffer.\n");
    goto cleanup;
  }
  
  /////
  // Start mixed stuff
  
  // Prepare pipeline segments
  mp3_channel.data = buffer;
  mp3_channel.size = buffersize;
  mp3_channel.encoding = fmt123_to_mixed(mp3_encoding);
  mp3_channel.channels = mp3_channels;
  mp3_channel.layout = MIXED_ALTERNATING;
  mp3_channel.samplerate = mp3_samplerate;
  
  out_channel.data = buffer;
  out_channel.size = buffersize;
  out_channel.encoding = fmt123_to_mixed(out_encoding);
  out_channel.channels = out_channels;
  out_channel.layout = MIXED_ALTERNATING;
  out_channel.samplerate = out_samplerate;
  
  if(   !mixed_make_segment_source(&mp3_channel, &mp3_segment)
     || !mixed_make_segment_general(1.0, 0.9, &gen_segment)
     || !mixed_make_segment_drain(&out_channel, &out_segment)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }
  
  uint8_t mp3_samplesize = mixed_samplesize(mp3_channel.encoding);
  uint8_t out_samplesize = mixed_samplesize(out_channel.encoding);
  size_t samples_in_buffer = buffersize/((mp3_samplesize < out_samplesize)
                                         ? out_samplesize
                                         : mp3_samplesize);

  // Buffers are sized in samples.
  left.size = samples_in_buffer;
  right.size = samples_in_buffer;
  
  // Attach buffers to segments
  if(   !mixed_make_buffer(&left)
     || !mixed_make_buffer(&right)){
    printf("Failed to allocate mixer buffers: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(   !mixed_segment_set_buffer(0, &left, &mp3_segment)
     || !mixed_segment_set_buffer(1, &right, &mp3_segment)
     || !mixed_segment_set_buffer(0, &left, &gen_segment)
     || !mixed_segment_set_buffer(1, &right, &gen_segment)
     || !mixed_segment_set_buffer(2, &left, &gen_segment)
     || !mixed_segment_set_buffer(3, &right, &gen_segment)
     || !mixed_segment_set_buffer(0, &left, &out_segment)
     || !mixed_segment_set_buffer(1, &right, &out_segment)){
    printf("Failed to attach buffers to segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Assemble the mixer
  mixer.samplerate = out_samplerate;
  
  if(   !mixed_mixer_add(&mp3_segment, &mixer)
     || !mixed_mixer_add(&gen_segment, &mixer)
     || !mixed_mixer_add(&out_segment, &mixer)){
    printf("Failed to assemble mixer: %s\n", mixed_error_string(-1));
  }

  // Perform the mixing
  mixed_mixer_start(&mixer);

  size_t read = 0, played = 0;
  do{
    if(mpg123_read(mh, buffer, samples_in_buffer*mp3_samplesize, &read) != MPG123_OK){
      printf("Failure during MP3 decoding: %s\n", mpg123_strerror(mh));
      goto cleanup;
    }
    size_t samples_read = read/mp3_samplesize;
    if(!mixed_mixer_mix(samples_read, &mixer)){
      printf("Failure during mixing: %s\n", mixed_error_string(-1));
      goto cleanup;
    }
    played = out123_play(oh, buffer, samples_read*out_samplesize);
    if(played < read){
      printf("Warning: device not catching up with input (%i vs %i)\n", played, read);
    }
  }while(read);

  mixed_mixer_end(&mixer);
  
  exit = 0;
 cleanup:
  mixed_free_segment(&mp3_segment);
  mixed_free_segment(&gen_segment);
  mixed_free_segment(&out_segment);
  mixed_free_buffer(&left);
  mixed_free_buffer(&right);
  mixed_free_mixer(&mixer);
  
  if(oh){
    out123_close(oh);
    out123_del(oh);
  }
  
  if(mh){
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
  }

  if(buffer){
    free(buffer);
  }
  return exit;
}
