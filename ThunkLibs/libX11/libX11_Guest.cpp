/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

extern "C" {
#define XUTIL_DEFINE_FUNCTIONS
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <X11/Xproto.h>
#include <X11/XKBlib.h>

// Include Xlibint.h and undefine some of its macros that clash with the standard library
#include <X11/Xlibint.h>
#undef min
#undef max
}

#include <cstdint>
#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libX11.inl"

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

    char* XSetICValues(XIC ic, ...) {
        fprintf(stderr, "XSetICValues\n");
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
        auto rv = fexfn_pack_XSetICValues_internal(ic, args.size(), &args[0]);
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

    char* XSetIMValues(XIM ic, ...) {
        fprintf(stderr, "XSetIMValues\n");
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
        auto rv = fexfn_pack_XSetIMValues_internal(ic, args.size(), &args[0]);
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

    XVaNestedList XVaCreateNestedList(int unused_arg, ...) {
        fprintf(stderr, "XVaCreateNestedList\n");
        va_list ap;
        std::vector<void*> args;
        va_start(ap, unused_arg);
        for (;;) {
            auto arg = va_arg(ap, void*);
            if (arg == 0)
                break;
            args.push_back(arg);
            fprintf(stderr, "%p\n", arg);
        }

        va_end(ap);
        auto rv = fexfn_pack_XVaCreateNestedList_internal(unused_arg, args.size(), &args[0]);
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
      MakeHostFunctionGuestCallable(funcptr);
    }

    for (auto& funcptr : ret->wire_vec) {
      if (!funcptr) {
        continue;
      }
      MakeHostFunctionGuestCallable(funcptr);
    }

    MakeHostFunctionGuestCallable(ret->resource_alloc);
    MakeHostFunctionGuestCallable(ret->idlist_alloc);
#if (X11_VERSION_MAJOR >= 1 && X11_VERSION_MINOR >= 7 && X11_VERSION_PATCH >= 0)
    // Doesn't exist on older X11
    MakeHostFunctionGuestCallable(ret->exit_handler);
#endif

    return ret;
  }

  Status _XReply(Display* display, xReply* reply, int extra, Bool discard) {
    for(auto handler = display->async_handlers; handler; handler = handler->next) {
      // Make host-callable and overwrite in-place
      // NOTE: This data seems to be stack-allocated specifically for XReply usually, so it's *probably* safe to overwrite
      handler->handler = AllocateHostTrampolineForGuestFunction(handler->handler);
    }
    return fexfn_pack__XReply(display, reply, extra, discard);
  }

  static int _XInitDisplayLock(Display* display) {
    MakeHostFunctionGuestCallable(display->lock_fns->lock_display);
    MakeHostFunctionGuestCallable(display->lock_fns->unlock_display);
    return 0;
  }

  Status XInitThreads() {
    return fexfn_pack_XInitThreadsInternal((uintptr_t)_XInitDisplayLock, (uintptr_t)CallbackUnpack<decltype(_XInitDisplayLock)>::Unpack);
  }

  // Register the host function pointers written by _XInitImageFuncPtrs (and
  // its callers) to be guest-callable
  static void FixupImageFuncPtrs(XImage* image) {
    image->f.create_image = XCreateImage;
    MakeHostFunctionGuestCallable(image->f.destroy_image);
    MakeHostFunctionGuestCallable(image->f.get_pixel);
    MakeHostFunctionGuestCallable(image->f.put_pixel);
    // TODO: image->f.sub_image
    MakeHostFunctionGuestCallable(image->f.add_pixel);
  }

  void _XInitImageFuncPtrs(XImage* image) {
    fexfn_pack__XInitImageFuncPtrs(image);
    FixupImageFuncPtrs(image);
  }

  XImage *XCreateImage(
        Display* display, Visual* visual,
        unsigned int depth, int format,
        int offset, char* data,
        unsigned int width, unsigned int height,
        int pad, int bpp) {
    auto ret = fexfn_pack_XCreateImage(display, visual, depth, format, offset, data, width, height, pad, bpp);
    FixupImageFuncPtrs(ret);
    return ret;
  }

  Status XInitImage(XImage* image) {
    auto ret = fexfn_pack_XInitImage(image);
    FixupImageFuncPtrs(image);
    return ret;
  }

  void (*_XLockMutex_fn)(LockInfoPtr) = LockMutexFunction;
  void (*_XUnlockMutex_fn)(LockInfoPtr) = UnlockMutexFunction;
  LockInfoPtr _Xglobal_lock = (LockInfoPtr)0x4142434445464748ULL;
}

LOAD_LIB(libX11)
