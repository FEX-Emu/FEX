/*
$info$
tags: thunklibs|drm
$end_info$
*/

#include <stdio.h>

#include <xf86drm.h>

#include "common/Host.h"
#include <dlfcn.h>
#include <malloc.h>

#include "thunkgen_host_libdrm.inl"

static size_t fexfn_impl_libdrm_FEX_usable_size(void* a_0) {
  return malloc_usable_size(a_0);
}

static void fexfn_impl_libdrm_FEX_free_on_host(void* a_0) {
  free(a_0);
}

EXPORTS(libdrm)
