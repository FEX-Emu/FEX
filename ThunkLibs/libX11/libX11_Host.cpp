#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "../Thunk.h"
#include <dlfcn.h>

typedef int XSetErrorHandlerFN(Display*, XErrorEvent*);
typedef int XIfEventFN(Display*, XEvent*, XPointer);

static _XIC *fexthunks_impl_libX11_XCreateIC_internal(XIM a_0, size_t count, unsigned long *list);
static char* fexthunks_impl_libX11_XGetICValues_internal(XIC a_0, size_t count, unsigned long *list);

#include "libX11_Forwards.inl"

_XIC *fexthunks_impl_libX11_XCreateIC_internal(XIM a_0, size_t count, unsigned long *list) {
    switch(count) {
        case 0: return fexthunks_impl_libX11_XCreateIC(a_0, nullptr); break;
        case 1: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], nullptr); break;
        case 2: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], nullptr); break;
        case 3: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], list[2], nullptr); break;
        case 4: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], nullptr); break;
        case 5: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], nullptr); break;
        case 6: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr); break;
        case 7: return fexthunks_impl_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr); break;
        default:
        printf("XCreateIC_internal FAILURE\n");
    }
}

char* fexthunks_impl_libX11_XGetICValues_internal(XIC a_0, size_t count, unsigned long *list) {
    switch(count) {
        case 0: return fexthunks_impl_libX11_XGetICValues(a_0, nullptr); break;
        case 1: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], nullptr); break;
        case 2: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], nullptr); break;
        case 3: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], list[2], nullptr); break;
        case 4: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], nullptr); break;
        case 5: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr); break;
        case 6: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr); break;
        case 7: return fexthunks_impl_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr); break;
        default:
        printf("XCreateIC_internal FAILURE\n");
    }
}


static ExportEntry exports[] = {
    #include "libX11_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libX11) 

