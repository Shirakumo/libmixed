cmake_minimum_required(VERSION 3.1)
project(mixed)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

## Generate git header
find_program(GIT_SCM git DOC "Git version control")
mark_as_advanced(GIT_SCM)
find_file(GITDIR NAMES .git PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
if (GIT_SCM AND GITDIR)
  exec_program(
    "git" ${CMAKE_CURRENT_SOURCE_DIR} ARGS "describe" "--tags" "--long"
    OUTPUT_VARIABLE VERSION )
else()
  file(READ "VERSION" VERSION)
endif()

## Libsamplerate
file(GLOB samplerate_src "libsamplerate/*.c")
add_library(samplerate OBJECT ${samplerate_src})
target_compile_options(samplerate PRIVATE -O3 -mavx2 -mfma)

## Spiralfft
file(GLOB spiralfft_src "spiralfft/*.c")
add_library(spiralfft STATIC
  ${spiralfft_src})
target_compile_options(spiralfft PRIVATE -O3 -mavx2 -mfma)

## Main Library
include_directories("src/" "spiralfft/")
add_library(mixed SHARED
  "src/buffer.c"
  "src/common.c"
  "src/encoding.c"
  "src/encoding.h"
  "src/internal.h"
  "src/ladspa.h"
  "src/mixed.h"
  "src/pitch.c"
  "src/segment.c"
  "src/segment_sequence.c"
  "src/transfer.c"
  "src/vector.c"
  "src/segments/basic_mixer.c"
  "src/segments/delay.c"
  "src/segments/fade.c"
  "src/segments/frequency_pass.c"
  "src/segments/gate.c"
  "src/segments/generator.c"
  "src/segments/ladspa.c"
  "src/segments/noise.c"
  "src/segments/packer.c"
  "src/segments/pitch.c"
  "src/segments/queue.c"
  #"src/segments/repeat.c"
  "src/segments/space_mixer.c"
  "src/segments/volume_control.c"
  $<TARGET_OBJECTS:samplerate>
  $<TARGET_OBJECTS:spiralfft>)
string(REGEX MATCH "([0-9]+)\\.([0-9]+)-[0-9]+-(.+)" TMP ${VERSION})
set(MIXED_VERSION_MAJOR ${CMAKE_MATCH_1})
set(MIXED_VERSION_MINOR ${CMAKE_MATCH_2})
set(MIXED_VERSION_PATCH ${CMAKE_MATCH_3})
set(MIXED_VERSION_STRING ${MIXED_VERSION_MAJOR}.${MIXED_VERSION_MINOR}-${MIXED_VERSION_PATCH})
set_property(TARGET mixed PROPERTY VERSION ${MIXED_VERSION_STRING})
set_property(TARGET mixed PROPERTY SOVERSION ${MIXED_VERSION_MAJOR})
set_property(TARGET mixed PROPERTY C_STANDARD 99)
set_property(TARGET mixed PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(mixed PRIVATE MIXED_BUILD=1 MIXED_VERSION="${MIXED_VERSION_STRING}")
target_compile_options(mixed PRIVATE -mrdrnd -mavx2 -mfma -W -Wall -Wextra -Wpedantic -Wno-ignored-attributes)
target_link_options(mixed PRIVATE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(spiralfft PRIVATE -fvisibility=hidden)
  target_compile_options(mixed PRIVATE -fvisibility=hidden)
endif()
if(WIN32)
  target_link_libraries(mixed m)
else()
  target_link_libraries(mixed dl m)
endif()

## Tester
add_executable(tester
  "test/tester.h"
  "test/tester.c"
  "test/buffer.c"
  "test/transfer.c"
  "test/packer.c")
add_dependencies(tester mixed)
set_property(TARGET tester PROPERTY C_STANDARD 99)
target_link_libraries(tester mixed)

## Example Programs
find_package(mpg123)
find_package(out123)
if(OUT123_FOUND AND MPG123_FOUND)
  file(GLOB example_src "examples/*.c")
  foreach(example_file ${example_src})
    get_filename_component(example_name ${example_file} NAME_WE)
    set(example_name "example_${example_name}")
    add_executable(${example_name} ${example_file})
    add_dependencies(${example_name} mixed)
    set_property(TARGET ${example_name} PROPERTY C_STANDARD 99)
    target_link_libraries(${example_name} mixed ${OUT123_LIBRARIES} ${MPG123_LIBRARIES})
  endforeach()
endif()

install(TARGETS mixed DESTINATION /usr/local/lib)

add_custom_target(test
  COMMAND "${CMAKE_BINARY_DIR}/tester"
  DEPENDS tester)
  
