cmake_minimum_required(VERSION 3.1)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
project(mixed C)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(BUILD_STATIC "Build the static library" ON)
option(BUILD_SHARED "Build the shared library" ON)
option(BUILD_EXAMPLES "Build the example applications" ON)
option(BUILD_TESTER "Build the tester application" ON)

## Generate version
find_program(GIT_SCM git DOC "Git version control")
mark_as_advanced(GIT_SCM)
find_file(GITDIR NAMES .git PATHS ${PROJECT_SOURCE_DIR} NO_DEFAULT_PATH)
if (GIT_SCM AND GITDIR)
  exec_program(
    "git" ${PROJECT_SOURCE_DIR} ARGS "describe" "--tags" "--long"
    OUTPUT_VARIABLE VERSION)
else()
  file(READ "VERSION" VERSION)
endif()
string(REGEX MATCH "([0-9]+)\\.([0-9]+)(-[0-9]+)?(-(.+))?" TMP ${VERSION})
set(MIXED_VERSION_MAJOR ${CMAKE_MATCH_1})
set(MIXED_VERSION_MINOR ${CMAKE_MATCH_2})
set(MIXED_VERSION_PATCH ${CMAKE_MATCH_5})
if(NOT MIXED_VERSION_PATCH)
  set(MIXED_VERSION_PATCH "0")
endif()
set(MIXED_VERSION_STRING ${MIXED_VERSION_MAJOR}.${MIXED_VERSION_MINOR}-${MIXED_VERSION_PATCH})
message(STATUS "Building ${MIXED_VERSION_MAJOR}.${MIXED_VERSION_MINOR}.${MIXED_VERSION_PATCH}")

## Optimize for target architecture
include(TargetArch)
target_architecture(ARCH)
if(ARCH MATCHES "x86_64" OR ARG MATCHES "i386")
  ## Minimum supported platform is SSE4.2
  message(STATUS "Enabling SSE4.2")
  set(CPU_EXTENSION_FLAGS "-msse4.2" "-mrdrnd")
endif()

set(COMPILATION_FLAGS -fvisibility=hidden -g ${CPU_EXTENSION_FLAGS})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(ARCH MATCHES "x86_64")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -static-libgcc -include ${CMAKE_SOURCE_DIR}/glibcver/glibc_2.9_x64.h)
  elseif(ARCH MATCHES "i386")
    set(COMPILATION_FLAGS ${COMPILATION_FLAGS} -static-libgcc -include ${CMAKE_SOURCE_DIR}/glibcver/glibc_2.9_x86.h)
  endif()
endif()

## Libsamplerate
add_library(samplerate OBJECT
  "libsamplerate/samplerate.c"
  "libsamplerate/src_linear.c"
  "libsamplerate/src_sinc.c"
  "libsamplerate/src_zoh.c")
target_compile_options(samplerate PRIVATE -O3 ${COMPILATION_FLAGS})
if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  target_compile_options(samplerate PRIVATE -fpic -flto)
endif()

## Spiralfft
if(ARCH MATCHES "x86_64")
  add_library(spiralfft OBJECT
    "spiralfft/sse/spiral_fft_float.c"
    "spiralfft/sse/spiral_private.c")
else()
  add_library(spiralfft OBJECT
    "spiralfft/scalar/spiral_fft_float.c"
    "spiralfft/scalar/spiral_private.c")
endif()
target_include_directories(spiralfft PRIVATE "spiralfft/")
target_compile_options(spiralfft PRIVATE -O3 ${COMPILATION_FLAGS})
if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  target_compile_options(spiralfft PRIVATE -fpic -flto)
endif()

## Libmixed
add_library(mixed OBJECT
  "src/buffer.c"
  "src/common.c"
  "src/encoding.c"
  "src/encoding.h"
  "src/hilbert.c"
  "src/internal.h"
  "src/ladspa.h"
  "src/lowpass.c"
  "src/mixed.h"
  "src/pack.c"
  "src/pitch.c"
  "src/plugin.c"
  "src/segment.c"
  "src/transfer.c"
  "src/vector.c"
  "src/segments/basic_mixer.c"
  "src/segments/chain.c"
  "src/segments/channel.c"
  "src/segments/delay.c"
  "src/segments/distribute.c"
  "src/segments/fade.c"
  "src/segments/frequency_pass.c"
  "src/segments/gate.c"
  "src/segments/generator.c"
  "src/segments/ladspa.c"
  "src/segments/noise.c"
  "src/segments/null.c"
  "src/segments/packer.c"
  "src/segments/pitch.c"
  "src/segments/quantize.c"
  "src/segments/queue.c"
  #"src/segments/repeat.c"
  "src/segments/space_mixer.c"
  "src/segments/speed_change.c"
  "src/segments/volume_control.c")
target_include_directories(mixed PRIVATE "src/" "libsamplerate/" "spiralfft/")
set_property(TARGET mixed PROPERTY C_STANDARD 99)
set_property(TARGET mixed PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(mixed PRIVATE MIXED_BUILD=1 MIXED_VERSION="${MIXED_VERSION_STRING}")
target_compile_options(mixed PRIVATE ${COMPILATION_FLAGS} -W -Wall -Wextra -Wpedantic -Wno-ignored-attributes -Wno-enum-conversion)

if(BUILD_STATIC)
  add_library(mixed_static STATIC $<TARGET_OBJECTS:samplerate> $<TARGET_OBJECTS:spiralfft> $<TARGET_OBJECTS:mixed>)
  set_target_properties(mixed_static PROPERTIES OUTPUT_NAME mixed)
  install(TARGETS mixed_static DESTINATION /usr/local/lib)
endif()
if(BUILD_SHARED)
  add_library(mixed_shared SHARED $<TARGET_OBJECTS:samplerate> $<TARGET_OBJECTS:spiralfft> $<TARGET_OBJECTS:mixed>)
  set_target_properties(mixed_shared PROPERTIES OUTPUT_NAME mixed)
  set_property(TARGET mixed_shared PROPERTY VERSION ${MIXED_VERSION_STRING})
  set_property(TARGET mixed_shared PROPERTY SOVERSION ${MIXED_VERSION_MAJOR})
  install(TARGETS mixed_shared DESTINATION /usr/local/lib)
  if(WIN32)
    target_link_libraries(mixed_shared m)
  else()
    target_link_libraries(mixed_shared dl m)
  endif()
endif()

## Tester
if(BUILD_TESTER)
  add_executable(tester
    "test/tester.h"
    "test/tester.c"
    "test/buffer.c"
    "test/encoding.c"
    "test/pack.c"
    "test/transfer.c"
    "test/packer.c"
    "test/distribute.c")
  add_dependencies(tester mixed_shared)
  set_property(TARGET tester PROPERTY C_STANDARD 99)
  target_compile_options(tester PRIVATE ${COMPILATION_FLAGS})
  target_link_libraries(tester mixed_shared pthread)

  add_custom_target(test
    COMMAND "${CMAKE_BINARY_DIR}/tester"
    DEPENDS tester)
endif()

## Example Programs
if(BUILD_EXAMPLES)
  find_package(mpg123)
  find_package(out123)
  find_package(Curses)
  if(OUT123_FOUND AND MPG123_FOUND AND CURSES_FOUND)
    file(GLOB example_src "examples/*.c")
    foreach(example_file ${example_src})
      get_filename_component(example_name ${example_file} NAME_WE)
      set(example_name "example_${example_name}")
      add_executable(${example_name} ${example_file})
      add_dependencies(${example_name} mixed_shared)
      set_property(TARGET ${example_name} PROPERTY C_STANDARD 99)
      target_include_directories(${example_name} PRIVATE ${CURSES_INCLUDE_DIRS} ${MPG123_INCLUDE_DIR} ${OUT123_INCLUDE_DIR})
      target_link_libraries(${example_name} mixed_shared ${CURSES_LIBRARIES} ${OUT123_LIBRARIES} ${MPG123_LIBRARIES})
    endforeach()
  else()
    message(STATUS "Cannot build examples")
  endif()
endif()
