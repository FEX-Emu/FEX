/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <cstdint>
#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "callback_typedefs.inl"

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

    // Implemented manually
    XSetErrorHandlerCBFN* XSetErrorHandler(XErrorHandler a_0) {
        a_0 = HostTrampolineForGuestcall(fexcallback_libX11_XIfEventCBFN, &fexfn_unpack_libX11_XSetErrorHandlerCBFN, a_0);
        auto rv = fexfn_pack_XSetErrorHandler(a_0);

        return GuestcallTargetForHostTrampoline(rv);
    }

    static void LockMutexFunction() {
      fprintf(stderr, "libX11: LockMutex\n");
    }

    static void UnlockMutexFunction() {
      fprintf(stderr, "libX11: LockMutex\n");
    }

  void (*_XLockMutex_fn)() = LockMutexFunction;
  void (*_XUnlockMutex_fn)() = UnlockMutexFunction;
  typedef struct _LockInfoRec *LockInfoPtr;
  LockInfoPtr _Xglobal_lock = (LockInfoPtr)0x4142434445464748ULL;
}

LOAD_LIB(libX11)
