/*
$info$
tags: thunklibs|drm
$end_info$
*/

#include <xf86drm.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

extern "C" {
  void FEX_malloc_free_on_host(void *Ptr) {
    struct {void *p;} args;
    args.p = Ptr;
    fexthunks_libdrm_FEX_free_on_host(&args);
  }

  size_t FEX_malloc_usable_size(void *Ptr) {
    struct {void *p; size_t rv;} args;
    args.p = Ptr;
    fexthunks_libdrm_FEX_usable_size(&args);
    return args.rv;
  }

  static void fexfn_pack_drmMsg(const char *format,...){
    va_list ap;
    if (1) {
      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);
    }
  }

  static char * fexfn_pack_drmGetDeviceNameFromFd(int a_0){
    struct {int a_0;char * rv;} args;
    args.a_0 = a_0;
    fexthunks_libdrm_drmGetDeviceNameFromFd(&args);

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv  = (char*)NewPtr;
    }

    return args.rv;
  }

  static char * fexfn_pack_drmGetDeviceNameFromFd2(int a_0){
    struct {int a_0;char * rv;} args;
    args.a_0 = a_0;
    fexthunks_libdrm_drmGetDeviceNameFromFd2(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv  = (char*)NewPtr;
    }

    return args.rv;
  }

  static char * fexfn_pack_drmGetPrimaryDeviceNameFromFd(int a_0){
    struct {int a_0;char * rv;} args;
    args.a_0 = a_0;
    fexthunks_libdrm_drmGetPrimaryDeviceNameFromFd(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv  = (char*)NewPtr;
    }

    return args.rv;
  }

  static char * fexfn_pack_drmGetRenderDeviceNameFromFd(int a_0){
    struct {int a_0;char * rv;} args;
    args.a_0 = a_0;
    fexthunks_libdrm_drmGetRenderDeviceNameFromFd(&args);

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv  = (char*)NewPtr;
    }

    return args.rv;
  }

  void drmMsg(const char *format,...) __attribute__((alias("fexfn_pack_drmMsg")));
  char * drmGetDeviceNameFromFd(int a_0) __attribute__((alias("fexfn_pack_drmGetDeviceNameFromFd")));
  char * drmGetDeviceNameFromFd2(int a_0) __attribute__((alias("fexfn_pack_drmGetDeviceNameFromFd2")));
  char * drmGetPrimaryDeviceNameFromFd(int a_0) __attribute__((alias("fexfn_pack_drmGetPrimaryDeviceNameFromFd")));
  char * drmGetRenderDeviceNameFromFd(int a_0) __attribute__((alias("fexfn_pack_drmGetRenderDeviceNameFromFd")));

}

LOAD_LIB(libdrm)
