include(CheckCSourceRuns)

function(check_flag var macroname flag)
  if(NOT DEFINED ${var})
    message(STATUS "Checking for ${macroname}")
    set(test_file "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/${macroname}.c")
    file(WRITE ${test_file} "
void main(){
#ifndef ${macroname}
#error \"No support\"
#endif
}")
    
    try_compile(${var} "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp" ${test_file}
      COMPILE_DEFINITIONS "-march=native")
    if(${var})
      message(STATUS "Checking for ${macroname} - found")
    endif()
  endif()
  if(${var})
    set(CPU_EXTENSION_FLAGS ${flag} ${CPU_EXTENSION_FLAGS} PARENT_SCOPE)
  endif()
endfunction()

set(CPU_EXTENSION_FLAGS "-march=native")
check_flag(HAS_SSE "__SSE__" "-msse")
check_flag(HAS_SSE2 "__SSE2__" "-msse2")
check_flag(HAS_SSE3 "__SSE3__" "-msse3")
check_flag(HAS_SSE4_1 "__SSE4_1__" "-msse4.1")
check_flag(HAS_SSE4_2 "__SSE4_2__" "-msse4.2")
check_flag(HAS_AVX "__AVX__" "-mavx")
check_flag(HAS_AVX2 "__AVX2__" "-mavx2")
