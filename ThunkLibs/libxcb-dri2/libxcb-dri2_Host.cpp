/*
$info$
tags: thunklibs|xcb-dri2
$end_info$
*/

#include <cstring>
#include <stdio.h>

#include <xcb/dri2.h>
#include <xcb/xcbext.h>

#include "common/Host.h"
#include <dlfcn.h>
#include <malloc.h>

#include "thunkgen_host_libxcb-dri2.inl"

static void fexfn_impl_libxcb_dri2_FEX_xcb_dri2_init_extension(xcb_connection_t * a_0, xcb_extension_t * a_1);

static size_t fexfn_impl_libxcb_dri2_FEX_usable_size(void *a_0){
  return malloc_usable_size(a_0);
}

static void fexfn_impl_libxcb_dri2_FEX_free_on_host(void *a_0){
  free(a_0);
}

static void fexfn_impl_libxcb_dri2_FEX_xcb_dri2_init_extension(xcb_connection_t * a_0, xcb_extension_t * a_1){
  xcb_extension_t *ext{};
  if (strcmp(a_1->name, "DRI2") == 0) {
    ext = (xcb_extension_t *)dlsym(fexldr_ptr_libxcb_dri2_so, "xcb_dri2_id");
  }
  else {
    fprintf(stderr, "Unknown xcb extension '%s'\n", a_1->name);
    __builtin_trap();
    return;
  }

  typedef const struct xcb_query_extension_reply_t * fexldr_type_libxcb_xcb_get_extension_data(xcb_connection_t * a_0,xcb_extension_t * a_1);
  fexldr_type_libxcb_xcb_get_extension_data *fexldr_ptr_libxcb_xcb_get_extension_data;

  fexldr_ptr_libxcb_xcb_get_extension_data = (fexldr_type_libxcb_xcb_get_extension_data*)dlsym(RTLD_DEFAULT, "xcb_get_extension_data");

  [[maybe_unused]] auto res = fexldr_ptr_libxcb_xcb_get_extension_data(a_0, ext);

  // Copy over the global id
  a_1->global_id = ext->global_id;
}

EXPORTS(libxcb_dri2)
