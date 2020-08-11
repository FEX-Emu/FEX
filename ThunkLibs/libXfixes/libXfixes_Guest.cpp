#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "Thunk.h"
#include <stdarg.h>

LOAD_LIB(libXfixes)

#include "libXfixes_thunks.inl"
#include <vector>