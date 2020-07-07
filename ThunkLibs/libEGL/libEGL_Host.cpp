#include <cstdio>
#include <dlfcn.h>

#include <EGL/egl.h>

#include "../Thunk.h"

#include "libEGL_Forwards.inl"

static ExportEntry exports[] = {
    #include "libEGL_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libEGL) 