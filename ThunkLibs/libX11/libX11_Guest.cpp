#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "../Thunk.h"
#include <stdarg.h>

LOAD_LIB(libX11)

typedef int XSetErrorHandlerFN(Display*, XErrorEvent*);
typedef int XIfEventFN(Display*, XEvent*, XPointer);

#include "libX11_Thunks.inl"
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
}

