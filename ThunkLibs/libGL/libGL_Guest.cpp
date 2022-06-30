/*
$info$
tags: thunklibs|GL
desc: Handles glXGetProcAddress
$end_info$
*/

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#undef GL_ARB_viewport_array
#include "glcorearb.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <X11/Xmu/CloseHook.h>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"
#include "symbol_list.inl"

typedef void voidFunc();

#define dbgf(...) //printf
#define errf(...) fprintf(stderr, __VA_ARGS__)

#define IMPL(Name) Name
#define PACKER(Name) fexfn_pack_##Name

// px11 is enabled when libX11 is not thunked
static const bool px11_Enabled = !IsLibLoaded("libX11");

namespace glx {

FEX_PACKFN_LINKAGE XVisualInfo *IMPL(MapVisualInfoHostToGuest)(Display *dpy, XVisualInfo *HostVis) {
    if (!px11_Enabled) {
        return HostVis;
    }

	if (!HostVis) {
		dbgf("MapVisualInfoHostToGuest: Can't map null HostVis\n");
		return nullptr;
	}

	XVisualInfo v;

	// FEX_TODO("HostVis might not be same as guest XVisualInfo here")
	v.screen = HostVis->screen;
	v.visualid = HostVis->visualid;

	PACKER(px11_XFree)(HostVis);

	int c;
	auto vguest = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask, &v, &c);

	if (c >= 1 && vguest != nullptr) {
		return vguest;
	} else {
		errf("MapVisualInfoHostToGuest: Guest XGetVisualInfo returned null\n");
		return nullptr;
	}
}

FEX_PACKFN_LINKAGE XVisualInfo *IMPL(MapVisualInfoGuestToHost)(Display *dpy, XVisualInfo *GuestVis) {
	// FEX_TODO("Implement this")
	return GuestVis;
}

FEX_PACKFN_LINKAGE GLXFBConfig *IMPL(MapGLXFBConfigHostToGuest)(GLXFBConfig *Host, int count) {
    if (!px11_Enabled) {
        return Host;
    }

	if (!Host || count <= 0) {
		dbgf("MapGLXFBConfigHostToGuest: Host (%p) is null or count (%d) <= 0\n", Host, count);
		return nullptr;
	}

	auto rv = (GLXFBConfig *)Xmalloc(sizeof(GLXFBConfig) *count);

	if (!rv) {
		dbgf("MapGLXFBConfigHostToGuest: Xmalloc failed\n");
		return nullptr;
	}

	for (int i = 0; i < count; i++) {
		rv[i] = Host[i];
	}

	PACKER(px11_XFree)(Host);

	return rv;
}

FEX_PACKFN_LINKAGE GLXFBConfigSGIX *IMPL(MapGLXFBConfigSGIXHostToGuest)(GLXFBConfigSGIX *Host, int count) {
    if (!px11_Enabled) {
        return Host;
    }

	if (!Host || count <= 0) {
		dbgf("MapGLXFBConfigSGIXHostToGuest: Host (%p) is null or count (%d) <= 0\n", Host, count);
		return nullptr;
	}

	auto rv = (GLXFBConfigSGIX *)Xmalloc(sizeof(GLXFBConfigSGIX) *count);

	if (!rv) {
		dbgf("MapGLXFBConfigSGIXHostToGuest: Xmalloc failed\n");
		return nullptr;
	}

	for (int i = 0; i < count; i++) {
		rv[i] = Host[i];
	}

	PACKER(px11_XFree)(Host);

	return rv;
}

static int DisplayCloseCallback(Display *dpy, XPointer data) {
    assert(px11_Enabled);
	PACKER(px11_RemoveGuestX11)(dpy);
	return 0;
}

FEX_PACKFN_LINKAGE Display *IMPL(GuestToHostX11)(Display *dpy) {
    if (!px11_Enabled) {
        return dpy;
    }
    
	auto rv = PACKER(px11_GuestToHostX11)(dpy);

	if (!rv) {
		XmuAddCloseDisplayHook(dpy, &DisplayCloseCallback, nullptr);
		rv = PACKER(px11_AddGuestX11)(dpy, XDisplayString(dpy));

		if (!rv) {
			errf("GuestToHostX11: px11_AddGuestX11 failed\n");
		}
	}
	return rv;
}

#define SYNC_GUEST_HOST() do { if (px11_Enabled) { XSync(dpy, False); PACKER(px11_FlushFromGuestX11)(dpy); } } while(0)
#define SYNC_HOST_GUEST() do { if (px11_Enabled) { PACKER(px11_FlushFromGuestX11)(dpy); XSync(dpy, False); } } while(0)


#define PACKER_OPTIONAL_HOSTCALL(Name) PACKER(Name)
#define OPTIONAL_HOSTCALL_ABI
#define OPTIONAL_HOSTCALL_LASTARG
#define OPTIONAL_HOSTCALL_ONLYARG

extern "C" {
    #include "glx.inl"
}

#undef PACKER_OPTIONAL_HOSTCALL
#undef OPTIONAL_HOSTCALL_ABI
#undef OPTIONAL_HOSTCALL_LASTARG
#undef OPTIONAL_HOSTCALL_ONLYARG

#undef IMPL

#define IMPL(Name) hostcall_##Name
#define PACKER_OPTIONAL_HOSTCALL(Name) fexfn_hostaddr_pack_hostcall_##Name
#define OPTIONAL_HOSTCALL_ABI CUSTOM_ABI_HOST_ADDR;
#define OPTIONAL_HOSTCALL_LASTARG , host_addr
#define OPTIONAL_HOSTCALL_ONLYARG host_addr

#include "glx.inl"

#undef PACKER_OPTIONAL_HOSTCALL
#undef OPTIONAL_HOSTCALL_ABI
#undef OPTIONAL_HOSTCALL_LASTARG
#undef OPTIONAL_HOSTCALL_ONLYARG

}

#undef IMPL

using namespace glx;

// Maps OpenGL API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers =
    std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(hostcall_##name);
        std::unordered_map<std::string_view, uintptr_t> Ret;
        FOREACH_internal_SYMBOL(PAIR)
        return Ret;
#undef PAIR
    });

extern "C" {
	voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
        auto Ret = PACKER(glXGetProcAddressARB)(procname);
        if (!Ret) {
            return nullptr;
        }

        auto TargetFuncIt = HostPtrInvokers.find(reinterpret_cast<const char*>(procname));
        if (TargetFuncIt == HostPtrInvokers.end()) {
        // Extension found in host but not in our interface definition => treat as fatal error
        fprintf(stderr, "glXGetProcAddress: not found %s\n", procname);
        __builtin_trap();
        }

        LinkAddressToFunction((uintptr_t)Ret, TargetFuncIt->second);
        return Ret;
	}

    // This is a mesa-only extension as GLX 1.4 has not been ratified
	voidFunc *glXGetProcAddress(const GLubyte *procname) {
		return glXGetProcAddressARB(procname);
	}
}

LOAD_LIB(libGL)
