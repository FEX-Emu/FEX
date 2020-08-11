#include <cstdio>
#include <dlfcn.h>

#include <EGL/egl.h>

#include "Thunk.h"

#include "libEGL_initializers.inl"
#include "libEGL_forwards.inl"

static ExportEntry exports[] = {
    #include "libEGL_thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libEGL) 