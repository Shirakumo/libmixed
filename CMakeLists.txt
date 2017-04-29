cmake_minimum_required(VERSION 3.1)
project(mixed)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

## Main Library
include_directories("src/")
file(GLOB lib_src "src/*.c" "src/segments/*.c")
add_library(mixed SHARED ${lib_src})
set_property(TARGET mixed PROPERTY C_STANDARD 99)
target_link_libraries(mixed libm.a)

## Test Program
find_package(mpg123)
if(OUT123_FOUND AND MPG123_FOUND)
  file(GLOB test_src "test/*.c")
  add_executable(mixed_test ${test_src})
  add_dependencies(mixed_test mixed)
  set_property(TARGET mixed_test PROPERTY C_STANDARD 99)
  target_link_libraries(mixed_test mixed)
endif()

## Warnings
if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  add_definitions(/W2)
elseif(CMAKE_BUILD_TOOL MATCHES "(gcc|clang)")
  add_definitions(-W -Wall -Wextra -pedantic)
endif()

install(TARGETS mixed DESTINATION /usr/local/lib)
