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

#include <X11/extensions/extutil.h>

// Include Xlibint.h and undefine some of its macros that clash with the standard library
#include <X11/Xlibint.h>
#undef min
#undef max
}

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <stdio.h>
#include <cstring>
#include <map>
#include <list>
#include <string>
#include <unistd.h>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libX11.inl"
#include "X11Common.h"

// Custom implementations //

#include <vector>

namespace {
  // The various X11 variadic functions take a flattened sequence of key:value pairs as arguments.
  // The key element is a pointer to a string.
  // The value element is a pointer to the data of that key type.
  //
  // Some keys describe function callbacks for various events that the server interface can call.
  // FEX needs to walk these keys and ensure any callback function has a trampoline so the native
  // X11 library can call it.
  template<typename CallbackType>
  static std::list<CallbackType> ConvertCallbackArguments(std::vector<void*> &IncomingArguments) {
    assert(IncomingArguments.size() % 2 == 0 && "Incoming arguments needs to be in pairs");

    std::list<CallbackType> Callbacks;
    // Walk the arguments and convert any callbacks.
    const size_t ArgumentPairs = IncomingArguments.size() / 2;
    for (size_t i = 0; i < ArgumentPairs; ++i) {
      const char *Key = static_cast<const char*>(IncomingArguments[i * 2]);
      void** Data = &IncomingArguments[i * 2 + 1];
      if (!*Data) {
        continue;
      }

      // Check if the key is a callback and needs to be modified.
      auto KeyIt = std::find(X11::CallbackKeys.begin(), X11::CallbackKeys.end(), Key);
      if (KeyIt == X11::CallbackKeys.end()) {
        continue;
      }

      // Key matches a callback, we need to wrap this.
      CallbackType *IncomingCallback = reinterpret_cast<CallbackType*>(*Data);
      CallbackType *ConvertedCallback = &Callbacks.emplace_back(CallbackType {
        // Client data stays the same.
        .client_data = IncomingCallback->client_data,

        // Callback needs a trampoline.
        .callback = AllocateHostTrampolineForGuestFunction(IncomingCallback->callback),
      });

      // Add this converted back in.
      *Data = ConvertedCallback;
    }

    return Callbacks;
  }
}

extern "C" {
    char* XGetICValues(XIC ic, ...) {
      fprintf(stderr, "XGetICValues\n");
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

      auto rv = fexfn_pack_XGetICValues_internal(ic, args.size(), &args[0]);
      fprintf(stderr, "RV: %p\n", rv);
      return rv;
    }

    char* XSetICValues(XIC ic, ...) {
      fprintf(stderr, "XSetICValues\n");
      va_list ap;
      std::vector<void*> IncomingArguments;
      va_start(ap, ic);
      for (;;) {
        auto Key = va_arg(ap, void*);
        if (Key == 0)
          break;

        auto Value = va_arg(ap, void*);
        IncomingArguments.emplace_back(Key);
        IncomingArguments.emplace_back(Value);
      }

      va_end(ap);

      // Callback memory needs to live beyond internal function call.
      std::list<XICCallback> Callbacks = ConvertCallbackArguments<XICCallback>(IncomingArguments);

      auto rv = fexfn_pack_XSetICValues_internal(ic, IncomingArguments.size(), &IncomingArguments[0]);
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
      std::vector<void*> IncomingArguments;
      va_start(ap, ic);
      for (;;) {
        auto Key = va_arg(ap, void*);
        if (Key == 0)
          break;

        auto Value = va_arg(ap, void*);
        IncomingArguments.emplace_back(Key);
        IncomingArguments.emplace_back(Value);
        fprintf(stderr, "%s\n", (char*)Key);
      }

      va_end(ap);

      // Callback memory needs to live beyond internal function call.
      std::list<XIMCallback> Callbacks = ConvertCallbackArguments<XIMCallback>(IncomingArguments);

      // Send a count (not including nullptr);
      auto rv = fexfn_pack_XSetIMValues_internal(ic, IncomingArguments.size(), &IncomingArguments[0]);
      fprintf(stderr, "RV: %p\n", rv);
      return rv;
    }

    _XIC* XCreateIC(XIM im, ...) {
      fprintf(stderr, "XCreateIC\n");
      va_list ap;
      std::vector<void*> IncomingArguments;

      va_start(ap, im);
      for (;;) {
        auto Key = va_arg(ap, void*);
        if (Key == 0)
          break;

        auto Value = va_arg(ap, void*);
        IncomingArguments.emplace_back(Key);
        IncomingArguments.emplace_back(Value);
      }

      va_end(ap);

      // Callback memory needs to live beyond internal function call.
      std::list<XICCallback> Callbacks = ConvertCallbackArguments<XICCallback>(IncomingArguments);

      auto rv = fexfn_pack_XCreateIC_internal(im, IncomingArguments.size(), &IncomingArguments[0]);
      return rv;
    }

    XVaNestedList XVaCreateNestedList(int unused_arg, ...) {
      fprintf(stderr, "XVaCreateNestedList\n");
      va_list ap;
      std::vector<void*> IncomingArguments;

      va_start(ap, unused_arg);
      for (;;) {
        auto Key = va_arg(ap, void*);
        if (Key == 0)
          break;

        auto Value = va_arg(ap, void*);
        IncomingArguments.emplace_back(Key);
        IncomingArguments.emplace_back(Value);
      }

      va_end(ap);

      // Callback memory needs to live beyond internal function call.
      std::list<XICCallback> Callbacks = ConvertCallbackArguments<XICCallback>(IncomingArguments);

      auto rv = fexfn_pack_XVaCreateNestedList_internal(unused_arg, IncomingArguments.size(), &IncomingArguments[0]);
      fprintf(stderr, "RV: %p\n", rv);
      return rv;
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

  Bool XRegisterIMInstantiateCallback(Display* dpy,
    struct _XrmHashBucketRec* rdb,
    char* res_name,
    char* res_class,
    XIDProc callback,
    XPointer client_data
  ) {
    return fexfn_pack_XRegisterIMInstantiateCallback(dpy, rdb, res_name, res_class, AllocateHostTrampolineForGuestFunction(callback), client_data);
  }

  Bool XUnregisterIMInstantiateCallback(
    Display* dpy,
    struct _XrmHashBucketRec* rdb,
    char* res_name,
    char* res_class,
    XIDProc callback,
    XPointer client_data
  ) {
    return fexfn_pack_XUnregisterIMInstantiateCallback(dpy, rdb, res_name, res_class, AllocateHostTrampolineForGuestFunction(callback), client_data);
  }

  void (*_XLockMutex_fn)(LockInfoPtr) = nullptr;
  void (*_XUnlockMutex_fn)(LockInfoPtr) = nullptr;
  LockInfoPtr _Xglobal_lock = (LockInfoPtr)0x4142434445464748ULL;
}

LOAD_LIB(libX11)
