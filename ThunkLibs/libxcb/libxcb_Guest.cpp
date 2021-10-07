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
#include "WorkEventData.h"
#include "common/CrossArchEvent.h"

#include <stdarg.h>

#include "callback_typedefs.inl"

using CBType = void (*)(void *closure);

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"
#include "callback_unpacks.inl"

static std::thread CBThread{};
static std::atomic<bool> CBDone{false};

static CrossArchEvent WaitForWork{};
static CrossArchEvent WorkDone{};
static CBWork CBWorkData{};

void FEX_Helper_GiveEvents() {
  struct {
    CrossArchEvent *WaitForWork;
    CrossArchEvent *WorkDone;
    CBWork *Work;
  } args;

  args.WaitForWork = &WaitForWork;
  args.WorkDone = &WorkDone;
  args.Work = &CBWorkData;

  fexthunks_libxcb_FEX_GiveEvents(&args);
}

static void CallbackThreadFunc() {
  // Set the thread name to make it easy to know what thread this is
  pthread_setname_np(pthread_self(), "xcb:take_socket");

  // Hand the host our helpers
  FEX_Helper_GiveEvents();
  while (!CBDone) {
    WaitForWorkFunc(&WaitForWork);
    if (CBDone) {
      return;
    }
    typedef void take_xcb_fn_t (void* a_0);
    auto callback = reinterpret_cast<take_xcb_fn_t*>(CBWorkData.cb);

    // On shutdown then callback can change to nullptr
    if (callback) {
      callback(CBWorkData.argsv);
    }
    NotifyWorkFunc(&WorkDone);
  }
}

// Callbacks
static void fexfn_unpack_libxcb_xcb_take_socket_cb(uintptr_t cb, void *argsv){
  // Hand the callback off to our native thread which will have proper TLS
  CBWorkData.cb = cb;
  CBWorkData.argsv = argsv;
  // Tell the thread it has work
  NotifyWorkFunc(&WaitForWork);
  // Wait for the work to be done
  WaitForWorkFunc(&WorkDone);
}

extern "C" {
  static void init_lib() {
    // Start a guest side thread that allows us to do callbacks from xcb safely
    if (!CBThread.joinable()) {
      CBThread = std::thread(CallbackThreadFunc);
    }
  }
  __attribute__((destructor)) static void close_lib() {
    if (CBThread.joinable()) {
      CBDone = true;
      NotifyWorkFunc(&WaitForWork);
      NotifyWorkFunc(&WorkDone);
      CBThread.join();
    }
  }

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

  int xcb_take_socket(xcb_connection_t * a_0,CBType a_1,void * a_2,int a_3,uint64_t * a_4){
    return xcb_take_socket_internal(a_0, a_1, a_2, a_3, a_4);
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

  static xcb_connection_t * fexfn_pack_xcb_connect(const char * a_0,int * a_1){
    struct {const char * a_0;int * a_1;xcb_connection_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;
    fexthunks_libxcb_xcb_connect(&args);
    InitializeExtensions(args.rv);
    return args.rv;
  }

  static xcb_connection_t * fexfn_pack_xcb_connect_to_display_with_auth_info(const char * a_0,xcb_auth_info_t * a_1,int * a_2){
    struct {const char * a_0;xcb_auth_info_t * a_1;int * a_2;xcb_connection_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_connect_to_display_with_auth_info(&args);
    InitializeExtensions(args.rv);
    return args.rv;
  }

  static void fexfn_pack_xcb_disconnect(xcb_connection_t * a_0){
    struct {xcb_connection_t * a_0;} args;
    args.a_0 = a_0;
    fexthunks_libxcb_xcb_disconnect(&args);
  }

  static int fexfn_pack_xcb_parse_display(const char * a_0,char ** a_1,int * a_2,int * a_3){
    struct {const char * a_0;char ** a_1;int * a_2;int * a_3;int rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
    fexthunks_libxcb_xcb_parse_display(&args);
    if (a_1 && *a_1) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(*a_1);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, *a_1, Usable);

      FEX_malloc_free_on_host(*a_1);
      *a_1 = (char*)NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_event_t * fexfn_pack_xcb_wait_for_event(xcb_connection_t * a_0){
    struct {xcb_connection_t * a_0;xcb_generic_event_t * rv;} args;
    args.a_0 = a_0;
    fexthunks_libxcb_xcb_wait_for_event(&args);

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_event_t * fexfn_pack_xcb_poll_for_event(xcb_connection_t * a_0){
    struct {xcb_connection_t * a_0;xcb_generic_event_t * rv;} args;
    args.a_0 = a_0;
    fexthunks_libxcb_xcb_poll_for_event(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_event_t * fexfn_pack_xcb_poll_for_queued_event(xcb_connection_t * a_0){
    struct {xcb_connection_t * a_0;xcb_generic_event_t * rv;} args;
    args.a_0 = a_0;
    fexthunks_libxcb_xcb_poll_for_queued_event(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_event_t * fexfn_pack_xcb_poll_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1){
    struct {xcb_connection_t * a_0;xcb_special_event_t * a_1;xcb_generic_event_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;
    fexthunks_libxcb_xcb_poll_for_special_event(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_event_t * fexfn_pack_xcb_wait_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1){
    struct {xcb_connection_t * a_0;xcb_special_event_t * a_1;xcb_generic_event_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;
    fexthunks_libxcb_xcb_wait_for_special_event(&args);
    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_generic_error_t * fexfn_pack_xcb_request_check(xcb_connection_t * a_0,xcb_void_cookie_t a_1){
    struct {xcb_connection_t * a_0;xcb_void_cookie_t a_1;xcb_generic_error_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;
    fexthunks_libxcb_xcb_request_check(&args);
    if (args.rv) {
      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(sizeof(xcb_generic_error_t));
      memcpy(NewPtr, args.rv, sizeof(xcb_generic_error_t));

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }
    return args.rv;
  }

  static void * fexfn_pack_xcb_wait_for_reply(xcb_connection_t * a_0, uint32_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;uint32_t a_1;xcb_generic_error_t ** a_2;void * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_wait_for_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static void * fexfn_pack_xcb_wait_for_reply64(xcb_connection_t * a_0,uint64_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;uint64_t a_1;xcb_generic_error_t ** a_2;void * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_wait_for_reply64(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = NewPtr;
    }

    return args.rv;
  }

  static int fexfn_pack_xcb_poll_for_reply(xcb_connection_t * a_0,unsigned int a_1,void ** a_2,xcb_generic_error_t ** a_3){
    struct {xcb_connection_t * a_0;unsigned int a_1;void ** a_2;xcb_generic_error_t ** a_3;int rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
    fexthunks_libxcb_xcb_poll_for_reply(&args);

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

    return args.rv;
  }

  static int fexfn_pack_xcb_poll_for_reply64(xcb_connection_t * a_0,uint64_t a_1,void ** a_2,xcb_generic_error_t ** a_3){
    struct {xcb_connection_t * a_0;uint64_t a_1;void ** a_2;xcb_generic_error_t ** a_3;int rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
    fexthunks_libxcb_xcb_poll_for_reply64(&args);

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

    return args.rv;
  }

  static xcb_query_extension_reply_t * fexfn_pack_xcb_query_extension_reply(xcb_connection_t * a_0,xcb_query_extension_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_extension_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_extension_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_extension_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }


    return args.rv;
  }

  static xcb_get_window_attributes_reply_t * fexfn_pack_xcb_get_window_attributes_reply(xcb_connection_t * a_0,xcb_get_window_attributes_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_window_attributes_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_window_attributes_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_window_attributes_reply(&args);
    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      void *NewPtr = malloc(sizeof(xcb_get_window_attributes_reply_t));
      memcpy(NewPtr, args.rv, sizeof(xcb_get_window_attributes_reply_t));

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_geometry_reply_t * fexfn_pack_xcb_get_geometry_reply(xcb_connection_t * a_0,xcb_get_geometry_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_geometry_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_geometry_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_geometry_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      void *NewPtr = malloc(sizeof(xcb_get_geometry_reply_t));
      memcpy(NewPtr, args.rv, sizeof(xcb_get_geometry_reply_t));

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_intern_atom_reply_t * fexfn_pack_xcb_intern_atom_reply(xcb_connection_t * a_0,xcb_intern_atom_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_intern_atom_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_intern_atom_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_intern_atom_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      // Usable size
      size_t Usable = FEX_malloc_usable_size(args.rv);

      // This will be a bit wasteful but this is an unsized pointer
      void *NewPtr = malloc(Usable);
      memcpy(NewPtr, args.rv, Usable);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_property_reply_t * fexfn_pack_xcb_get_property_reply(xcb_connection_t * a_0,xcb_get_property_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_property_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_property_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_property_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      void *NewPtr = malloc(sizeof(xcb_get_property_reply_t));
      memcpy(NewPtr, args.rv, sizeof(xcb_get_property_reply_t));

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_tree_reply_t * fexfn_pack_xcb_query_tree_reply(xcb_connection_t * a_0,xcb_query_tree_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_tree_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_tree_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_tree_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_atom_name_reply_t * fexfn_pack_xcb_get_atom_name_reply(xcb_connection_t * a_0,xcb_get_atom_name_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_atom_name_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_atom_name_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_atom_name_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_properties_reply_t * fexfn_pack_xcb_list_properties_reply(xcb_connection_t * a_0,xcb_list_properties_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_properties_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_properties_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_properties_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_selection_owner_reply_t * fexfn_pack_xcb_get_selection_owner_reply(xcb_connection_t * a_0,xcb_get_selection_owner_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_selection_owner_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_selection_owner_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_selection_owner_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_grab_pointer_reply_t * fexfn_pack_xcb_grab_pointer_reply(xcb_connection_t * a_0,xcb_grab_pointer_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_grab_pointer_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_grab_pointer_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_grab_pointer_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_grab_keyboard_reply_t * fexfn_pack_xcb_grab_keyboard_reply(xcb_connection_t * a_0,xcb_grab_keyboard_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_grab_keyboard_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_grab_keyboard_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_grab_keyboard_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_pointer_reply_t * fexfn_pack_xcb_query_pointer_reply(xcb_connection_t * a_0,xcb_query_pointer_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_pointer_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_pointer_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_pointer_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_motion_events_reply_t * fexfn_pack_xcb_get_motion_events_reply(xcb_connection_t * a_0,xcb_get_motion_events_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_motion_events_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_motion_events_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_motion_events_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_translate_coordinates_reply_t * fexfn_pack_xcb_translate_coordinates_reply(xcb_connection_t * a_0,xcb_translate_coordinates_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_translate_coordinates_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_translate_coordinates_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_translate_coordinates_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_input_focus_reply_t * fexfn_pack_xcb_get_input_focus_reply(xcb_connection_t * a_0,xcb_get_input_focus_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_input_focus_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_input_focus_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_input_focus_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_keymap_reply_t * fexfn_pack_xcb_query_keymap_reply(xcb_connection_t * a_0,xcb_query_keymap_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_keymap_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_keymap_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_keymap_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_font_reply_t * fexfn_pack_xcb_query_font_reply(xcb_connection_t * a_0,xcb_query_font_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_font_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_font_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_font_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_text_extents_reply_t * fexfn_pack_xcb_query_text_extents_reply(xcb_connection_t * a_0,xcb_query_text_extents_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_text_extents_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_text_extents_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_text_extents_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_fonts_reply_t * fexfn_pack_xcb_list_fonts_reply(xcb_connection_t * a_0,xcb_list_fonts_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_fonts_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_fonts_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_fonts_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_fonts_with_info_reply_t * fexfn_pack_xcb_list_fonts_with_info_reply(xcb_connection_t * a_0,xcb_list_fonts_with_info_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_fonts_with_info_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_fonts_with_info_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_fonts_with_info_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_font_path_reply_t * fexfn_pack_xcb_get_font_path_reply(xcb_connection_t * a_0,xcb_get_font_path_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_font_path_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_font_path_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_font_path_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_image_reply_t * fexfn_pack_xcb_get_image_reply(xcb_connection_t * a_0,xcb_get_image_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_image_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_image_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_image_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_installed_colormaps_reply_t * fexfn_pack_xcb_list_installed_colormaps_reply(xcb_connection_t * a_0,xcb_list_installed_colormaps_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_installed_colormaps_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_installed_colormaps_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_installed_colormaps_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_alloc_color_reply_t * fexfn_pack_xcb_alloc_color_reply(xcb_connection_t * a_0,xcb_alloc_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_alloc_color_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_alloc_color_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_alloc_color_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_alloc_named_color_reply_t * fexfn_pack_xcb_alloc_named_color_reply(xcb_connection_t * a_0,xcb_alloc_named_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_alloc_named_color_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_alloc_named_color_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_alloc_named_color_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_alloc_color_cells_reply_t * fexfn_pack_xcb_alloc_color_cells_reply(xcb_connection_t * a_0,xcb_alloc_color_cells_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_alloc_color_cells_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_alloc_color_cells_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_alloc_color_cells_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_alloc_color_planes_reply_t * fexfn_pack_xcb_alloc_color_planes_reply(xcb_connection_t * a_0,xcb_alloc_color_planes_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_alloc_color_planes_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_alloc_color_planes_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_alloc_color_planes_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_colors_reply_t * fexfn_pack_xcb_query_colors_reply(xcb_connection_t * a_0,xcb_query_colors_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_colors_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_colors_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_colors_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_lookup_color_reply_t * fexfn_pack_xcb_lookup_color_reply(xcb_connection_t * a_0,xcb_lookup_color_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_lookup_color_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_lookup_color_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_lookup_color_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_query_best_size_reply_t * fexfn_pack_xcb_query_best_size_reply(xcb_connection_t * a_0,xcb_query_best_size_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_query_best_size_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_query_best_size_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_query_best_size_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_extensions_reply_t * fexfn_pack_xcb_list_extensions_reply(xcb_connection_t * a_0,xcb_list_extensions_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_extensions_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_extensions_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_extensions_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_keyboard_mapping_reply_t * fexfn_pack_xcb_get_keyboard_mapping_reply(xcb_connection_t * a_0,xcb_get_keyboard_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_keyboard_mapping_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_keyboard_mapping_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_keyboard_mapping_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_keyboard_control_reply_t * fexfn_pack_xcb_get_keyboard_control_reply(xcb_connection_t * a_0,xcb_get_keyboard_control_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_keyboard_control_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_keyboard_control_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_keyboard_control_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_pointer_control_reply_t * fexfn_pack_xcb_get_pointer_control_reply(xcb_connection_t * a_0,xcb_get_pointer_control_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_pointer_control_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_pointer_control_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_pointer_control_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_screen_saver_reply_t * fexfn_pack_xcb_get_screen_saver_reply(xcb_connection_t * a_0,xcb_get_screen_saver_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_screen_saver_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_screen_saver_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_screen_saver_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_list_hosts_reply_t * fexfn_pack_xcb_list_hosts_reply(xcb_connection_t * a_0,xcb_list_hosts_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_list_hosts_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_list_hosts_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_list_hosts_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_set_pointer_mapping_reply_t * fexfn_pack_xcb_set_pointer_mapping_reply(xcb_connection_t * a_0,xcb_set_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_set_pointer_mapping_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_set_pointer_mapping_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_set_pointer_mapping_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_pointer_mapping_reply_t * fexfn_pack_xcb_get_pointer_mapping_reply(xcb_connection_t * a_0,xcb_get_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_pointer_mapping_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_pointer_mapping_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_pointer_mapping_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_set_modifier_mapping_reply_t * fexfn_pack_xcb_set_modifier_mapping_reply(xcb_connection_t * a_0,xcb_set_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_set_modifier_mapping_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_set_modifier_mapping_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_set_modifier_mapping_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  static xcb_get_modifier_mapping_reply_t * fexfn_pack_xcb_get_modifier_mapping_reply(xcb_connection_t * a_0,xcb_get_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2){
    struct {xcb_connection_t * a_0;xcb_get_modifier_mapping_cookie_t a_1;xcb_generic_error_t ** a_2;xcb_get_modifier_mapping_reply_t * rv;} args;
    args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
    fexthunks_libxcb_xcb_get_modifier_mapping_reply(&args);

    // We now need to do some fixups here
    if (a_2 && *a_2) {
      // If the error code pointer exists then we need to copy the contents and free the host facing pointer
      xcb_generic_error_t *NewError = (xcb_generic_error_t *)malloc(sizeof(xcb_generic_error_t));
      memcpy(NewError, *a_2, sizeof(xcb_generic_error_t));
      FEX_malloc_free_on_host(*a_2);

      // User is expected to free this
      *a_2 = NewError;
    }

    if (args.rv) {
      constexpr size_t ResultSize = sizeof(std::remove_pointer<decltype(args.rv)>::type);
      void *NewPtr = malloc(ResultSize);
      memcpy(NewPtr, args.rv, ResultSize);

      FEX_malloc_free_on_host(args.rv);
      args.rv = (decltype(args.rv))NewPtr;
    }

    return args.rv;
  }

  xcb_connection_t * xcb_connect(const char * a_0,int * a_2) __attribute__((alias("fexfn_pack_xcb_connect")));
  xcb_connection_t * xcb_connect_to_display_with_auth_info(const char * a_0,xcb_auth_info_t * a_1,int * a_2) __attribute__((alias("fexfn_pack_xcb_connect_to_display_with_auth_info")));
  void xcb_disconnect(xcb_connection_t * a_0) __attribute__((alias("fexfn_pack_xcb_disconnect")));
  int xcb_parse_display(const char * a_0,char ** a_1,int * a_2,int * a_3) __attribute__((alias("fexfn_pack_xcb_parse_display")));

  xcb_generic_event_t * xcb_wait_for_event(xcb_connection_t * a_0) __attribute__((alias("fexfn_pack_xcb_wait_for_event")));
  xcb_generic_event_t * xcb_poll_for_event(xcb_connection_t * a_0) __attribute__((alias("fexfn_pack_xcb_poll_for_event")));
  xcb_generic_event_t * xcb_poll_for_queued_event(xcb_connection_t * a_0) __attribute__((alias("fexfn_pack_xcb_poll_for_queued_event")));
  xcb_generic_event_t * xcb_poll_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1) __attribute__((alias("fexfn_pack_xcb_poll_for_special_event")));
  xcb_generic_event_t * xcb_wait_for_special_event(xcb_connection_t * a_0,xcb_special_event_t * a_1) __attribute__((alias("fexfn_pack_xcb_wait_for_special_event")));
  xcb_generic_error_t * xcb_request_check(xcb_connection_t * a_0,xcb_void_cookie_t a_1) __attribute__((alias("fexfn_pack_xcb_request_check")));

  void * xcb_wait_for_reply(xcb_connection_t * a_0, uint32_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_wait_for_reply")));
  void * xcb_wait_for_reply64(xcb_connection_t * a_0,uint64_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_wait_for_reply64")));
  int xcb_poll_for_reply(xcb_connection_t * a_0,unsigned int a_1,void ** a_2,xcb_generic_error_t ** a_3) __attribute__((alias("fexfn_pack_xcb_poll_for_reply")));
  int xcb_poll_for_reply64(xcb_connection_t * a_0,uint64_t a_1,void ** a_2,xcb_generic_error_t ** a_3) __attribute__((alias("fexfn_pack_xcb_poll_for_reply64")));

  xcb_query_extension_reply_t * xcb_query_extension_reply(xcb_connection_t * a_0,xcb_query_extension_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_extension_reply")));

  xcb_get_window_attributes_reply_t * xcb_get_window_attributes_reply(xcb_connection_t * a_0,xcb_get_window_attributes_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_window_attributes_reply")));
  xcb_get_geometry_reply_t * xcb_get_geometry_reply(xcb_connection_t * a_0,xcb_get_geometry_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_geometry_reply")));
  xcb_intern_atom_reply_t * xcb_intern_atom_reply(xcb_connection_t * a_0,xcb_intern_atom_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_intern_atom_reply")));

  xcb_get_property_reply_t * xcb_get_property_reply(xcb_connection_t * a_0,xcb_get_property_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_property_reply")));
  xcb_query_tree_reply_t * xcb_query_tree_reply(xcb_connection_t * a_0,xcb_query_tree_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_tree_reply")));
  xcb_get_atom_name_reply_t * xcb_get_atom_name_reply(xcb_connection_t * a_0,xcb_get_atom_name_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_atom_name_reply")));
  xcb_list_properties_reply_t * xcb_list_properties_reply(xcb_connection_t * a_0,xcb_list_properties_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_properties_reply")));
  xcb_get_selection_owner_reply_t * xcb_get_selection_owner_reply(xcb_connection_t * a_0,xcb_get_selection_owner_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_selection_owner_reply")));
  xcb_grab_pointer_reply_t * xcb_grab_pointer_reply(xcb_connection_t * a_0,xcb_grab_pointer_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_grab_pointer_reply")));
  xcb_grab_keyboard_reply_t * xcb_grab_keyboard_reply(xcb_connection_t * a_0,xcb_grab_keyboard_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_grab_keyboard_reply")));
  xcb_query_pointer_reply_t * xcb_query_pointer_reply(xcb_connection_t * a_0,xcb_query_pointer_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_pointer_reply")));
  xcb_get_motion_events_reply_t * xcb_get_motion_events_reply(xcb_connection_t * a_0,xcb_get_motion_events_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_motion_events_reply")));
  xcb_translate_coordinates_reply_t * xcb_translate_coordinates_reply(xcb_connection_t * a_0,xcb_translate_coordinates_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_translate_coordinates_reply")));
  xcb_get_input_focus_reply_t * xcb_get_input_focus_reply(xcb_connection_t * a_0,xcb_get_input_focus_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_input_focus_reply")));
  xcb_query_keymap_reply_t * xcb_query_keymap_reply(xcb_connection_t * a_0,xcb_query_keymap_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_keymap_reply")));
  xcb_query_font_reply_t * xcb_query_font_reply(xcb_connection_t * a_0,xcb_query_font_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_font_reply")));
  xcb_query_text_extents_reply_t * xcb_query_text_extents_reply(xcb_connection_t * a_0,xcb_query_text_extents_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_text_extents_reply")));
  xcb_list_fonts_reply_t * xcb_list_fonts_reply(xcb_connection_t * a_0,xcb_list_fonts_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_fonts_reply")));
  xcb_list_fonts_with_info_reply_t * xcb_list_fonts_with_info_reply(xcb_connection_t * a_0,xcb_list_fonts_with_info_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_fonts_with_info_reply")));
  xcb_get_font_path_reply_t * xcb_get_font_path_reply(xcb_connection_t * a_0,xcb_get_font_path_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_font_path_reply")));
  xcb_get_image_reply_t * xcb_get_image_reply(xcb_connection_t * a_0,xcb_get_image_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_image_reply")));
  xcb_list_installed_colormaps_reply_t * xcb_list_installed_colormaps_reply(xcb_connection_t * a_0,xcb_list_installed_colormaps_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_installed_colormaps_reply")));
  xcb_alloc_color_reply_t * xcb_alloc_color_reply(xcb_connection_t * a_0,xcb_alloc_color_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_alloc_color_reply")));
  xcb_alloc_named_color_reply_t * xcb_alloc_named_color_reply(xcb_connection_t * a_0,xcb_alloc_named_color_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_alloc_named_color_reply")));
  xcb_alloc_color_cells_reply_t * xcb_alloc_color_cells_reply(xcb_connection_t * a_0,xcb_alloc_color_cells_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_alloc_color_cells_reply")));
  xcb_alloc_color_planes_reply_t * xcb_alloc_color_planes_reply(xcb_connection_t * a_0,xcb_alloc_color_planes_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_alloc_color_planes_reply")));
  xcb_query_colors_reply_t * xcb_query_colors_reply(xcb_connection_t * a_0,xcb_query_colors_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_colors_reply")));
  xcb_lookup_color_reply_t * xcb_lookup_color_reply(xcb_connection_t * a_0,xcb_lookup_color_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_lookup_color_reply")));
  xcb_query_best_size_reply_t * xcb_query_best_size_reply(xcb_connection_t * a_0,xcb_query_best_size_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_query_best_size_reply")));
  xcb_list_extensions_reply_t * xcb_list_extensions_reply(xcb_connection_t * a_0,xcb_list_extensions_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_extensions_reply")));
  xcb_get_keyboard_mapping_reply_t * xcb_get_keyboard_mapping_reply(xcb_connection_t * a_0,xcb_get_keyboard_mapping_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_keyboard_mapping_reply")));
  xcb_get_keyboard_control_reply_t * xcb_get_keyboard_control_reply(xcb_connection_t * a_0,xcb_get_keyboard_control_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_keyboard_control_reply")));
  xcb_get_pointer_control_reply_t * xcb_get_pointer_control_reply(xcb_connection_t * a_0,xcb_get_pointer_control_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_pointer_control_reply")));
  xcb_get_screen_saver_reply_t * xcb_get_screen_saver_reply(xcb_connection_t * a_0,xcb_get_screen_saver_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_screen_saver_reply")));
  xcb_list_hosts_reply_t * xcb_list_hosts_reply(xcb_connection_t * a_0,xcb_list_hosts_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_list_hosts_reply")));
  xcb_set_pointer_mapping_reply_t * xcb_set_pointer_mapping_reply(xcb_connection_t * a_0,xcb_set_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_set_pointer_mapping_reply")));
  xcb_get_pointer_mapping_reply_t * xcb_get_pointer_mapping_reply(xcb_connection_t * a_0,xcb_get_pointer_mapping_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_pointer_mapping_reply")));
  xcb_set_modifier_mapping_reply_t * xcb_set_modifier_mapping_reply(xcb_connection_t * a_0,xcb_set_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_set_modifier_mapping_reply")));
  xcb_get_modifier_mapping_reply_t * xcb_get_modifier_mapping_reply(xcb_connection_t * a_0,xcb_get_modifier_mapping_cookie_t a_1,xcb_generic_error_t ** a_2) __attribute__((alias("fexfn_pack_xcb_get_modifier_mapping_reply")));
}

struct {
    #include "callback_unpacks_header.inl"
} callback_unpacks = {
    #include "callback_unpacks_header_init.inl"
};

LOAD_LIB_WITH_CALLBACKS_INIT(libxcb, init_lib)
