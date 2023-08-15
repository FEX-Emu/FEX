/*
$info$
tags: thunklibs|xcb
$end_info$
*/

#include <chrono>
#include <cstring>
#include <malloc.h>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/syscall.h>

#include <xcb/xcb.h>
#include <xcb/xcbext.h>

#include "common/Host.h"
#include <dlfcn.h>
#include <unordered_map>

#include "thunkgen_host_libxcb.inl"

static void fexfn_impl_libxcb_FEX_xcb_init_extension(xcb_connection_t*, xcb_extension_t*);
static size_t fexfn_impl_libxcb_FEX_usable_size(void*);
static void fexfn_impl_libxcb_FEX_free_on_host(void*);

static size_t fexfn_impl_libxcb_FEX_usable_size(void *a_0){
  return malloc_usable_size(a_0);
}

static void fexfn_impl_libxcb_FEX_free_on_host(void *a_0){
  free(a_0);
}

static void fexfn_impl_libxcb_FEX_xcb_init_extension(xcb_connection_t * a_0, xcb_extension_t * a_1){
  xcb_extension_t *ext{};

  if (strcmp(a_1->name, "BIG-REQUESTS") == 0) {
    ext = (xcb_extension_t *)dlsym(fexldr_ptr_libxcb_so, "xcb_big_requests_id");
  }
  else if (strcmp(a_1->name, "XC-MISC") == 0) {
    ext = (xcb_extension_t *)dlsym(fexldr_ptr_libxcb_so, "xcb_xc_misc_id");
  }
  else {
    fprintf(stderr, "Unknown xcb extension '%s'\n", a_1->name);
    __builtin_trap();
    return;
  }

  if (!ext) {
    fprintf(stderr, "Couldn't find extension symbol: '%s'\n", a_1->name);
    __builtin_trap();
    return;
  }
  [[maybe_unused]] auto res = fexldr_ptr_libxcb_xcb_get_extension_data(a_0, ext);

  // Copy over the global id
  a_1->global_id = ext->global_id;
}

EXPORTS(libxcb)
