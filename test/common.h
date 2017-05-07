#include <math.h>
#include <mpg123.h>
#include <out123.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

struct out{
  out123_handle *handle;
  struct mixed_channel channel;
  struct mixed_segment segment;
  struct mixed_buffer left;
  struct mixed_buffer right;
};

void free_out(struct out *out){
  if(!out) return;
  if(out->handle){
    out123_close(out->handle);
    out123_del(out->handle);
  }
  if(out->channel.data){
    free(out->channel.data);
  }
  mixed_free_segment(&out->segment);
  mixed_free_buffer(&out->left);
  mixed_free_buffer(&out->right);
  free(out);
}

int load_out_segment(size_t samples, struct out **_out){
  long out_samplerate = 44100;
  int out_channels = 2;
  int out_encoding = MPG123_ENC_SIGNED_16;
  char *out_encname = "signed 16 bit";
  uint8_t out_samplesize = 0;
  int out_framesize = 0;
  struct out *out = calloc(1, sizeof(struct out));

  if(!out){
    printf("Failed to allocate mixer data.\n");
    goto cleanup;
  }
  
  out->handle = out123_new();
  if(out->handle == 0){
    printf("Failed to create OUT123 handle.\n");
    goto cleanup;
  }

  if(out123_open(out->handle, 0, 0) != OUT123_OK){
    printf("Failed to open sound device: %s\n", out123_strerror(out->handle));
    goto cleanup;
  }

  if(out123_start(out->handle, out_samplerate, out_channels, MPG123_ENC_FLOAT_32)){
    printf("Failed to start playback on device: %s\n", out123_strerror(out->handle));
    goto cleanup;
  }
  
  if(out123_getformat(out->handle, &out_samplerate, &out_channels, &out_encoding, &out_framesize) != OUT123_OK){
    printf("Failed to get format properties of your device: %s\n", out123_strerror(out->handle));
    goto cleanup;
  }

  out_encname = (char *)out123_enc_longname(out_encoding);
  printf("OUT: %i channels @ %li Hz, %s\n", out_channels, out_samplerate, out_encname);
  
  // Prepare pipeline segments
  out->channel.encoding = fmt123_to_mixed(out_encoding);
  out->channel.channels = out_channels;
  out->channel.layout = MIXED_ALTERNATING;
  out->channel.samplerate = out_samplerate;
  out_samplesize = mixed_samplesize(out->channel.encoding);
  out->channel.data = calloc(samples*out_samplesize, sizeof(uint8_t));
  out->channel.size = samples*out_samplesize;

  if(!out->channel.data){
    printf("Couldn't allocate output buffer.\n");
    goto cleanup;
  }
  
  if(!mixed_make_segment_drain(&out->channel, &out->segment)){
    printf("Failed to create segments: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_make_buffer(samples, &out->left) ||
     !mixed_make_buffer(samples, &out->right)){
    printf("Failed to allocate mixer buffers: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &out->left, &out->segment) ||
     !mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &out->right, &out->segment)){
    printf("Failed to set buffers for out: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  // Make sure to play back some empty samples first.
  for(int i=0; i<out_samplerate; i+=samples){
    out123_play(out->handle, out->channel.data, out->channel.size);
  }

  *_out = out;
  return 1;

 cleanup:
  free_out(out);
  return 0;
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

  if(!mixed_make_buffer(samples, &mp3->left) ||
     !mixed_make_buffer(samples, &mp3->right)){
    printf("Failed to allocate mixer buffers: %s\n", mixed_error_string(-1));
    goto cleanup;
  }

  if(!mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &mp3->left, &mp3->segment) ||
     !mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &mp3->right, &mp3->segment)){
    printf("Failed to set buffers for %s: %s\n", file, mixed_error_string(-1));
    goto cleanup;
  }

  *_mp3 = mp3;
  return 1;

 cleanup:
  free_mp3(mp3);
  return 0;
}
