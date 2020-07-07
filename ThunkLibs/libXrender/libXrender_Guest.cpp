#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "../Thunk.h"
#include <stdarg.h>

LOAD_LIB(libXrender)

#include "libXrender_Thunks.inl"
#include <vector>