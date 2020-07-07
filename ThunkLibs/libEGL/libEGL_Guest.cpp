#include <GL/glx.h>
#include <EGL/egl.h>

#include <stdio.h>
#include <cstring>

#include "../Thunk.h"

LOAD_LIB(libEGL)

#include "libEGL_Thunks.inl"

typedef void voidFunc();


extern "C" {
	voidFunc *eglGetProcAddress(const char *procname) {
		// TODO: Fix this HACK
		return glXGetProcAddress((const GLubyte*)procname);
	}
}

