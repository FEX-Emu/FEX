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

#include "thunkgen_guest_libdrm.inl"

extern "C" {
void FEX_malloc_free_on_host(void* Ptr) {
  struct {
    void* p;
  } args;
  args.p = Ptr;
  fexthunks_libdrm_FEX_free_on_host(&args);
}

size_t FEX_malloc_usable_size(void* Ptr) {
  struct {
    void* p;
    size_t rv;
  } args;
  args.p = Ptr;
  fexthunks_libdrm_FEX_usable_size(&args);
  return args.rv;
}

void drmMsg(const char* format, ...) {
  va_list ap;
  if (1) {
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
  }
}

char* drmGetDeviceNameFromFd(int a_0) {
  auto ret = fexfn_pack_drmGetDeviceNameFromFd(a_0);

  if (ret) {
    // Usable size
    size_t Usable = FEX_malloc_usable_size(ret);

    // This will be a bit wasteful but this is an unsized pointer
    void* NewPtr = malloc(Usable);
    memcpy(NewPtr, ret, Usable);

    FEX_malloc_free_on_host(ret);
    ret = (char*)NewPtr;
  }

  return ret;
}

char* drmGetDeviceNameFromFd2(int a_0) {
  auto ret = fexfn_pack_drmGetDeviceNameFromFd2(a_0);
  if (ret) {
    // Usable size
    size_t Usable = FEX_malloc_usable_size(ret);

    // This will be a bit wasteful but this is an unsized pointer
    void* NewPtr = malloc(Usable);
    memcpy(NewPtr, ret, Usable);

    FEX_malloc_free_on_host(ret);
    ret = (char*)NewPtr;
  }

  return ret;
}

char* drmGetPrimaryDeviceNameFromFd(int a_0) {
  auto ret = fexfn_pack_drmGetPrimaryDeviceNameFromFd(a_0);
  if (ret) {
    // Usable size
    size_t Usable = FEX_malloc_usable_size(ret);

    // This will be a bit wasteful but this is an unsized pointer
    void* NewPtr = malloc(Usable);
    memcpy(NewPtr, ret, Usable);

    FEX_malloc_free_on_host(ret);
    ret = (char*)NewPtr;
  }

  return ret;
}

char* drmGetRenderDeviceNameFromFd(int a_0) {
  auto ret = fexfn_pack_drmGetRenderDeviceNameFromFd(a_0);

  if (ret) {
    // Usable size
    size_t Usable = FEX_malloc_usable_size(ret);

    // This will be a bit wasteful but this is an unsized pointer
    void* NewPtr = malloc(Usable);
    memcpy(NewPtr, ret, Usable);

    FEX_malloc_free_on_host(ret);
    ret = (char*)NewPtr;
  }

  return ret;
}
}

LOAD_LIB(libdrm)
