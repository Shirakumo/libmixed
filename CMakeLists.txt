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
  file(WRITE "VERSION" ${VERSION})
  message(STATUS "Rebuilt VERSION")
else()
  file(READ "VERSION" VERSION)
endif()
add_definitions( -DVERSION="${VERSION}" )

## Libsamplerate
file(GLOB samplerate_src "libsamplerate/*.c")
add_library(samplerate OBJECT ${samplerate_src})

## Main Library
include_directories("src/")
file(GLOB mixed_src "src/*.c" "src/segments/*.c")
add_library(mixed SHARED ${mixed_src} $<TARGET_OBJECTS:samplerate>)
string(REGEX MATCH "([0-9]+)\\.([0-9]+)-[0-9]+-(.+)" TMP ${VERSION})
set(MIXED_VERSION_MAJOR ${CMAKE_MATCH_1})
set(MIXED_VERSION_MINOR ${CMAKE_MATCH_2})
set(MIXED_VERSION_PATCH ${CMAKE_MATCH_3})
set(MIXED_VERSION_STRING ${MIXED_VERSION_MAJOR}.${MIXED_VERSION_MINOR}-${MIXED_VERSION_PATCH})
set_property(TARGET mixed PROPERTY VERSION ${MIXED_VERSION_STRING})
set_property(TARGET mixed PROPERTY SOVERSION ${MIXED_VERSION_MAJOR})
set_property(TARGET mixed PROPERTY C_STANDARD 99)
set_property(TARGET mixed PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(mixed PRIVATE MIXED_BUILD=1)
target_compile_options(mixed PRIVATE -mrdrnd)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
    OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(mixed PRIVATE -fvisibility=hidden)
endif()
if(WIN32)
  target_link_libraries(mixed m)
else()
  target_link_libraries(mixed dl m)
endif()

## Test Programs
find_package(mpg123)
find_package(out123)
if(OUT123_FOUND AND MPG123_FOUND)
  file(GLOB test_src "test/*.c")
  foreach(test_file ${test_src})
    get_filename_component(test_name ${test_file} NAME_WE)
    set(test_name "test_${test_name}")
    add_executable(${test_name} ${test_file})
    add_dependencies(${test_name} mixed)
    set_property(TARGET ${test_name} PROPERTY C_STANDARD 99)
    target_link_libraries(${test_name} mixed ${OUT123_LIBRARIES} ${MPG123_LIBRARIES})
  endforeach()
endif()

## Warnings
if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  add_definitions(/W2)
elseif(CMAKE_BUILD_TOOL MATCHES "(gcc|clang)")
  add_definitions(-W -Wall -Wextra -pedantic)
endif()
add_definitions(-Wno-ignored-attributes)

install(TARGETS mixed DESTINATION /usr/local/lib)
