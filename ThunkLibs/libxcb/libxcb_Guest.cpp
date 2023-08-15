/*
$info$
tags: thunklibs|xcb
$end_info$
*/

#include <xcb/xcb.h>
#include <xcb/xcbext.h>

#include <atomic>
#include <condition_variable>
#include <stdio.h>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "common/Guest.h"

#include <stdarg.h>

#include "thunkgen_guest_libxcb.inl"

extern "C" {
  xcb_extension_t xcb_big_requests_id = {
    .name = "BIG-REQUESTS",
    .global_id = 0,
  };

  xcb_extension_t xcb_xc_misc_id = {
    .name = "XC-MISC",
    .global_id = 0,
  };

  static void InitializeExtensions(xcb_connection_t *c) {
    FEX_xcb_init_extension(c, &xcb_big_requests_id);
    FEX_xcb_init_extension(c, &xcb_xc_misc_id);
  }

  void FEX_malloc_free_on_host(void *Ptr) {
    struct {void *p;} args;
    args.p = Ptr;
    fexthunks_libxcb_FEX_free_on_host(&args);
  }

  size_t FEX_malloc_usable_size(void *Ptr) {
    struct {void *p; size_t rv;} args;
    args.p = Ptr;
    fexthunks_libxcb_FEX_usable_size(&args);
    return args.rv;
  }

  xcb_connection_t * xcb_connect_to_fd(int a_0, xcb_auth_info_t * a_1) {
    auto ret = fexfn_pack_xcb_connect_to_fd(a_0, a_1);

    if (xcb_get_file_descriptor(ret) != -1) {
      // Only create callback on valid xcb connections.
      // Checking for FD is the easiest way to do this.
      //CreateCallback();
    }
    InitializeExtensions(ret);
    return ret;
  }

  xcb_connection_t * xcb_connect(const char * a_0,int * a_1){
    auto ret = fexfn_pack_xcb_connect(a_0, a_1);
    InitializeExtensions(ret);
    return ret;
  }

  xcb_connection_t * xcb_connect_to_display_with_auth_info(const char * a_0,xcb_auth_info_t * a_1,int * a_2){
    auto ret = fexfn_pack_xcb_connect_to_display_with_auth_info(a_0, a_1, a_2);
    InitializeExtensions(ret);
    return ret;
  }

  void xcb_disconnect(xcb_connection_t * a_0){
    fexfn_pack_xcb_disconnect(a_0);
  }

  int xcb_parse_display(const char * a_0,char ** a_1,int * a_2,int * a_3){
    auto ret = fexfn_pack_xcb_parse_display(a_0, a_1, a_2, a_3);
    if (a_1 && *a_1) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(*a_1);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, *a_1, Usable);

      FEX_malloc_free_on_host(*a_1);
      *a_1 = (char*)NewPtr;
    }

    return ret;
  }

  xcb_generic_event_t * xcb_wait_for_event(xcb_connection_t * a_0){
    auto ret = fexfn_pack_xcb_wait_for_event(a_0);

    if (ret) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_generic_event_t * xcb_poll_for_event(xcb_connection_t * a_0){
    auto ret = fexfn_pack_xcb_poll_for_event(a_0);
    if (ret) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_generic_event_t * xcb_poll_for_queued_event(xcb_connection_t * a_0){
    auto ret = fexfn_pack_xcb_poll_for_queued_event(a_0);
    if (ret) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_generic_event_t * xcb_poll_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1){
    auto ret = fexfn_pack_xcb_poll_for_special_event(a_0, a_1);
    if (ret) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_generic_event_t * xcb_wait_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1){
    auto ret = fexfn_pack_xcb_wait_for_special_event(a_0, a_1);
    if (ret) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_generic_error_t * xcb_request_check(xcb_connection_t * a_0,xcb_void_cookie_t a_1){
    auto ret = fexfn_pack_xcb_request_check(a_0, a_1);
    if (ret) {
      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(sizeof(xcb_generic_error_t));
      memcpy(NewPtr, ret, sizeof(xcb_generic_error_t));

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }
    return ret;
  }

  void * xcb_wait_for_reply(xcb_connection_t * a_0, uint32_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_wait_for_reply(a_0, a_1, a_2);

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
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  void * xcb_wait_for_reply64(xcb_connection_t * a_0,uint64_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_wait_for_reply64(a_0, a_1, a_2);

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
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = NewPtr;
    }

    return ret;
  }

  int xcb_poll_for_reply(xcb_connection_t * a_0,unsigned int a_1,void ** a_2,xcb_generic_error_t ** a_3){
    auto ret = fexfn_pack_xcb_poll_for_reply(a_0, a_1, a_2, a_3);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(*a_2);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, *a_2, Usable);

      FEX_malloc_free_on_host(*a_2);
      *a_2 = NewPtr;
    }

    if (a_3 && *a_3) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_3, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_3);

      // User is expected to free this
      *a_3 = NewError;
    }

    return ret;
  }

  int xcb_poll_for_reply64(xcb_connection_t * a_0,uint64_t a_1,void ** a_2,xcb_generic_error_t ** a_3){
    auto ret = fexfn_pack_xcb_poll_for_reply64(a_0, a_1, a_2, a_3);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(*a_2);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, *a_2, Usable);

      FEX_malloc_free_on_host(*a_2);
      *a_2 = NewPtr;
    }

    if (a_3 && *a_3) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_3, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_3);

      // User is expected to free this
      *a_3 = NewError;
    }

    return ret;
  }

  xcb_query_extension_reply_t * xcb_query_extension_reply(xcb_connection_t * a_0,xcb_query_extension_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_extension_reply(a_0, a_1, a_2);

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
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }


    return ret;
  }

  xcb_get_window_attributes_reply_t * xcb_get_window_attributes_reply(xcb_connection_t * a_0,xcb_get_window_attributes_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_window_attributes_reply(a_0, a_1, a_2);
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
      void *NewPtr = malloc(sizeof(xcb_get_window_attributes_reply_t));
      memcpy(NewPtr, ret, sizeof(xcb_get_window_attributes_reply_t));

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_get_geometry_reply_t * xcb_get_geometry_reply(xcb_connection_t * a_0,xcb_get_geometry_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_geometry_reply(a_0, a_1, a_2);

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
      void *NewPtr = malloc(sizeof(xcb_get_geometry_reply_t));
      memcpy(NewPtr, ret, sizeof(xcb_get_geometry_reply_t));

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_intern_atom_reply_t * xcb_intern_atom_reply(xcb_connection_t * a_0,xcb_intern_atom_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_intern_atom_reply(a_0, a_1, a_2);

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
      // Usable size
      size_t Usable = FEX_malloc_usable_size(ret);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, ret, Usable);

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_get_property_reply_t * xcb_get_property_reply(xcb_connection_t * a_0,xcb_get_property_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_property_reply(a_0, a_1, a_2);

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
      void *NewPtr = malloc(sizeof(xcb_get_property_reply_t));
      memcpy(NewPtr, ret, sizeof(xcb_get_property_reply_t));

      FEX_malloc_free_on_host(ret);
      ret = (decltype(ret))NewPtr;
    }

    return ret;
  }

  xcb_query_tree_reply_t * xcb_query_tree_reply(xcb_connection_t * a_0,xcb_query_tree_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_tree_reply(a_0, a_1, a_2);

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

  xcb_get_atom_name_reply_t * xcb_get_atom_name_reply(xcb_connection_t * a_0,xcb_get_atom_name_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_atom_name_reply(a_0, a_1, a_2);

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

  xcb_list_properties_reply_t * xcb_list_properties_reply(xcb_connection_t * a_0,xcb_list_properties_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_properties_reply(a_0, a_1, a_2);

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

  xcb_get_selection_owner_reply_t * xcb_get_selection_owner_reply(xcb_connection_t * a_0,xcb_get_selection_owner_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_selection_owner_reply(a_0, a_1, a_2);

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

  xcb_grab_pointer_reply_t * xcb_grab_pointer_reply(xcb_connection_t * a_0,xcb_grab_pointer_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_grab_pointer_reply(a_0, a_1, a_2);

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

  xcb_grab_keyboard_reply_t * xcb_grab_keyboard_reply(xcb_connection_t * a_0,xcb_grab_keyboard_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_grab_keyboard_reply(a_0, a_1, a_2);

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

  xcb_query_pointer_reply_t * xcb_query_pointer_reply(xcb_connection_t * a_0,xcb_query_pointer_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_pointer_reply(a_0, a_1, a_2);

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

  xcb_get_motion_events_reply_t * xcb_get_motion_events_reply(xcb_connection_t * a_0,xcb_get_motion_events_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_motion_events_reply(a_0, a_1, a_2);

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

  xcb_translate_coordinates_reply_t * xcb_translate_coordinates_reply(xcb_connection_t * a_0,xcb_translate_coordinates_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_translate_coordinates_reply(a_0, a_1, a_2);

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

  xcb_get_input_focus_reply_t * xcb_get_input_focus_reply(xcb_connection_t * a_0,xcb_get_input_focus_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_input_focus_reply(a_0, a_1, a_2);

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

  xcb_query_keymap_reply_t * xcb_query_keymap_reply(xcb_connection_t * a_0,xcb_query_keymap_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_keymap_reply(a_0, a_1, a_2);

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

  xcb_query_font_reply_t * xcb_query_font_reply(xcb_connection_t * a_0,xcb_query_font_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_font_reply(a_0, a_1, a_2);

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

  xcb_query_text_extents_reply_t * xcb_query_text_extents_reply(xcb_connection_t * a_0,xcb_query_text_extents_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_text_extents_reply(a_0, a_1, a_2);

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

  xcb_list_fonts_reply_t * xcb_list_fonts_reply(xcb_connection_t * a_0,xcb_list_fonts_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_fonts_reply(a_0, a_1, a_2);

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

  xcb_list_fonts_with_info_reply_t * xcb_list_fonts_with_info_reply(xcb_connection_t * a_0,xcb_list_fonts_with_info_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_fonts_with_info_reply(a_0, a_1, a_2);

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

  xcb_get_font_path_reply_t * xcb_get_font_path_reply(xcb_connection_t * a_0,xcb_get_font_path_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_font_path_reply(a_0, a_1, a_2);

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

  xcb_get_image_reply_t * xcb_get_image_reply(xcb_connection_t * a_0,xcb_get_image_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_image_reply(a_0, a_1, a_2);

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

  xcb_list_installed_colormaps_reply_t * xcb_list_installed_colormaps_reply(xcb_connection_t * a_0,xcb_list_installed_colormaps_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_installed_colormaps_reply(a_0, a_1, a_2);

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

  xcb_alloc_color_reply_t * xcb_alloc_color_reply(xcb_connection_t * a_0,xcb_alloc_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_alloc_color_reply(a_0, a_1, a_2);

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

  xcb_alloc_named_color_reply_t * xcb_alloc_named_color_reply(xcb_connection_t * a_0,xcb_alloc_named_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_alloc_named_color_reply(a_0, a_1, a_2);

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

  xcb_alloc_color_cells_reply_t * xcb_alloc_color_cells_reply(xcb_connection_t * a_0,xcb_alloc_color_cells_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_alloc_color_cells_reply(a_0, a_1, a_2);

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

  xcb_alloc_color_planes_reply_t * xcb_alloc_color_planes_reply(xcb_connection_t * a_0,xcb_alloc_color_planes_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_alloc_color_planes_reply(a_0, a_1, a_2);

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

  xcb_query_colors_reply_t * xcb_query_colors_reply(xcb_connection_t * a_0,xcb_query_colors_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_colors_reply(a_0, a_1, a_2);

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

  xcb_lookup_color_reply_t * xcb_lookup_color_reply(xcb_connection_t * a_0,xcb_lookup_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_lookup_color_reply(a_0, a_1, a_2);

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

  xcb_query_best_size_reply_t * xcb_query_best_size_reply(xcb_connection_t * a_0,xcb_query_best_size_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_query_best_size_reply(a_0, a_1, a_2);

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

  xcb_list_extensions_reply_t * xcb_list_extensions_reply(xcb_connection_t * a_0,xcb_list_extensions_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_extensions_reply(a_0, a_1, a_2);

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

  xcb_get_keyboard_mapping_reply_t * xcb_get_keyboard_mapping_reply(xcb_connection_t * a_0,xcb_get_keyboard_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_keyboard_mapping_reply(a_0, a_1, a_2);

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

  xcb_get_keyboard_control_reply_t * xcb_get_keyboard_control_reply(xcb_connection_t * a_0,xcb_get_keyboard_control_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_keyboard_control_reply(a_0, a_1, a_2);

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

  xcb_get_pointer_control_reply_t * xcb_get_pointer_control_reply(xcb_connection_t * a_0,xcb_get_pointer_control_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_pointer_control_reply(a_0, a_1, a_2);

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

  xcb_get_screen_saver_reply_t * xcb_get_screen_saver_reply(xcb_connection_t * a_0,xcb_get_screen_saver_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_screen_saver_reply(a_0, a_1, a_2);

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

  xcb_list_hosts_reply_t * xcb_list_hosts_reply(xcb_connection_t * a_0,xcb_list_hosts_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_list_hosts_reply(a_0, a_1, a_2);

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

  xcb_set_pointer_mapping_reply_t * xcb_set_pointer_mapping_reply(xcb_connection_t * a_0,xcb_set_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_set_pointer_mapping_reply(a_0, a_1, a_2);

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

  xcb_get_pointer_mapping_reply_t * xcb_get_pointer_mapping_reply(xcb_connection_t * a_0,xcb_get_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_pointer_mapping_reply(a_0, a_1, a_2);

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

  xcb_set_modifier_mapping_reply_t * xcb_set_modifier_mapping_reply(xcb_connection_t * a_0,xcb_set_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_set_modifier_mapping_reply(a_0, a_1, a_2);

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

  xcb_get_modifier_mapping_reply_t * xcb_get_modifier_mapping_reply(xcb_connection_t * a_0,xcb_get_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    auto ret = fexfn_pack_xcb_get_modifier_mapping_reply(a_0, a_1, a_2);

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

LOAD_LIB(libxcb)
