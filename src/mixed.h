#ifndef __LIBMIXED_H__
#define __LIBMIXED_H__
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#  ifdef MIXED_STATIC_DEFINE
#    define MIXED_EXPORT
#  else
#    ifndef MIXED_EXPORT
#      define MIXED_EXPORT __declspec(dllexport)
#    endif
#  endif
#else
#  define MIXED_EXPORT
#endif
#include <stdint.h>
#include <stdlib.h>

  

#ifdef __cplusplus
}
#endif
