#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include "common/Host.h"
#include <dlfcn.h>

#include "callback_structs.inl"
#include "callback_typedefs.inl"

struct {
    #include "callback_unpacks_header.inl"
} *callback_unpacks;

#include "ldr_ptrs.inl"

_XIC *fexfn_impl_libX11_XCreateIC_internal(XIM a_0, size_t count, unsigned long *list) {
    switch(count) {
        case 0: return fexldr_ptr_libX11_XCreateIC(a_0, nullptr); break;
        case 1: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], nullptr); break;
        case 2: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], nullptr); break;
        case 3: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], nullptr); break;
        case 4: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], nullptr); break;
        case 5: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], nullptr); break;
        case 6: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr); break;
        case 7: return fexldr_ptr_libX11_XCreateIC(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr); break;
        default:
        printf("XCreateIC_internal FAILURE\n");
        return nullptr;
    }
}

static char ErrorReply[] = "FEX: Unable to match arg count";

char* fexfn_impl_libX11_XGetICValues_internal(XIC a_0, size_t count, unsigned long *list) {
    switch(count) {
        case 0: return fexldr_ptr_libX11_XGetICValues(a_0, nullptr); break;
        case 1: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], nullptr); break;
        case 2: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], nullptr); break;
        case 3: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], nullptr); break;
        case 4: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], nullptr); break;
        case 5: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr); break;
        case 6: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr); break;
        case 7: return fexldr_ptr_libX11_XGetICValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr); break;
        default:
        printf("XCreateIC_internal FAILURE\n");
        return ErrorReply;
    }
}

struct XIfEventCB_args {
    XIfEventCBFN *fn;
    XPointer arg;
};

static int XIfEventCB(Display* a0, XEvent* a1, XPointer a2) {

    XIfEventCB_args *arg = (XIfEventCB_args*)a2;

    XIfEventCB_Args argsrv { a0, a1, arg->arg};

    call_guest(callback_unpacks->libX11_XIfEventCB, (void*) arg->fn, &argsrv);
    
    return argsrv.rv;
}

int fexfn_impl_libX11_XIfEvent_internal(Display* a0, XEvent* a1, XIfEventCBFN* a2, XPointer a3) {
    XIfEventCB_args args = { a2, a3 };

    return fexldr_ptr_libX11_XIfEvent(a0, a1, &XIfEventCB, (XPointer)&args);
}

XErrorHandler guest_handler;

int XSetErrorHandlerCB(Display* a_0, XErrorEvent* a_1) {
    XSetErrorHandlerCB_Args argsrv { a_0, a_1};
    
    call_guest(callback_unpacks->libX11_XSetErrorHandlerCB, (void*) guest_handler, &argsrv);
    
    return argsrv.rv;
}

XSetErrorHandlerCBFN* fexfn_impl_libX11_XSetErrorHandler_internal(XErrorHandler a_0) {
    auto old = guest_handler;
    guest_handler = a_0;

    fexldr_ptr_libX11_XSetErrorHandler(&XSetErrorHandlerCB);
    return old;
}

#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS_WITH_CALLBACKS(libX11) 

