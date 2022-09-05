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

#include <cstdlib>

#include "common/Host.h"

#include "thunkgen_host_libGL.inl"

void* symbolFromGlXGetProcAddr(void*, const char* name) {
    return (void*)glXGetProcAddress((const GLubyte*)name);
}

EXPORTS(libGL)
