/*
$info$
tags: thunklibs|xcb-dri3
$end_info$
*/

#include <xcb/dri3.h>
#include <xcb/xcbext.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libxcb-dri3.inl"

extern "C" {
  xcb_extension_t xcb_dri3_id = {
    .name = "DRI3",
    .global_id = 0,
  };

  void FEX_malloc_free_on_host(void *Ptr) {
    struct {void *p;} args;
    args.p = Ptr;
    fexthunks_libxcb_dri3_FEX_free_on_host(&args);
  }

  size_t FEX_malloc_usable_size(void *Ptr) {
    struct {void *p; size_t rv;} args;
    args.p = Ptr;
    fexthunks_libxcb_dri3_FEX_usable_size(&args);
    return args.rv;
  }

  static void InitializeExtensions(xcb_connection_t *c) {
    FEX_xcb_dri3_init_extension(c, &xcb_dri3_id);
  }

  xcb_dri3_query_version_cookie_t xcb_dri3_query_version(xcb_connection_t * a_0,uint32_t a_1,uint32_t a_2){
    auto ret = fexfn_pack_xcb_dri3_query_version(a_0, a_1, a_2);
    InitializeExtensions(a_0);
    return ret;
  }

  xcb_dri3_query_version_cookie_t xcb_dri3_query_version_unchecked(xcb_connection_t * a_0,uint32_t a_1,uint32_t a_2){
    auto ret = fexfn_pack_xcb_dri3_query_version_unchecked(a_0, a_1, a_2);
    InitializeExtensions(a_0);
    return ret;
  }

  xcb_dri3_query_version_reply_t * xcb_dri3_query_version_reply(xcb_connection_t * a_0,xcb_dri3_query_version_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_query_version_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_dri3_open_reply_t * xcb_dri3_open_reply(xcb_connection_t * a_0,xcb_dri3_open_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_open_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_dri3_buffer_from_pixmap_reply_t * xcb_dri3_buffer_from_pixmap_reply(xcb_connection_t * a_0,xcb_dri3_buffer_from_pixmap_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_buffer_from_pixmap_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_dri3_fd_from_fence_reply_t * xcb_dri3_fd_from_fence_reply(xcb_connection_t * a_0,xcb_dri3_fd_from_fence_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_fd_from_fence_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_dri3_get_supported_modifiers_reply_t * xcb_dri3_get_supported_modifiers_reply(xcb_connection_t * a_0,xcb_dri3_get_supported_modifiers_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_get_supported_modifiers_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_dri3_buffers_from_pixmap_reply_t * xcb_dri3_buffers_from_pixmap_reply(xcb_connection_t * a_0,xcb_dri3_buffers_from_pixmap_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_dri3_buffers_from_pixmap_reply(a_0, a_1, a_2);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (ret) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(ret)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, ret, ResultSize);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }
}

LOAD_LIB(libxcb_dri3)
