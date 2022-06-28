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

#include <stdio.h>
#include <cstdlib>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "common/Guest.h"

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"
#include "symbol_list.inl"

typedef void voidFunc();

// Maps OpenGL API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers =
    std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(fexfn_pack_hostcall_##name);
        std::unordered_map<std::string_view, uintptr_t> Ret;
        FOREACH_internal_SYMBOL(PAIR)
        return Ret;
#undef PAIR
    });

extern "C" {
	voidFunc *glXGetProcAddress(const GLubyte *procname) {
    auto Ret = fexfn_pack_glXGetProcAddress(procname);
    if (!Ret) {
        return nullptr;
    }

    auto TargetFuncIt = HostPtrInvokers.find(reinterpret_cast<const char*>(procname));
    if (TargetFuncIt == HostPtrInvokers.end()) {
      // Extension found in host but not in our interface definition => treat as fatal error
      fprintf(stderr, "glXGetProcAddress: not found %s\n", procname);
      __builtin_trap();
    }

    LinkHostAddressToGuestFunction((uintptr_t)Ret, TargetFuncIt->second);
    return Ret;
	}

	voidFunc *glXGetProcAddressARB(const GLubyte *procname) {
		return glXGetProcAddress(procname);
	}
}

LOAD_LIB(libGL)
