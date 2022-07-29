/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <cstdlib>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#undef min
#undef max

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <X11/Xproto.h>

#include <X11/extensions/XKBstr.h>

#include "common/Host.h"
#include <dlfcn.h>
#include <utility>

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
        fprintf(stderr, "XCreateICValues_internal FAILURE\n");
        return ErrorReply;
    }
}

char* fexfn_impl_libX11_XGetIMValues_internal(XIM a_0, size_t count, void **list) {
    switch(count) {
        case 0: return fexldr_ptr_libX11_XGetIMValues(a_0, nullptr); break;
        case 1: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], nullptr); break;
        case 2: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], nullptr); break;
        case 3: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], nullptr); break;
        case 4: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], nullptr); break;
        case 5: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], nullptr); break;
        case 6: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], nullptr); break;
        case 7: return fexldr_ptr_libX11_XGetIMValues(a_0, list[0], list[1], list[2], list[3], list[4], list[5], list[6], nullptr); break;
        default:
        fprintf(stderr, "XCreateIMValues_internal FAILURE\n");
        return ErrorReply;
    }
}

Status fexfn_impl_libX11_XInitThreadsInternal(uintptr_t, uintptr_t);

Status fexfn_impl_libX11__XReply(Display*, xReply*, int, Bool);

#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

static int (*ACTUAL_XInitDisplayLock_fn)(Display*) = nullptr;
static int (*INTERNAL_XInitDisplayLock_fn)(Display*) = nullptr;

static int _XInitDisplayLock(Display* display) {
    auto ret = ACTUAL_XInitDisplayLock_fn(display);
    INTERNAL_XInitDisplayLock_fn(display);
    return ret;
}

Status fexfn_impl_libX11_XInitThreadsInternal(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
    auto ret = fexldr_ptr_libX11_XInitThreads();
    auto _XInitDisplayLock_fn = (int(**)(Display*))dlsym(fexldr_ptr_libX11_so, "_XInitDisplayLock_fn");
    ACTUAL_XInitDisplayLock_fn = std::exchange(*_XInitDisplayLock_fn, _XInitDisplayLock);
    MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &INTERNAL_XInitDisplayLock_fn);
    return ret;
}

Status fexfn_impl_libX11__XReply(Display* display, xReply* reply, int extra, Bool discard) {
    for(auto handler = display->async_handlers; handler; handler = handler->next) {
        FinalizeHostTrampolineForGuestFunction(handler->handler);
    }
    return fexldr_ptr_libX11__XReply(display, reply, extra, discard);
}

EXPORTS(libX11)
