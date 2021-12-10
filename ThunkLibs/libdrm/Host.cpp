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

#include "ldr_ptrs.inl"

static size_t fexfn_impl_libdrm_FEX_usable_size(void *a_0){
  return malloc_usable_size(a_0);
}

static void fexfn_impl_libdrm_FEX_free_on_host(void *a_0){
  free(a_0);
}

#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libdrm)

