#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include "Thunk.h"
#include <dlfcn.h>

#include "libXrender_initializers.inl"
#include "libXrender_forwards.inl"

static ExportEntry exports[] = {
    #include "libXrender_thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libXrender) 

