/*
$info$
tags: thunklibs|xcb-glx
$end_info$
*/

#include <xcb/glx.h>
#include <xcb/xcbext.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libxcb-glx.inl"

extern "C" {
xcb_extension_t xcb_glx_id = {
  .name = "GLX",
  .global_id = 0,
};

void FEX_malloc_free_on_host(void* Ptr) {
  struct {
    void* p;
  } args;
  args.p = Ptr;
  fexthunks_libxcb_glx_FEX_free_on_host(&args);
}

size_t FEX_malloc_usable_size(void* Ptr) {
  struct {
    void* p;
    size_t rv;
  } args;
  args.p = Ptr;
  fexthunks_libxcb_glx_FEX_usable_size(&args);
  return args.rv;
}

static void InitializeExtensions(xcb_connection_t* c) {
  FEX_xcb_glx_init_extension(c, &xcb_glx_id);
}

xcb_void_cookie_t xcb_glx_create_context_checked(xcb_connection_t* a_0, xcb_glx_context_t a_1, xcb_visualid_t a_2, uint32_t a_3,
                                                 xcb_glx_context_t a_4, uint8_t a_5) {
  auto ret = fexfn_pack_xcb_glx_create_context_checked(a_0, a_1, a_2, a_3, a_4, a_5);
  InitializeExtensions(a_0);
  return ret;
}

xcb_void_cookie_t
xcb_glx_create_context(xcb_connection_t* a_0, xcb_glx_context_t a_1, xcb_visualid_t a_2, uint32_t a_3, xcb_glx_context_t a_4, uint8_t a_5) {
  auto ret = fexfn_pack_xcb_glx_create_context(a_0, a_1, a_2, a_3, a_4, a_5);
  InitializeExtensions(a_0);
  return ret;
}

xcb_glx_make_current_reply_t* xcb_glx_make_current_reply(xcb_connection_t* a_0, xcb_glx_make_current_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_make_current_reply(a_0, a_1, a_2);

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

xcb_glx_is_direct_reply_t* xcb_glx_is_direct_reply(xcb_connection_t* a_0, xcb_glx_is_direct_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_is_direct_reply(a_0, a_1, a_2);

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

xcb_glx_query_version_reply_t* xcb_glx_query_version_reply(xcb_connection_t* a_0, xcb_glx_query_version_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_query_version_reply(a_0, a_1, a_2);

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
xcb_glx_get_visual_configs_reply_t*
xcb_glx_get_visual_configs_reply(xcb_connection_t* a_0, xcb_glx_get_visual_configs_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_visual_configs_reply(a_0, a_1, a_2);

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

xcb_glx_vendor_private_with_reply_reply_t*
xcb_glx_vendor_private_with_reply_reply(xcb_connection_t* a_0, xcb_glx_vendor_private_with_reply_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_vendor_private_with_reply_reply(a_0, a_1, a_2);

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

xcb_glx_query_extensions_string_reply_t*
xcb_glx_query_extensions_string_reply(xcb_connection_t* a_0, xcb_glx_query_extensions_string_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_query_extensions_string_reply(a_0, a_1, a_2);

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

xcb_glx_query_server_string_reply_t*
xcb_glx_query_server_string_reply(xcb_connection_t* a_0, xcb_glx_query_server_string_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_query_server_string_reply(a_0, a_1, a_2);

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

xcb_glx_get_fb_configs_reply_t* xcb_glx_get_fb_configs_reply(xcb_connection_t* a_0, xcb_glx_get_fb_configs_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_fb_configs_reply(a_0, a_1, a_2);

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

xcb_glx_query_context_reply_t* xcb_glx_query_context_reply(xcb_connection_t* a_0, xcb_glx_query_context_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_query_context_reply(a_0, a_1, a_2);

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

xcb_glx_make_context_current_reply_t*
xcb_glx_make_context_current_reply(xcb_connection_t* a_0, xcb_glx_make_context_current_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_make_context_current_reply(a_0, a_1, a_2);

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

xcb_glx_get_drawable_attributes_reply_t*
xcb_glx_get_drawable_attributes_reply(xcb_connection_t* a_0, xcb_glx_get_drawable_attributes_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_drawable_attributes_reply(a_0, a_1, a_2);

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

xcb_glx_gen_lists_reply_t* xcb_glx_gen_lists_reply(xcb_connection_t* a_0, xcb_glx_gen_lists_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_gen_lists_reply(a_0, a_1, a_2);

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

xcb_glx_render_mode_reply_t* xcb_glx_render_mode_reply(xcb_connection_t* a_0, xcb_glx_render_mode_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_render_mode_reply(a_0, a_1, a_2);

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

xcb_glx_finish_reply_t* xcb_glx_finish_reply(xcb_connection_t* a_0, xcb_glx_finish_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_finish_reply(a_0, a_1, a_2);

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

xcb_glx_read_pixels_reply_t* xcb_glx_read_pixels_reply(xcb_connection_t* a_0, xcb_glx_read_pixels_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_read_pixels_reply(a_0, a_1, a_2);

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

xcb_glx_get_booleanv_reply_t* xcb_glx_get_booleanv_reply(xcb_connection_t* a_0, xcb_glx_get_booleanv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_booleanv_reply(a_0, a_1, a_2);

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

xcb_glx_get_clip_plane_reply_t* xcb_glx_get_clip_plane_reply(xcb_connection_t* a_0, xcb_glx_get_clip_plane_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_clip_plane_reply(a_0, a_1, a_2);

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

xcb_glx_get_doublev_reply_t* xcb_glx_get_doublev_reply(xcb_connection_t* a_0, xcb_glx_get_doublev_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_doublev_reply(a_0, a_1, a_2);

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

xcb_glx_get_error_reply_t* xcb_glx_get_error_reply(xcb_connection_t* a_0, xcb_glx_get_error_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_error_reply(a_0, a_1, a_2);

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

xcb_glx_get_floatv_reply_t* xcb_glx_get_floatv_reply(xcb_connection_t* a_0, xcb_glx_get_floatv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_floatv_reply(a_0, a_1, a_2);

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

xcb_glx_get_integerv_reply_t* xcb_glx_get_integerv_reply(xcb_connection_t* a_0, xcb_glx_get_integerv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_integerv_reply(a_0, a_1, a_2);

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

xcb_glx_get_lightfv_reply_t* xcb_glx_get_lightfv_reply(xcb_connection_t* a_0, xcb_glx_get_lightfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_lightfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_lightiv_reply_t* xcb_glx_get_lightiv_reply(xcb_connection_t* a_0, xcb_glx_get_lightiv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_lightiv_reply(a_0, a_1, a_2);

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

xcb_glx_get_mapdv_reply_t* xcb_glx_get_mapdv_reply(xcb_connection_t* a_0, xcb_glx_get_mapdv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_mapdv_reply(a_0, a_1, a_2);

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

xcb_glx_get_mapfv_reply_t* xcb_glx_get_mapfv_reply(xcb_connection_t* a_0, xcb_glx_get_mapfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_mapfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_mapiv_reply_t* xcb_glx_get_mapiv_reply(xcb_connection_t* a_0, xcb_glx_get_mapiv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_mapiv_reply(a_0, a_1, a_2);

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

xcb_glx_get_materialfv_reply_t* xcb_glx_get_materialfv_reply(xcb_connection_t* a_0, xcb_glx_get_materialfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_materialfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_materialiv_reply_t* xcb_glx_get_materialiv_reply(xcb_connection_t* a_0, xcb_glx_get_materialiv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_materialiv_reply(a_0, a_1, a_2);

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

xcb_glx_get_pixel_mapfv_reply_t*
xcb_glx_get_pixel_mapfv_reply(xcb_connection_t* a_0, xcb_glx_get_pixel_mapfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_pixel_mapfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_pixel_mapuiv_reply_t*
xcb_glx_get_pixel_mapuiv_reply(xcb_connection_t* a_0, xcb_glx_get_pixel_mapuiv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_pixel_mapuiv_reply(a_0, a_1, a_2);

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

xcb_glx_get_pixel_mapusv_reply_t*
xcb_glx_get_pixel_mapusv_reply(xcb_connection_t* a_0, xcb_glx_get_pixel_mapusv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_pixel_mapusv_reply(a_0, a_1, a_2);

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

xcb_glx_get_polygon_stipple_reply_t*
xcb_glx_get_polygon_stipple_reply(xcb_connection_t* a_0, xcb_glx_get_polygon_stipple_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_polygon_stipple_reply(a_0, a_1, a_2);

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

xcb_glx_get_string_reply_t* xcb_glx_get_string_reply(xcb_connection_t* a_0, xcb_glx_get_string_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_string_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_envfv_reply_t* xcb_glx_get_tex_envfv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_envfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_envfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_enviv_reply_t* xcb_glx_get_tex_enviv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_enviv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_enviv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_gendv_reply_t* xcb_glx_get_tex_gendv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_gendv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_gendv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_genfv_reply_t* xcb_glx_get_tex_genfv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_genfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_genfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_geniv_reply_t* xcb_glx_get_tex_geniv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_geniv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_geniv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_image_reply_t* xcb_glx_get_tex_image_reply(xcb_connection_t* a_0, xcb_glx_get_tex_image_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_image_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_parameterfv_reply_t*
xcb_glx_get_tex_parameterfv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_parameteriv_reply_t*
xcb_glx_get_tex_parameteriv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_level_parameterfv_reply_t*
xcb_glx_get_tex_level_parameterfv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_level_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_level_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_tex_level_parameteriv_reply_t*
xcb_glx_get_tex_level_parameteriv_reply(xcb_connection_t* a_0, xcb_glx_get_tex_level_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_tex_level_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_is_enabled_reply_t* xcb_glx_is_enabled_reply(xcb_connection_t* a_0, xcb_glx_is_enabled_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_is_enabled_reply(a_0, a_1, a_2);

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

xcb_glx_is_list_reply_t* xcb_glx_is_list_reply(xcb_connection_t* a_0, xcb_glx_is_list_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_is_list_reply(a_0, a_1, a_2);

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

xcb_glx_are_textures_resident_reply_t*
xcb_glx_are_textures_resident_reply(xcb_connection_t* a_0, xcb_glx_are_textures_resident_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_are_textures_resident_reply(a_0, a_1, a_2);

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

xcb_glx_gen_textures_reply_t* xcb_glx_gen_textures_reply(xcb_connection_t* a_0, xcb_glx_gen_textures_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_gen_textures_reply(a_0, a_1, a_2);

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

xcb_glx_is_texture_reply_t* xcb_glx_is_texture_reply(xcb_connection_t* a_0, xcb_glx_is_texture_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_is_texture_reply(a_0, a_1, a_2);

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

xcb_glx_get_color_table_reply_t*
xcb_glx_get_color_table_reply(xcb_connection_t* a_0, xcb_glx_get_color_table_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_color_table_reply(a_0, a_1, a_2);

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

xcb_glx_get_color_table_parameterfv_reply_t* xcb_glx_get_color_table_parameterfv_reply(
  xcb_connection_t* a_0, xcb_glx_get_color_table_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_color_table_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_color_table_parameteriv_reply_t* xcb_glx_get_color_table_parameteriv_reply(
  xcb_connection_t* a_0, xcb_glx_get_color_table_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_color_table_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_get_convolution_filter_reply_t*
xcb_glx_get_convolution_filter_reply(xcb_connection_t* a_0, xcb_glx_get_convolution_filter_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_convolution_filter_reply(a_0, a_1, a_2);

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

xcb_glx_get_convolution_parameterfv_reply_t* xcb_glx_get_convolution_parameterfv_reply(
  xcb_connection_t* a_0, xcb_glx_get_convolution_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_convolution_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_convolution_parameteriv_reply_t* xcb_glx_get_convolution_parameteriv_reply(
  xcb_connection_t* a_0, xcb_glx_get_convolution_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_convolution_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_get_separable_filter_reply_t*
xcb_glx_get_separable_filter_reply(xcb_connection_t* a_0, xcb_glx_get_separable_filter_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_separable_filter_reply(a_0, a_1, a_2);

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

xcb_glx_get_histogram_reply_t* xcb_glx_get_histogram_reply(xcb_connection_t* a_0, xcb_glx_get_histogram_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_histogram_reply(a_0, a_1, a_2);

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

xcb_glx_get_histogram_parameterfv_reply_t*
xcb_glx_get_histogram_parameterfv_reply(xcb_connection_t* a_0, xcb_glx_get_histogram_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_histogram_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_histogram_parameteriv_reply_t*
xcb_glx_get_histogram_parameteriv_reply(xcb_connection_t* a_0, xcb_glx_get_histogram_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_histogram_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_get_minmax_reply_t* xcb_glx_get_minmax_reply(xcb_connection_t* a_0, xcb_glx_get_minmax_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_minmax_reply(a_0, a_1, a_2);

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

xcb_glx_get_minmax_parameterfv_reply_t*
xcb_glx_get_minmax_parameterfv_reply(xcb_connection_t* a_0, xcb_glx_get_minmax_parameterfv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_minmax_parameterfv_reply(a_0, a_1, a_2);

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

xcb_glx_get_minmax_parameteriv_reply_t*
xcb_glx_get_minmax_parameteriv_reply(xcb_connection_t* a_0, xcb_glx_get_minmax_parameteriv_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_minmax_parameteriv_reply(a_0, a_1, a_2);

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

xcb_glx_get_compressed_tex_image_arb_reply_t* xcb_glx_get_compressed_tex_image_arb_reply(
  xcb_connection_t* a_0, xcb_glx_get_compressed_tex_image_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_compressed_tex_image_arb_reply(a_0, a_1, a_2);

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

xcb_glx_gen_queries_arb_reply_t*
xcb_glx_gen_queries_arb_reply(xcb_connection_t* a_0, xcb_glx_gen_queries_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_gen_queries_arb_reply(a_0, a_1, a_2);

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

xcb_glx_is_query_arb_reply_t* xcb_glx_is_query_arb_reply(xcb_connection_t* a_0, xcb_glx_is_query_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_is_query_arb_reply(a_0, a_1, a_2);

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

xcb_glx_get_queryiv_arb_reply_t*
xcb_glx_get_queryiv_arb_reply(xcb_connection_t* a_0, xcb_glx_get_queryiv_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_queryiv_arb_reply(a_0, a_1, a_2);

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

xcb_glx_get_query_objectiv_arb_reply_t*
xcb_glx_get_query_objectiv_arb_reply(xcb_connection_t* a_0, xcb_glx_get_query_objectiv_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_query_objectiv_arb_reply(a_0, a_1, a_2);

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

xcb_glx_get_query_objectuiv_arb_reply_t*
xcb_glx_get_query_objectuiv_arb_reply(xcb_connection_t* a_0, xcb_glx_get_query_objectuiv_arb_cookie_t a_1, xcb_generic_error_t** a_2) {
  auto ret = fexfn_pack_xcb_glx_get_query_objectuiv_arb_reply(a_0, a_1, a_2);

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

LOAD_LIB(libxcb_glx)
