/*
$info$
tags: thunklibs|xcb-xfixes
$end_info$
*/

#include <xcb/xfixes.h>
#include <xcb/xcbext.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libxcb-xfixes.inl"

extern "C" {
xcb_extension_t xcb_xfixes_id = {
  .name = "XFIXES",
  .global_id = 0,
};

void FEX_malloc_free_on_host(void* Ptr) {
  struct {
    void* p;
  } args;
  args.p = Ptr;
  fexthunks_libxcb_xfixes_FEX_free_on_host(&args);
}

size_t FEX_malloc_usable_size(void* Ptr) {
  struct {
    void* p;
    size_t rv;
  } args;
  args.p = Ptr;
  fexthunks_libxcb_xfixes_FEX_usable_size(&args);
  return args.rv;
}

static void InitializeExtensions(xcb_connection_t* c) {
  FEX_xcb_xfixes_init_extension(c, &xcb_xfixes_id);
}

xcb_xfixes_query_version_cookie_t xcb_xfixes_query_version(xcb_connection_t* a_0, uint32_t a_1, uint32_t a_2) {
  auto ret = fexfn_pack_xcb_xfixes_query_version(a_0, a_1, a_2);
  InitializeExtensions(a_0);
  return ret;
}

xcb_xfixes_query_version_cookie_t xcb_xfixes_query_version_unchecked(xcb_connection_t* a_0, uint32_t a_1, uint32_t a_2) {
  auto ret = fexfn_pack_xcb_xfixes_query_version_unchecked(a_0, a_1, a_2);
  InitializeExtensions(a_0);
  return ret;
}

xcb_xfixes_query_version_reply_t*
xcb_xfixes_query_version_reply(xcb_connection_t* a_0, xcb_xfixes_query_version_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_xfixes_query_version_reply(a_0, a_1, a_2);

  // We now need to do some fixups here
  if (a_2 && *a_2) {
    // If the error code pointer exists then we need to copy the contents and free the host facing pointer
    xcb_generic_error_t* NewError = (xcb_generic_error_t*)malloc(sizeof(xcb_generic_error_t));
    memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
    FEX_malloc_free_on_host(*a_2);

    // User is expected to free this
    *a_2 = NewError;
  }

  if (ret) {
    constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
    void* NewPtr = malloc(ResultSize);
    memcpy(NewPtr, ret, ResultSize);

    FEX_malloc_free_on_host(ret);
    ret = (decltype(ret))NewPtr;
  }

  return ret;
}

xcb_xfixes_get_cursor_image_reply_t*
xcb_xfixes_get_cursor_image_reply(xcb_connection_t* a_0, xcb_xfixes_get_cursor_image_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_xfixes_get_cursor_image_reply(a_0, a_1, a_2);

  // We now need to do some fixups here
  if (a_2 && *a_2) {
    // If the error code pointer exists then we need to copy the contents and free the host facing pointer
    xcb_generic_error_t* NewError = (xcb_generic_error_t*)malloc(sizeof(xcb_generic_error_t));
    memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
    FEX_malloc_free_on_host(*a_2);

    // User is expected to free this
    *a_2 = NewError;
  }

  if (ret) {
    constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
    void* NewPtr = malloc(ResultSize);
    memcpy(NewPtr, ret, ResultSize);

    FEX_malloc_free_on_host(ret);
    ret = (decltype(ret))NewPtr;
  }

  return ret;
}

xcb_xfixes_fetch_region_reply_t*
xcb_xfixes_fetch_region_reply(xcb_connection_t* a_0, xcb_xfixes_fetch_region_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_xfixes_fetch_region_reply(a_0, a_1, a_2);

  // We now need to do some fixups here
  if (a_2 && *a_2) {
    // If the error code pointer exists then we need to copy the contents and free the host facing pointer
    xcb_generic_error_t* NewError = (xcb_generic_error_t*)malloc(sizeof(xcb_generic_error_t));
    memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
    FEX_malloc_free_on_host(*a_2);

    // User is expected to free this
    *a_2 = NewError;
  }

  if (ret) {
    constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
    void* NewPtr = malloc(ResultSize);
    memcpy(NewPtr, ret, ResultSize);

    FEX_malloc_free_on_host(ret);
    ret = (decltype(ret))NewPtr;
  }

  return ret;
}

xcb_xfixes_get_cursor_name_reply_t*
xcb_xfixes_get_cursor_name_reply(xcb_connection_t* a_0, xcb_xfixes_get_cursor_name_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_xfixes_get_cursor_name_reply(a_0, a_1, a_2);

  // We now need to do some fixups here
  if (a_2 && *a_2) {
    // If the error code pointer exists then we need to copy the contents and free the host facing pointer
    xcb_generic_error_t* NewError = (xcb_generic_error_t*)malloc(sizeof(xcb_generic_error_t));
    memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
    FEX_malloc_free_on_host(*a_2);

    // User is expected to free this
    *a_2 = NewError;
  }

  if (ret) {
    constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
    void* NewPtr = malloc(ResultSize);
    memcpy(NewPtr, ret, ResultSize);

    FEX_malloc_free_on_host(ret);
    ret = (decltype(ret))NewPtr;
  }

  return ret;
}

xcb_xfixes_get_cursor_image_and_name_reply_t* xcb_xfixes_get_cursor_image_and_name_reply(
  xcb_connection_t* a_0, xcb_xfixes_get_cursor_image_and_name_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_xfixes_get_cursor_image_and_name_reply(a_0, a_1, a_2);

  // We now need to do some fixups here
  if (a_2 && *a_2) {
    // If the error code pointer exists then we need to copy the contents and free the host facing pointer
    xcb_generic_error_t* NewError = (xcb_generic_error_t*)malloc(sizeof(xcb_generic_error_t));
    memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
    FEX_malloc_free_on_host(*a_2);

    // User is expected to free this
    *a_2 = NewError;
  }

  if (ret) {
    constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
    void* NewPtr = malloc(ResultSize);
    memcpy(NewPtr, ret, ResultSize);

    FEX_malloc_free_on_host(ret);
    ret = (decltype(ret))NewPtr;
  }

  return ret;
}
}

LOAD_LIB(libxcb_xfixes)
