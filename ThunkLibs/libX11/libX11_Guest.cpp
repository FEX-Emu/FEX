#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "callback_typedefs.inl"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

#include "callback_unpacks.inl"

// Custom implementations //

#include <vector>

extern "C" {
    char* XGetICValues(XIC ic, ...) {
        printf("XGetICValues\n");
        va_list ap;
        std::vector<unsigned long> args;
        va_start(ap, ic); 
        for (;;) {
            auto arg = va_arg(ap, unsigned long);
            if (arg == 0)
                break;
            args.push_back(arg);
            printf("%016lX\n", arg);
        }
            
        va_end(ap);
        auto rv = XGetICValues_internal(ic, args.size(), &args[0]);
        printf("RV: %p\n", rv);
        return rv;
    }
    
    _XIC* XCreateIC(XIM im, ...) {
        printf("XCreateIC\n");
        va_list ap;
        std::vector<unsigned long> args;
        va_start(ap, im); 
        for (;;) {
            auto arg = va_arg(ap, unsigned long);
            if (arg == 0)
                break;
            args.push_back(arg);
            printf("%016lX\n", arg);
        }
            
        va_end(ap);
        auto rv = XCreateIC_internal(im, args.size(), &args[0]);
        printf("RV: %p\n", rv);
        return rv;
    }

    int XIfEvent(Display* a0, XEvent* a1, XIfEventCBFN* a2, XPointer a3) {
        return XIfEvent_internal(a0, a1, a2, a3);
    }

    XSetErrorHandlerCBFN* XSetErrorHandler(XErrorHandler a_0) {
        return XSetErrorHandler_internal(a_0);
    }

    int (*XESetCloseDisplay(Display *display, int extension, int (*proc)()))() {
        printf("libX11: XESetCloseDisplay\n");
        return nullptr;
    }
      
}

struct {
    #include "callback_unpacks_header.inl"
} callback_unpacks = {
    #include "callback_unpacks_header_init.inl"
};

LOAD_LIB_WITH_CALLBACKS(libX11)