/*
$info$
tags: thunklibs|GL
desc: Uses glXGetProcAddress instead of dlsym
$end_info$
*/

#define XLIB_ILLEGAL_ACCESS

#include <cstdio>
#include <dlfcn.h>

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "common/Host.h"
#include <shared_mutex>
#include <map>

#include "private_api.h"

static std::shared_mutex DisplayMapLock;
static std::map<Display *, Display *> GuestToHost;
static std::map<Display *, Display *> HostToGuest;

static Display *fexfn_impl_libGL_fgl_HostToGuest(Display *Host) {
    std::shared_lock lk(DisplayMapLock);

    auto rv = HostToGuest.find(Host);
    if (rv != HostToGuest.end()) {
        return rv->second;
    } else {
        dbgf("fgl: Failed to map Host Display %p ('%s') to Guest\n", Host, Host->display_name);
        return nullptr;
    }
}

static Display *fexfn_impl_libGL_fgl_GuestToHost(Display *Guest, const char *DisplayName) {
    std::unique_lock lk(DisplayMapLock);
    auto rv = GuestToHost.find(Guest);

    if (rv != GuestToHost.end()) {
        return rv->second;
    } else {
        auto Host = XOpenDisplay(DisplayName);
        if (Host) {
            dbgf("fgl: Mapping Guest Display %p ('%s') to Host %p\n", Guest, DisplayName, Host);
            GuestToHost[Guest] = Host;
            HostToGuest[Host] = Guest;
            return Host;
        } else {
            dbgf("fgl: Failed to open Guest Display %p ('%s') on Host\n", Guest, DisplayName);
            return nullptr;
        }
    }
}

static void fexfn_impl_libGL_fgl_XFree(void *p) {
    XFree(p);
}
static void fexfn_impl_libGL_fgl_FlushFromGuest(Display *Guest, const char *DisplayName) {
    auto Host = fexfn_impl_libGL_fgl_GuestToHost(Guest, DisplayName);

    if (Host) {
        XFlush(Host);
    }
}


static void fexfn_impl_libGL_glDebugMessageCallbackAMD_internal(GLDEBUGPROCAMD, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

static void fexfn_impl_libGL_glDebugMessageCallbackARB_internal(GLDEBUGPROCARB, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

static void fexfn_impl_libGL_glDebugMessageCallback_internal(GLDEBUGPROC, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

static void* symbolFromGlXGetProcAddr(void*, const char* name) {
    return (void*)glXGetProcAddress((const GLubyte*)name);
}

#include "ldr_ptrs.inl"
#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libGL)
