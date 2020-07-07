#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include "../Thunk.h"
#include <dlfcn.h>

#include "libXrender_Forwards.inl"

static ExportEntry exports[] = {
    #include "libXrender_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libXrender) 

