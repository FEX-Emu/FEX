#include <cstdio>
#include <dlfcn.h>

#define GL_GLEXT_PROTOTYPES 1
#define GLX_GLXEXT_PROTOTYPES 1

#include "glcorearb.h"

#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include "../Thunk.h"

#include "libGL_Forwards.inl"

static ExportEntry exports[] = {
    #include "libGL_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libGL) 