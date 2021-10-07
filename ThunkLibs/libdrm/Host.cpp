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
#include "function_unpacks.inl"

void fexfn_unpack_libdrm_FEX_usable_size(void *argsv){
  struct arg_t {void* a_0;size_t rv;};
  auto args = (arg_t*)argsv;
  args->rv = malloc_usable_size(args->a_0);
}

void fexfn_unpack_libdrm_FEX_free_on_host(void *argsv){
  struct arg_t {void* a_0;};
  auto args = (arg_t*)argsv;
  free(args->a_0);
}

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libdrm)

