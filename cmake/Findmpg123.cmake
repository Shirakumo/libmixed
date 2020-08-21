find_path( MPG123_INCLUDE_DIR 
  NAMES
  mpg123.h
  HINTS
  "${MPG123_LOCATION}/include"
  "$ENV{MPG123_LOCATION}/include"
  PATHS
  "$ENV{PROGRAMFILES}/mpg123/include"
  "$ENV{PROGRAMFILES}/mpg123/include"
  /sw
  /usr
  /usr/local
  /opt
  /opt/local
  DOC 
  "The directory where mpg123/mpg123.h resides"
  )

find_library( MPG123_mpg123_LIBRARY 
  NAMES
  libmpg123
  mpg123
  HINTS
  "${MPG123_LOCATION}/lib"
  "${MPG123_LOCATION}/lib/x64"
  "$ENV{MPG123_LOCATION}/lib"
  PATHS
  /sw
  /usr
  /usr/local
  /opt
  /opt/local
  DOC 
  "The mpg123 library"
  )

set(MPG123_FOUND "NO")

if(MPG123_INCLUDE_DIR AND MPG123_mpg123_LIBRARY)
  set(MPG123_LIBRARIES "${MPG123_mpg123_LIBRARY}")
  set(MPG123_FOUND "YES")
  set(MPG123_LIBRARY "${MPG123_LIBRARIES}")
  set(MPG123_INCLUDE_PATH "${MPG123_INCLUDE_DIR}")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(mpg123
  REQUIRED_VARS
  MPG123_INCLUDE_DIR
  MPG123_LIBRARIES
  VERSION_VAR
  MPG123_VERSION)

mark_as_advanced(
  MPG123_INCLUDE_DIR
  MPG123_LIBRARIES
  MPG123_mpg123_LIBRARY)
