#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

#include "Thunk.h"
#include <dlfcn.h>

#include "libXfixes_initializers.inl"
#include "libXfixes_forwards.inl"

static ExportEntry exports[] = {
    #include "libXfixes_thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libXfixes) 

