/*
$info$
tags: thunklibs|GL
desc: Uses glXGetProcAddress instead of dlsym
$end_info$
*/

#include <cstdio>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

#include <dlfcn.h>

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <cstdlib>

#include "common/Host.h"

#define IMPL(Name) fexfn_impl_libGL_##Name

#define dbgf(...) //fprintf(stderr, __VA_ARGS__)
#define errf(...) fprintf(stderr, __VA_ARGS__)

namespace glx {

static std::shared_mutex DisplayMapLock;
static std::unordered_map<Display *, Display *> GuestToHost;
static std::unordered_map<Display *, Display *> HostToGuest;

static Display *IMPL(px11_AddGuestX11)(Display *Guest, const char *DisplayName) {
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

static void IMPL(px11_RemoveGuestX11)(Display *Guest) {
    std::unique_lock lk(DisplayMapLock);
    auto Host = GuestToHost.find(Guest);

    if (Host != GuestToHost.end()) {
        XCloseDisplay(Host->second);
        HostToGuest.erase(Host->second);
    }

    GuestToHost.erase(Guest);
}

static Display *IMPL(px11_HostToGuestX11)(Display *Host) {
    std::shared_lock lk(DisplayMapLock);

    auto rv = HostToGuest.find(Host);
    if (rv != HostToGuest.end()) {
        return rv->second;
    } else {
        dbgf("fgl: Failed to map Host Display %p ('') to Guest\n", Host);
        return nullptr;
    }
}

static Display *IMPL(px11_GuestToHostX11)(Display *Guest) {
    std::shared_lock lk(DisplayMapLock);
    auto rv = GuestToHost.find(Guest);

    if (rv != GuestToHost.end()) {
        return rv->second;
    } else {
        return nullptr;
    }
}

static void IMPL(px11_XFree)(void *p) {
    XFree(p);
}

static XVisualInfo *IMPL(px11_XVisual)(Display *Guest, int screen, unsigned int XVisual) {
    XVisualInfo v;

	dbgf("px11_XVisual: %d, %u\n", screen, XVisual);

	// FEX_TODO("HostVis might not be same as guest XVisualInfo here")
	v.screen = screen;
	v.visualid = XVisual;//HostVis->visualid;

	//dbgf("%d, %lu\n", v.screen, v.visualid);

    
	int c;
	auto vguest = XGetVisualInfo(IMPL(px11_GuestToHostX11)(Guest), VisualScreenMask | VisualIDMask, &v, &c);

	if (c >= 1 && vguest != nullptr) {
		return vguest;
	} else {
		errf("MapVisualInfoHostToGuest: Guest XGetVisualInfo returned null\n");
		return nullptr;
	}
}


static void IMPL(px11_FlushFromGuestX11)(Display *Guest) {
    auto Host = IMPL(px11_GuestToHostX11)(Guest);

    if (Host) {
        XSync(Host, False);
    }
}

}

void IMPL(glDebugMessageCallbackAMD_internal)(GLDEBUGPROCAMD, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void IMPL(glDebugMessageCallbackARB_internal)(GLDEBUGPROCARB, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void IMPL(glDebugMessageCallback_internal)(GLDEBUGPROC, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void* symbolFromGlXGetProcAddr(void*, const char* name) {
    return (void*)glXGetProcAddress((const GLubyte*)name);
}

#include "ldr_ptrs.inl"

using namespace glx;
#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

EXPORTS(libGL)
