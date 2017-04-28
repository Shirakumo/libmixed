find_path( OUT123_INCLUDE_DIR 
  NAMES
  out123/out123.h
  out123.h
  HINTS
  "${OUT123_LOCATION}/include"
  "$ENV{OUT123_LOCATION}/include"
  PATHS
  "$ENV{PROGRAMFILES}/out123/include"
  "$ENV{PROGRAMFILES}/mpg123/include"
  /usr/include
  /usr/local/include
  DOC 
  "The directory where out123/out123.h resides"
  )

find_library( OUT123_out123_LIBRARY 
  NAMES
  libout123
  out123
  HINTS
  "${OUT123_LOCATION}/lib"
  "${OUT123_LOCATION}/lib/x64"
  "$ENV{OUT123_LOCATION}/lib"
  PATHS
  /usr/lib
  /usr/local/lib
  DOC 
  "The out123 library"
  )

set(OUT123_FOUND "NO")

if(OUT123_INCLUDE_DIR AND OUT123_out123_LIBRARY)
  set(OUT123_LIBRARIES "${OUT123_out123_LIBRARY}")
  set(OUT123_FOUND "YES")
  set(OUT123_LIBRARY "${OUT123_LIBRARIES}")
  set(OUT123_INCLUDE_PATH "${OUT123_INCLUDE_DIR}")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(OUT123 
  REQUIRED_VARS
  OUT123_INCLUDE_DIR
  OUT123_LIBRARIES
  VERSION_VAR
  OUT123_VERSION)

mark_as_advanced(
  OUT123_INCLUDE_DIR
  OUT123_LIBRARIES
  OUT123_out123_LIBRARY)
