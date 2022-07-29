/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <X11/XKBlib.h>

// Include Xlibint.h and undefine some of its macros that clash with the standard library
#include <X11/Xlibint.h>
#undef min
#undef max

#include <cstdint>
#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunks.inl"
#include "callback_unpacks.inl"

#include "function_packs.inl"
#include "function_packs_public.inl"


// Custom implementations //

#include <vector>

extern "C" {
    char* XGetICValues(XIC ic, ...) {
        fprintf(stderr, "XGetICValues\n");
        va_list ap;
        std::vector<unsigned long> args;
        va_start(ap, ic);
        for (;;) {
            auto arg = va_arg(ap, unsigned long);
            if (arg == 0)
                break;
            args.push_back(arg);
            fprintf(stderr, "%016lX\n", arg);
        }

        va_end(ap);
        auto rv = fexfn_pack_XGetICValues_internal(ic, args.size(), &args[0]);
        fprintf(stderr, "RV: %p\n", rv);
        return rv;
    }

    char* XGetIMValues(XIM ic, ...) {
        fprintf(stderr, "XGetIMValues\n");
        va_list ap;
        std::vector<void*> args;
        va_start(ap, ic);
        for (;;) {
            auto arg = va_arg(ap, void*);
            if (arg == 0)
                break;
            args.push_back(arg);
            fprintf(stderr, "%p\n", arg);
        }

        va_end(ap);
        auto rv = fexfn_pack_XGetIMValues_internal(ic, args.size(), &args[0]);
        fprintf(stderr, "RV: %p\n", rv);
        return rv;
    }

    _XIC* XCreateIC(XIM im, ...) {
        fprintf(stderr, "XCreateIC\n");
        va_list ap;
        std::vector<unsigned long> args;
        va_start(ap, im);
        for (;;) {
            auto arg = va_arg(ap, unsigned long);
            if (arg == 0)
                break;
            args.push_back(arg);
            fprintf(stderr, "%016lX\n", arg);
        }

        va_end(ap);
        auto rv = fexfn_pack_XCreateIC_internal(im, args.size(), &args[0]);
        fprintf(stderr, "RV: %p\n", rv);
        return rv;
    }

    static void LockMutexFunction(LockInfoPtr) {
      fprintf(stderr, "libX11: LockMutex\n");
    }

    static void UnlockMutexFunction(LockInfoPtr) {
      fprintf(stderr, "libX11: LockMutex\n");
    }

  int XFree(void* ptr) {
    // This function must be able to handle both guest heap pointers *and* host heap pointers,
    // so it only forwards to the native host library for the latter.
    //
    // This is because Xlibint users allocate memory using internal macros aliasing to libc's
    // malloc but then they free using the function XFree. For libX11, this is not a problem since
    // the allocation happens in a thunked API function (and hence on the host heap), but if a
    // function from an unthunked library accesses Xlibint, it will allocate on the guest heap.
    //
    // One notable example where this was encountered is XF86VidModeGetAllModeLines.

    if (!ptr || IsHostHeapAllocation(ptr)) {
      return fexfn_pack_XFree(ptr);
    } else {
      free(ptr);
      return 1;
    }
  }

  void XFreeEventData(Display* display, XGenericEventCookie* cookie) {
    // Has the same heap-mismatch issue as XFree, so we have to reimplement it manually
    if (_XIsEventCookie(display, (XEvent*)cookie) && cookie->data) {
      XFree(cookie->data);
      cookie->data = nullptr;
      cookie->cookie = 0;
    }
  }

  Display* XOpenDisplay(const char* name) {
    auto ret = fexfn_pack_XOpenDisplay(name);

    for (auto& funcptr : ret->event_vec) {
      if (!funcptr) {
        continue;
      }
      auto caller = (uintptr_t)GetCallerForHostFunction(funcptr);
      LinkAddressToFunction((uintptr_t)funcptr, (uintptr_t)caller);
    }

    for (auto& funcptr : ret->wire_vec) {
      if (!funcptr) {
        continue;
      }
      auto caller = (uintptr_t)GetCallerForHostFunction(funcptr);
      LinkAddressToFunction((uintptr_t)funcptr, (uintptr_t)caller);
    }

    {
      auto caller = (uintptr_t)GetCallerForHostFunction(ret->resource_alloc);
      LinkAddressToFunction((uintptr_t)ret->resource_alloc, (uintptr_t)caller);
    }

    {
      auto caller = (uintptr_t)GetCallerForHostFunction(ret->idlist_alloc);
      LinkAddressToFunction((uintptr_t)ret->idlist_alloc, (uintptr_t)caller);
    }

    {
      auto caller = (uintptr_t)GetCallerForHostFunction(ret->exit_handler);
      LinkAddressToFunction((uintptr_t)ret->exit_handler, (uintptr_t)caller);
    }

    return ret;
  }

  void (*_XLockMutex_fn)(LockInfoPtr) = LockMutexFunction;
  void (*_XUnlockMutex_fn)(LockInfoPtr) = UnlockMutexFunction;
  LockInfoPtr _Xglobal_lock = (LockInfoPtr)0x4142434445464748ULL;
}

LOAD_LIB(libX11)
