/*
$info$
tags: thunklibs|GL
desc: Uses glXGetProcAddress instead of dlsym
$end_info$
*/

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

void fexfn_impl_libGL_glDebugMessageCallbackAMD_internal(GLDEBUGPROCAMD, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void fexfn_impl_libGL_glDebugMessageCallbackARB_internal(GLDEBUGPROCARB, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void fexfn_impl_libGL_glDebugMessageCallback_internal(GLDEBUGPROC, const void*) {
    fprintf(stderr, "%s: Stubbed\n", __FUNCTION__);
}

void* symbolFromGlXGetProcAddr(void*, const char* name) {
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
