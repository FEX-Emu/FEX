/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <cstdlib>
#include <stdio.h>

#include <X11/Xlib.h>

// Include Xlibint.h and undefine some of its macros that clash with the standard library
#include <X11/Xlibint.h>
#undef min
#undef max

#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include <X11/extensions/XKBstr.h>

#include "common/Host.h"
#include <dlfcn.h>

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

#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libX11)
