/*
$info$
tags: thunklibs|GL
desc: Uses glXGetProcAddress instead of dlsym
$end_info$
*/

#define XLIB_ILLEGAL_ACCESS

#include <cstdio>
#include <dlfcn.h>

#include "glincludes.inl"

#include "common/Host.h"
#include <shared_mutex>
#include <map>

#define IMPL(x) x

#include "libFGL_private.h"

#include "ldr_ptrs.inl"


namespace fglx {
static std::shared_mutex DisplayMapLock;
static std::map<Display *, Display *> GuestToHost;
static std::map<Display *, Display *> HostToGuest;

static Display *fexfn_impl_libGL_fgl_AddGuestX11(Display *Guest, const char *DisplayName) {
    std::unique_lock lk(DisplayMapLock);
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

static void fexfn_impl_libGL_fgl_RemoveGuestX11(Display *Guest) {
    std::unique_lock lk(DisplayMapLock);
    auto Host = GuestToHost.find(Guest);

    if (Host != GuestToHost.end()) {
        XCloseDisplay(Host->second);
        HostToGuest.erase(Host->second);
    }

    GuestToHost.erase(Guest);
}

static Display *fexfn_impl_libGL_fgl_HostToGuestX11(Display *Host) {
    std::shared_lock lk(DisplayMapLock);

    auto rv = HostToGuest.find(Host);
    if (rv != HostToGuest.end()) {
        return rv->second;
    } else {
        dbgf("fgl: Failed to map Host Display %p ('%s') to Guest\n", Host, Host->display_name);
        return nullptr;
    }
}

static Display *fexfn_impl_libGL_fgl_GuestToHostX11(Display *Guest) {
    std::shared_lock lk(DisplayMapLock);
    auto rv = GuestToHost.find(Guest);

    if (rv != GuestToHost.end()) {
        return rv->second;
    } else {
        return nullptr;
    }
}

static void fexfn_impl_libGL_fgl_XFree(void *p) {
    XFree(p);
}

static void fexfn_impl_libGL_fgl_FlushFromGuestX11(Display *Guest) {
    auto Host = fexfn_impl_libGL_fgl_GuestToHostX11(Guest);

    if (Host) {
        XSync(Host, False);
    }
}

}

namespace fegl {
static std::shared_mutex DisplayMapLockEGL;
static std::map<EGLDisplay, Display *> HostToXGuestEGL;
static std::map<EGLDisplay, Display *> HostToXHostEGL;
static std::map<Display *, Display *> XHostToXGuestEGL;
static std::map<Display *, Display *> XGuestToXHostEGL;


static Display *fexfn_impl_libGL_fgl_AddXGuestEGL(Display *XGuest, const char *DisplayName) {
    std::unique_lock lk(DisplayMapLockEGL);
    auto XHost = XOpenDisplay(DisplayName);
    if (XHost) {
      dbgf("fegl: Mapping Guest Display %p ('%s') to Host %p\n", XGuest, DisplayName, XHost);
      XGuestToXHostEGL[XGuest] = XHost;
      XHostToXGuestEGL[XHost] = XGuest;
      return XHost;
    } else {
      dbgf("fegl: Failed to open Guest Display %p ('%s') on Host\n", XGuest, DisplayName);
      return nullptr;
    }
}

static void fexfn_impl_libGL_fgl_RemoveXGuestEGL(Display *XGuest) {
    std::unique_lock lk(DisplayMapLockEGL);
    Display *XHost = nullptr;

    {   
        auto itXHost = XGuestToXHostEGL.find(XGuest);

        if (itXHost != XGuestToXHostEGL.end()) {
            XHost = itXHost->second;
            XCloseDisplay(XHost);
        }
    }

    XGuestToXHostEGL.erase(XGuest);
    XGuestToXHostEGL.erase(XHost);

    for (auto it = HostToXHostEGL.begin(); it != HostToXHostEGL.end(); ) {
        if (it->second == XHost) {
            it = HostToXHostEGL.erase(it);
        } else {
            it++;
        }
    }

    for (auto it = HostToXGuestEGL.begin(); it != HostToXGuestEGL.end(); ) {
        if (it->second == XGuest) {
            it = HostToXHostEGL.erase(it);
        } else {
            it++;
        }
    }
}


static Display *fexfn_impl_libGL_fgl_XGuestToXHostEGL(Display *XGuest) {
    std::shared_lock lk(DisplayMapLockEGL);
    auto rv = XGuestToXHostEGL.find(XGuest);

    if (rv != XGuestToXHostEGL.end()) {
        return rv->second;
    } else {
        return nullptr;
    }
}

static Display *fexfn_impl_libGL_fgl_HostToXGuestEGL(EGLDisplay Host) {
    std::shared_lock lk(DisplayMapLockEGL);

    auto XGuest = HostToXGuestEGL.find(Host);

    if (XGuest != HostToXGuestEGL.end()) {
        return XGuest->second;
    } else {
        return nullptr;
    }
}

static Display *fexfn_impl_libGL_fgl_FlushFromHostEGL(EGLDisplay Host) {
    std::shared_lock lk(DisplayMapLockEGL);

    auto XHost = HostToXHostEGL.find(Host);

    if (XHost != HostToXHostEGL.end()) {
        XSync(XHost->second, False);
    }

    // same as fexfn_impl_libGL_fgl_HostToXGuestEGL
    auto XGuest = HostToXGuestEGL.find(Host);

    if (XGuest != HostToXGuestEGL.end()) {
        return XGuest->second;
    } else {
        return nullptr;
    }
}

static EGLDisplay fexfn_impl_libGL_eglGetDisplay(NativeDisplayType native_display) {
    if (native_display == EGL_DEFAULT_DISPLAY) {
        return fexldr_ptr_libGL_eglGetDisplay(native_display);
    } else {
        auto XHost = (Display*)native_display;

        auto Host = fexldr_ptr_libGL_eglGetDisplay(native_display);

        if (Host == EGL_NO_DISPLAY) {
            return Host;
        } else {
            std::unique_lock lk(DisplayMapLockEGL);
            auto XGuest = XHostToXGuestEGL.find(XHost);
            if (XGuest == XHostToXGuestEGL.end()) {
                dbgf("fegl: eglGetDisplay failed to map XHost %p, Host %p to XGuest\n", XHost, Host);
            } else {
                HostToXHostEGL[Host] = XHost;
                HostToXGuestEGL[Host] = XGuest->second;

                dbgf("fegl: eglGetDisplay mapped Host %p to (XHost %p, XGuest %p)\n", Host, XHost, XGuest->second);
            }
            return Host;
        }
    }
}

static void fexfn_impl_libGL_glDebugMessageCallbackAMD_internal(GLDEBUGPROCAMD, const void*) {
    errf("%s: Stubbed\n", __FUNCTION__);
}

static void fexfn_impl_libGL_glDebugMessageCallbackARB_internal(GLDEBUGPROCARB, const void*) {
    errf("%s: Stubbed\n", __FUNCTION__);
}

static void fexfn_impl_libGL_glDebugMessageCallback_internal(GLDEBUGPROC, const void*) {
    errf("%s: Stubbed\n", __FUNCTION__);
}

static void *symbolFromGlXGetProcAddr(void *, const char *name) {
    return (void*)glXGetProcAddress((const GLubyte*)name);
}

static void *symbolFromEglGetProcAddr(void *, const char *name) {
    return (void*)eglGetProcAddress(name);
}

}

using namespace fglx;
using namespace fegl;
#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS_WITH_LIBNAME(libFGL, libGL)
