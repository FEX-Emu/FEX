/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include "common/Host.h"

#include <cstdlib>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <dlfcn.h>

#include "callback_structs.inl"
#include "callback_typedefs.inl"

struct CallbackUnpacks {
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
        fprintf(stderr, "XCreateIC_internal FAILURE\n");
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
        fprintf(stderr, "XCreateIC_internal FAILURE\n");
        return ErrorReply;
    }
}

DECL_COPYABLE_TRAMPLOLINE(XUnregisterIMInstantiateCallbackCBFN)
DECL_COPYABLE_TRAMPLOLINE(XRemoveConnectionWatchCBFN)
DECL_COPYABLE_TRAMPLOLINE(XIfEventCBFN)
DECL_COPYABLE_TRAMPLOLINE(XSetErrorHandlerCBFN)

int fexfn_impl_libX11_XIfEvent_internal(Display* a0, XEvent* a1, XIfEventCBFN* a2, XPointer a3) {

    auto fn = BIND_CALLBACK(a2, XIfEventCB, libX11);

    return fexldr_ptr_libX11_XIfEvent(a0, a1, fn, a3);
}

Bool fexfn_impl_libX11_XUnregisterIMInstantiateCallback_internal(
    Display* a0, struct _XrmHashBucketRec* a1,
    char* a2, char* a3, XUnregisterIMInstantiateCallbackCBFN* a4, XPointer a5) {

    auto fn = BIND_CALLBACK(a4, XUnregisterIMInstantiateCallbackCB, libX11);

    return fexldr_ptr_libX11_XUnregisterIMInstantiateCallback(a0, a1, a2, a3, fn, a5);
}

void fexfn_impl_libX11_XRemoveConnectionWatch_internal(Display* a0, XRemoveConnectionWatchCBFN* a1, XPointer a2) {

    auto fn = BIND_CALLBACK(a1, XRemoveConnectionWatchCB, libX11);

    return fexldr_ptr_libX11_XRemoveConnectionWatch(a0, fn, a2);
}

XSetErrorHandlerCBFN* fexfn_impl_libX11_XSetErrorHandler_internal(XSetErrorHandlerCBFN a_0) {
    
    auto fn = BIND_CALLBACK(a_0, XSetErrorHandlerCB, libX11);

    XSetErrorHandlerCBFN* old = fexldr_ptr_libX11_XSetErrorHandler(fn);

    auto guest = binder::get_target(old);
    assert(guest != 0 || old == 0);

    return guest;
}

#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS_WITH_CALLBACKS(libX11) 

