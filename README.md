## About libmixed
Libmixed is a small library providing a generic audio mixing API and environment. It also ships a few useful audio processing tools to allow you to build an audio system for games, music, etc.

## How To
Libmixed will not do any audio file reading or audio playback for you. To do that, see other kinds of libraries like [mpg123](https://www.mpg123.de/) and its associated out123. There's a whole heap of audio backend and format libraries out there though and you should pick whichever you find suitable. Libmixed is independent to your choice.

There's a set of example applications in the [test](test/) directory that you can look at to see how a fully working application would be put together. For the purposes of this documentation, and for the sake of brevity, we're going to look at the process in a more disconnected manner.

The first step involves allocating space for a `struct mixed_mixer` and for a `struct mixed_segment` for each of the segments in the pipeline that you would like. For each of those segments you might also need a set of `struct mixed_buffer` instances to hold the audio data.

Once you have the space allocated, either on stack or on heap, you instantiate the segments with their respective `make` functions. For example, to create a source segment you would call `mixed_make_segment_source`.

When the segments have been instantiated successfully, the next step is to set their inputs and outputs using `mixed_segment_set_in` and `mixed_segment_set_out` respectively. Usually the field you will want to set for an in/out is the `MIXED_BUFFER`, where the value is a pointer to a `struct mixed_buffer`. You will have to connect a buffer to each of the inputs and outputs of each segment as they are required. Failure to connect a required buffer will lead to crashes.

Once all the buffers are connected, you will want to add the segments to the mixer using `mixed_mixer_add`. The order should follow the topological sorting of the directed acyclic graph described by the inputs and outputs of your segments.

After that, your mixer is fully assembled and ready to process audio. Before you enter the main mixing loop though, you will need to call `mixed_mixer_start` as some segments need some additional steps to prepare after their settings have been configured. Within the mixing loop you'll then want to cause your sources to read in the necessary audio data, then call `mixed_mixer_mix` to process it, and finally make your drains play back or consume the processed audio somehow. On exit from the mixing loop, you'll want to call `mixed_mixer_end` to finalise the segments and ready them for reconfiguration.

The [mixed header](src/mixed.h) should describe the public API and the available segments in sufficient detail for you to be able to figure out how to put together your own systems as desired.

In "short", omitting error checking, your usual procedure will look as follows:

```C
// Set up source and drain

struct mixed_segment source = {0}, drain = {0}, something = {0};
struct mixed_buffer left = {0}, right = {0};
struct mixed_mixer mixer = {0};

mixed_make_buffer(samples, &left);
mixed_make_buffer(samples, &right);

mixed_make_segment_source(&source_channel, &source);
mixed_make_segment_drain(&drain_channel, &drain);
mixed_make_segment_something(... , &something);

mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &left, &source);
mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &right, &source);
mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &left, &something);
mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &right, &something);
mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &left, &something);
mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &right, &something);
mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &left, &drain);
mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &right, &drain);

mixed_mixer_add(&source, &mixer);
mixed_mixer_add(&something, &mixer);
mixed_mixer_add(&drain, &mixer);

mixed_mixer_start(&mixer);
do{
  // Process source...
  
  mixed_mixer_mix(samples, &mixer);
  
  // Process drain...
}while(!ended)
mixed_mixer_end(&mixer);

mixed_free_mixer(&mixer);
mixed_free_segment(&source);
mixed_free_segment(&something);
mixed_free_segment(&drain);
mixed_free_buffer(&left);
mixed_free_buffer(&right);

// Clean up source and drain
```

## Compilation
In order to compile the library, you will need:

* CMake 3.1+
* A C99 compiler
* libdl and libm

In order to compile the additional test programs, you'll also need:

* libmpg123
* libout123 (usually shipped with libmpg123)

The procedure is as per usual for CMake-based builds:

* `mkdir build && cd build`
* `cmake ..`
* `make`

## Included Sources
* [ladspa.h](https://web.archive.org/web/20150627144551/http://www.ladspa.org:80/ladspa_sdk/ladspa.h.txt)
