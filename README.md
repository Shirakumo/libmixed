## About libmixed
Libmixed is a library for real-time audio processing pipelines for use in audio/video/games. It can serve as a base architecture for complex DSP systems.

## How To
Libmixed will not do any audio file reading or audio playback for you. To do that, see other kinds of libraries like [mpg123](https://www.mpg123.de/) and its associated out123. There's a whole heap of audio backend and format libraries out there though and you should pick whichever you find suitable. Libmixed is independent to your choice.

There's a set of example applications in the [examples](examples/) directory that you can look at to see how a fully working application would be put together. For the purposes of this documentation, and for the sake of brevity, we're going to look at the process in a more disconnected manner.

The first step involves allocating space for a `struct mixed_segment` for each of the segments in the pipeline that you would like. For each of those segments you might also need a set of `struct mixed_buffer` instances to hold the audio data.

Once you have the space allocated, either on stack or on heap, you instantiate the segments with their respective `make` functions. For example, to create an unpack segment you would call `mixed_make_segment_unpacker()`.

When the segments have been instantiated successfully, the next step is to set their inputs and outputs using `mixed_segment_set_in()` and `mixed_segment_set_out()` respectively. Usually the field you will want to set for an in/out is the `MIXED_BUFFER`, where the value is a pointer to a `struct mixed_buffer`. You will have to connect a buffer to each of the inputs and outputs of each segment as they are required. Failure to connect a required buffer will lead to crashes.

Once all the buffers are connected, you will want to add them to a chain to manage them easily. A chain is created using `mixed_make_segment_chain()`, and you can add segments using `mixed_chain_add()`.

After that, your system is fully assembled and ready to process audio. Before you enter the main mixing loop though, you will need to call `mixed_segment_start()` on the chain, as some segments need some additional steps to prepare after their settings have been configured. Within the mixing loop you'll then want to cause your sources to read in the necessary audio data, then call `mixed_segment_mix()` to process the chain, and finally make your drains play back or consume the processed audio somehow. On exit from the mixing loop, you'll want to call `mixed_segment_end()` to finalise the segments and ready them for reconfiguration.

The [mixed header](@ref src/mixed.h) should describe the public API and the available segments in sufficient detail for you to be able to figure out how to put together your own systems as desired.

In "short", omitting error checking, your usual procedure will look as follows:

```C
// Set up source and drain

struct mixed_segment chain = {0}, source = {0}, drain = {0}, something = {0};
struct mixed_buffer left = {0}, right = {0};

mixed_make_buffer(samples, &left);
mixed_make_buffer(samples, &right);

mixed_make_segment_unpacker(&source_audio, &source);
mixed_make_segment_packer(&drain_audio, &drain);
mixed_make_segment_something(... , &something);

mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &left, &source);
mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &right, &source);
mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &left, &something);
mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &right, &something);
mixed_segment_set_out(MIXED_BUFFER, MIXED_LEFT, &left, &something);
mixed_segment_set_out(MIXED_BUFFER, MIXED_RIGHT, &right, &something);
mixed_segment_set_in(MIXED_BUFFER, MIXED_LEFT, &left, &drain);
mixed_segment_set_in(MIXED_BUFFER, MIXED_RIGHT, &right, &drain);

mixed_chain_add(&source, &chain);
mixed_chain_add(&something, &chain);
mixed_chain_add(&drain, &chain);

mixed_segment_start(&chain);
do{
  // Process source...
  
  mixed_segment_mix(samples, &chain);
  
  // Process drain...
}while(!ended)
mixed_segment_end(&chain);

mixed_free_segment(&chain);
mixed_free_segment(&source);
mixed_free_segment(&something);
mixed_free_segment(&drain);
mixed_free_buffer(&left);
mixed_free_buffer(&right);

// Clean up source and drain
```

## Provided Segments
Libmixed provides the following segments out of the box:

- `basic_mixer`  
  Basic linear mixer, adds several input sources together.
- `biquad_filter`  
  A biquad filter to allow for effects like lowpass, highpass, bandpass, etc.
- `chain`  
  A segment that processes other segments in sequence. Used for composing.
- `channel_convert`  
  Upmix or downmix channels. Supports the following conversions:
  - 1.0 -> 2.0
  - 2.0 -> 1.0
  - 2.0 -> 3.0
  - 2.0 -> 4.0
  - 2.0 -> 5.0
  - 2.0 -> 5.1 
  - 2.0 -> 7.1
- `compressor`  
  Compression filter to limit volume.
- `convolution`  
  Finite Input Response Convolution, typically used for reverb.
- `delay`  
  A single-channel delay line.
- `distribute`  
  Allows distributing an output buffer to multiple inputs at once. Useful when a signal needs to be processed in multiple ways at once.
- `equalizer`  
  An 8-band frequency equalizer.
- `fade`  
  Allows fading an input in and out over time.
- `fft`  
  Performs a forward or inverse Fast Fourier Transform. Useful for more manual sound processing or visualisation.
- `gate`  
  A basic noise gate.
- `generator`  
  Generates simple sound waves, either sine, square, sawtooth, or triangle waves.
- `ladspa`  
  A conversion plugin to load LADSPA-API effects into libmixed.
- `noise`  
  Generates pink, brown, or white noise.
- `void`  
  Consumes audio without processing it.
- `zero`
  Produces silence.
- `packer`  
  Packs audio from several buffers into an interleaved and packed audio stream.
- `unpacker`  
  Unpacks audio from an interleaved and packed audio stream to several buffers.
- `pitch`  
  Changes the pitch of the signal without affecting its speed.
- `plane_mixer`  
  Performs 2D, "planar" audio mixing, allowing for effects placement in 2D with attenuation and optional doppler.
- `quantize`  
  Quantizes the signal, useful for creating distorted, lo-fi audio.
- `queue`  
  Processes a single segment in a sequence until it stops producing output.
- `repeat`  
  Captures and replays an input signal.
- `space_mixer`  
  Performs 3D, "spatial" audio mixing, allowing for effects placement in 3D with attenuation and optional doppler.
- `speed_change`  
  Changes the playback speed of the incoming audio.
- `volume_control`  
  Adjusts the volume and pan of the incoming audio.

## Segment Plugin Architecture
Libmixed offers a standardised architecture for audio processing units, called 'segments'. Each segment consists of a structure with pointers to functions that should execute the segment's respective actions. These structures can be constructed arbitrarily, allowing integration from non-C languages. Each segment also supports full reflection of its inputs, outputs, and properties, allowing the construction of generic user interfaces that can manage segments without needing special casing for each individual one.

Information about a segment can be obtained through `mixed_segment_info()`, which will provide you with an array of `struct mixed_segment_info`, containing information about the segment itself (`name`, `description`), its restrictions (`flags`), the number of its inputs and outputs (`min_inputs`, `max_inputs`, `outputs`), and the supported fields that can be set or retrieved (`fields`).

A list of all known segments can be obtained via `mixed_list_segments()`. For each registered segment, information about its constructor arguments can then be obtained via `mixed_make_segment_info()`, and ultimately used to dynamically initialise a segment with `mixed_make_segment()`.

This allows adding and removing known segments at runtime, and provides an API that, again, allows automatically constructing user interfaces that could initialise arbitrary segments.

libmixed also includes an API to provide segment plugin libraries. Such plugin libraries can be loaded (`mixed_load_plugin()`) and unloaded (`mixed_close_plugin()`) dynamically. In order to create such a plugin library, the user must simply dynamically link against libmixed and provide two exported functions, `mixed_make_plugin()`, and `mixed_free_plugin()`, which will automatically be called when the plugin is loaded or unloaded.

The segment de/registration function is provided as an argument to allow third-party systems to provide integration into higher-level registration mechanisms without having to query libmixed after the fact.

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

On Windows you will usually want to build with MSYS2 and the following cmake command:

* `cmake .. -G "MSYS Makefiles"`

By default it will compile for SSE4.2 on x86 systems, with dispatchers for higher vectorisation APIs like AVX and AVX2. This allows libmixed to stay compatible with older systems while still being able to utilise the capabilities of more modern ones.

## Included Sources
* [ladspa.h](https://web.archive.org/web/20150627144551/http://www.ladspa.org:80/ladspa_sdk/ladspa.h.txt)
* [libsamplerate](http://www.mega-nerd.com/SRC/index.html) Please note that the BSD 2-Clause license restrictions also apply to libmixed, as it includes libsamplerate internally.
* [spiralfft](http://spiral.net/codegenerator.html) Please note that the BSD-style [Spiral License](http://www.spiral.net/doc/LICENSE) also applies to libmixed, as it includes spiralfft internally.
